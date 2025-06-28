#pragma once
#include "msutil.hpp"
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <future>
#include <functional>
/*

Silk

thread pooling thing

doesn't use mutex since I am chad programmer - muffinshades 2025
	- if this doesn't work or causes strange errors, could be 
	  because of custom mutex stuff, so just kinda copy the logic 
	  from https://github.com/mtrebi/thread-pool/blob/master/include/ThreadPool.h
	  in that cause and call it a day
	
it does use mutex since I am not chad programmer - muffinshades 2025

*/

namespace Silk {
	/*class QLock {
	private:
		bool _waiting = false;
		Task* obj = nullptr;
		size_t idle_count = 0;
	public:
		void wait();
		Task* getObjective();
		friend class QBarrier;
	};

	class QBarrier {
	private:
		std::vector<QLock*> locks;
	public:
		void lock(QLock* l);
		void notify_one();
	};*/

	enum ThreadErr {
		ThreadErr_Unknown,
		ThreadErr_InvalidFnPtr,
		ThreadErr_PoolNoCreated,
		ThreadErr_FailedToExecute
	};

	/*class TFail : public std::exception {
	private:
		ThreadErr __reason = ThreadErr_Unknown;
	public:
		TFail(ThreadErr reason) : __reason(reason), std::exception("Silk, thread error") {

		}
		ThreadErr reason() {
			return this->__reason;
		}
	};*/

	class Task {
	private:
		std::function<void()> obj;
	public:
		Task(std::function<void()> __obj) : obj(__obj) {};
		bool execute();
	};

	class TPool {
	private:
		//QBarrier barrier;
		std::queue<std::function<void()>> q;
		std::vector<std::thread> pool;
		size_t nThreads = 0;
		bool w_shutdown = false;
		std::mutex guard;
		std::condition_variable cv;
		void work();
	public:
		friend class TWorker;
		friend std::thread;
		TPool(size_t nThreads);
		template<typename _Fn, typename... _Args> auto Exe(_Fn&& func, _Args&&... args) -> std::future<decltype(func(args...))> {
			if (this->nThreads <= 0)
				throw std::invalid_argument("Not enough threads!");

			auto fn_bind = std::bind(std::forward<_Fn>(func), std::forward<_Args>(args)...); //bind the function 
			auto fn_ptr = std::make_shared<std::packaged_task<decltype(func(args...))()>>(fn_bind); //create a function pointer

			if (!fn_ptr)
				throw std::invalid_argument("idk");

			auto t_res = fn_ptr->get_future();
			{
				std::unique_lock<std::mutex> lock(this->guard);
				this->q.emplace(
					[fn_ptr]() {
						if (fn_ptr)
							(*fn_ptr)();
					}
				);
			}

			this->cv.notify_one();

			return t_res;
		}
		~TPool();
	};
};