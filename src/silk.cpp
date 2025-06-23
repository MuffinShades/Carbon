#include "silk.hpp"
#include <type_traits>

Silk::TPool::TPool(size_t nThreads) {
	if (nThreads <= 0) return;

	size_t max = std::thread::hardware_concurrency();

	if (max < 1) {
		std::cout << "Thread pool error! No Thread available" << std::endl;
		return;
	}

	if (nThreads > max - 1)
		nThreads = max - 1; //yk gotta leave at least one

	//create all the threads
	forrange(nThreads) {
		this->pool.emplace_back(&TPool::work, this);
	}

	this->nThreads = nThreads;
}

void Silk::TPool::work() {
	for (;;) {
		//Task* tsk = nullptr;

		std::function<void()> tsk;

		{
			std::unique_lock<std::mutex> w_lock(this->guard);

			this->cv.wait(w_lock, [this]() {
				return this->w_shutdown || !this->q.empty();
			});

			if (this->w_shutdown && this->q.empty()) {
				break;
			}
			
			if (!this->q.empty()) {
				tsk = this->q.front();
				this->q.pop();
			} else {
				continue;
			}
		}

		try {
			tsk();
		}
		catch (...) {
			std::cout << "Failed to execute task!" << std::endl;
		}
	}
}

bool Silk::Task::execute() {
	try {
		
	}
	catch (...) {
		return false;
	}

	std::cout << "Exe task..." << std::endl;
	this->obj();

	return true;
}

Silk::TPool::~TPool() {
	{
		std::unique_lock<std::mutex> lock(this->guard);
		this->w_shutdown = true;
	}

	cv.notify_all();

	for (auto& w : this->pool)
		if (w.joinable())
			w.join();
}

/*void Silk::TWorker::operator()() {
	if (!pool)
		return;

	Silk::QLock t_lock;

	while (!pool->w_shutdown) {

		//wait for something to do
		//TODO: add a safe queue like in the github
		//		repo that uses mutex
		if (pool->q.empty()) {
			pool->barrier.lock(&t_lock);
			t_lock.wait();

			Task* tsk = t_lock.getObjective();

			if (!tsk)
				continue;

			if (!tsk->execute())
				std::cout << "Failed to execute thread fn!" << std::endl;
		}
		else {
			Task t = std::move(pool->q.front());
			pool->q.pop();

			if (!t.execute())
				std::cout << "Failed to execute thread fn!" << std::endl;
		}
	}
}


void Silk::QLock::wait() {
	this->_waiting = true;
	while (this->_waiting) {
		if (this->idle_count++ > 0xffffff)
			this->idle_count = 0;
	}
}

void Silk::QBarrier::lock(QLock* l) {
	if (!l)
		return;

	this->locks.push_back(l);
}

void Silk::QBarrier::notify_one() {
	size_t sz;

	if (sz = this->locks.size() <= 0)
		return;

	QLock* t = this->locks[sz - 1];
	if (t)
		t->_waiting = false;
	this->locks.pop_back();
}*/