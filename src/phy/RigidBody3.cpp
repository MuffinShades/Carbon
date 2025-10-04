#include "RigidBody3.hpp"

void rigid_pre_compute(RigidBody3 *rb) {
    if (!rb)
        return;
}

void RigidBody3::addForce(Force F) {
    this->a = this->a + F.F * this->imass;
    this->center = this->obj_center + this->p;
    this->torque = this->torque + vec3::CrossProd(this->center - F.pos, F.F);
}

void RigidBody3::tick(f32 dt) {
    this->adjustITensor();

    this->aa = this->aa + this->iI * this->torque;

    this->v = this->v + this->a * dt;
    this->av = this->av + this->av * dt;

    constexpr f32 damp_fac = 0.05f;
    const f32 damp = powf(damp_fac, dt);
    
    this->p = this->p + this->v * dt;
    this->v = this->v * damp;
}