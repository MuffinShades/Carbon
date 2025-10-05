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

void RigidBody3::adjustITensor() {
    this->iI = this->o_iI * this->r_mat;
}

void RigidBody3::tick(f32 dt) {
    //angular calculations
    this->adjustITensor();

    this->aa = this->aa + this->iI * this->torque;
    this->av = this->av + this->av * dt;

    vec3 i_av = this->av * dt * 0.5f;

    this->rot = this->rot + Quat4(i_av.x, i_av.y, i_av.z, 0.0f) * this->rot;
    this->rot.normalize();

    this->r_mat = this->rot.toRotMatrix();

    //linear calculations
    this->v = this->v + this->a * dt;

    constexpr f32 damp_fac = 0.02f;
    const f32 damp = powf(damp_fac, dt);
    
    this->p = this->p + this->v * dt;
    this->v = this->v * damp; 

    this->x = this->p.x;
    this->y = this->p.y;
    this->z = this->p.z;
}

bool RigidBody3::inView(Camera* cam) {
    if (!cam) return false;


}

f32 RigidBody3::boundingRadius() {
    return this->br;
}

void collisionCheckStep2(RigidBody3 rb1, RigidBody3 rb2) {
    //sat
    bool c = false;

    //TODO: collision resolve
}
    
void collisionResolve(RigidBody3 rb1, RigidBody3 rb2, vec3 c_norm) {
    
}

void RBodyScene3::collisionChecks() {
    size_t i,j;

    const size_t nObjs = this->objs.size();

    RigidBody3 rb1, rb2;

    f32 dx,dy,dz,r_dis;

    for (i = 0; i < nObjs; ++i) {
        rb1 = this->objs[i];
        for (j = i + 1; j < nObjs; ++j) {
            rb2 = this->objs[j];

            dx = rb2.x - rb1.x;
            dy = rb2.y - rb1.y;
            dz = rb2.z - rb1.z;

            r_dis = rb2.boundingRadius() + rb1.boundingRadius();   
            r_dis *= r_dis;

            //check bounding radius
            if (dx*dx + dy*dy + dz*dz >= r_dis)
                continue;
                
            //do sat
            collisionCheckStep2(rb1, rb2);
        }
    }
}

void RBodyScene3::addBody(RigidBody3 rb) {

}

void RBodyScene3::tick(f32 dt) {
    for (auto b : this->objs)
        b.tick(dt);

    this->collisionChecks();
}

void RBodyScene3::render(graphics* g, Camera *cam) {
    if (!cam || !g)
        return;
}