#include "RigidBody3.hpp"
#include "../gl/geometry/cube.hpp"

struct rb_simple_prop_cuboid {
    vec3 dim;
};

mat4 computeCuboidIT(vec3 dim, f32 m) {
    const f32 S = (1.0f / 12.0f) * m,
              x2 = dim.x * dim.x,
              y2 = dim.y * dim.y,
              z2 = dim.z * dim.z;

    f32 i_dat[16] = {
        S * (z2 + y2), 0.0f, 0.0f, 0.0f,
        0.0f, S * (x2 + z2), 0.0f, 0.0f,
        0.0f, 0.0f, S * (x2 + y2), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    return mat4(i_dat);
}

f32 computeCuboidVolume(vec3 dim) {
    return dim.x * dim.y * dim.z;
}

void rigid_pre_compute(RigidBody3 *rb, enum class rb_simple_type s_ty, void *simple_properties) {
    if (!rb || !rb->mesh)
        return;

    Mesh *msh = rb->mesh;

    const size_t nv = msh->size();
    const auto *m_data = msh->data();
    Vertex v;
    size_t i;

    vec3 center = {0,0,0};

    //compute center of mass
    for (i = 0; i < nv; i++) {
        v = m_data[i];

        center = center + vec3(v.posf[0], v.posf[1], v.posf[2]);
    }

    if (nv > 0)
        center = center * (1.0f / (f32)nv);

    rb->obj_center = center;
    rb->central_trans_mat = mat4::CreateTranslationMatrix((center) * -1.0f);

    //compute volume, mass, and I tensor
    switch (s_ty) {
    case rb_simple_type::cuboid: {
        rb_simple_prop_cuboid *prop = (rb_simple_prop_cuboid*) simple_properties;

        if (!prop)
            break;

        rb->volume = computeCuboidVolume(prop->dim);
        rb->mass = rb->density * rb->volume;
        rb->o_I = computeCuboidIT(prop->dim, rb->mass);

        break;
    }
    default: {
        std::cout << "Not implemeneted yet ;-;";
        rb->mass = INFINITY;
    }
    }

    //simple inverse stuff
    //inverse mass
    constexpr f32 mass_epsilon = 0.000001f;

    if (rb->mass > 0) {
        rb->imass = 1.0f / rb->mass;

        if (rb->imass < mass_epsilon)
            rb->imass = 0.0f;
    } else
        rb->imass = INFINITY;

    //inverse inertia tensor
    f32 iit_dat[16];
    f32 iv;

    for (i = 0; i < 16; i++) {
        iv = rb->o_I.m[i];

        if (iv != 0)
            iit_dat[i] = 1.0f / iv;
        else
            iit_dat[i] = 0.0f;
    }

    rb->o_iI = mat4(iit_dat);
}

void RigidBody3::addForce(Force F) {
    this->a = this->a + F.F * this->imass;
    this->center = this->obj_center + this->p;
    this->torque = this->torque + vec3::CrossProd(this->center - F.pos, F.F);

    //
    std::cout << "--Force Added--" << std::endl;
    std::cout << "a: " << this->a.x << " " << this->a.y << " " << this->a.z << std::endl;
    std::cout << "c: " << this->center.x << " " << this->center.y << " " << this->center.z << std::endl;
    std::cout << "t: " << this->torque.x << " " << this->torque.y << " " << this->torque.z << std::endl;
    std::cout << "IMASS: " << this->imass << " " << this->mass << " " << this->volume << " " << this->density << std::endl;
}

void RigidBody3::addSimpleForce(vec3 F) {
    this->a = this->a + F * this->imass;
}

void RigidBody3::applySimpleImpulse(vec3 J) {
    this->v = this->v + J;
}

void applyImpulse(Force J) {

}

void RigidBody3::adjustITensor() {
    this->iI = this->o_iI * this->r_mat;
}

void RigidBody3::tick(f32 dt) {
    //angular calculations
    this->adjustITensor();

    this->aa = this->aa + this->iI * this->torque;
    this->av = this->av + this->aa * dt;

    vec3 i_av = this->av * dt * 0.5f;

    this->torque = vec3(0,0,0);

    //std::cout << "Intertia Tensor: " << this->iI[0][0] << std::endl;

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

    this->m_mat = this->r_mat * this->central_trans_mat * this->s_mat;
}

bool RigidBody3::inView(Camera* cam) {
    if (!cam) return false;


}

f32 RigidBody3::boundingRadius() {
    return this->br;
}

graphicsState *RigidBody3::getGraphicsState(graphics *g) {
    if (!this->made_gs)
        this->makeGraphicsState(g);

    return &this->gs;
}

void RigidBody3::makeGraphicsState(graphics *g) {
    if (!g || !this->mesh) return;

    g->useGraphicsState(&gs);
    g->iniStaticGraphicsState();
    g->bindMeshToVbo(this->mesh);
    g->useDefaultGraphicsState();

    this->made_gs = true;
}

mat4 RigidBody3::getMat() {
    return this->m_mat;
}

RigidBody3::RigidBody3(rb_simple_type s_ty, vec3 dim, f32 density, Material material) {
    if (s_ty == rb_simple_type::non_simple)
        return;

    switch (s_ty) {
    case rb_simple_type::cuboid: {
        this->density = density;

        rb_simple_prop_cuboid prop = {
            .dim = dim
        };

        if (!this->mesh)
            this->mesh = new Mesh;

        this->mesh->setMeshData(Geo::Cube::GetBaseCube(), nCubeVerts);

        rigid_pre_compute(this, s_ty, &prop);
        break;
    }
    default:
        return;
    }
}

//resolve collision between 2 bodies
void RBodyScene3::collisionResolve(RigidBody3* rb1, RigidBody3* rb2, vec3 c_norm) {
    
}

/*

Projects a 3d point onto a 3d normal which returns the value
which can then be used as the min / max for the sat

*/
f32 proj_v2n(vec3 vert, vec3 normal) {
    return vec3::DotProd(vert, normal);
}

void RBodyScene3::collisionCheckStep2(RigidBody3 *rb1, RigidBody3 *rb2) {
    //sat
    bool c = false;

    std::vector<vec3> check_normals;


    vec3 lp_axis;
    f32 lp_mag = INFINITY;

    for (vec3 n : check_normals) {
        
    }

    collisionResolve(rb1, rb2, lp_axis);
}

void RBodyScene3::collisionChecks() {
    size_t i,j;

    const size_t nObjs = this->objs.size();

    RigidBody3 *rb1, *rb2;

    f32 dx,dy,dz,r_dis;

    for (i = 0; i < nObjs; ++i) {
        rb1 = this->objs[i];
        for (j = i + 1; j < nObjs; ++j) {
            rb2 = this->objs[j];

            dx = rb2->x - rb1->x;
            dy = rb2->y - rb1->y;
            dz = rb2->z - rb1->z;

            r_dis = rb2->boundingRadius() + rb1->boundingRadius();   
            r_dis *= r_dis;

            //check bounding radius
            if (dx*dx + dy*dy + dz*dz >= r_dis)
                continue;
                
            //do sat
            collisionCheckStep2(rb1, rb2);
        }
    }
}

void RBodyScene3::addBody(RigidBody3 *rb) {
    this->objs.push_back(rb);
}

void RBodyScene3::tick(f32 dt) {
    for (auto b : this->objs) {
        b->applySimpleImpulse(g);
        b->tick(dt);
    }

    //this->collisionChecks();
}

void RBodyScene3::render(graphics* g, Camera *cam) {
    if (!cam || !g)
        return;

    graphicsState *gs;
    Shader *s = g->getCurrentShader();
    mat4 mm;

    for (auto b : this->objs) {
        gs = b->getGraphicsState(g);
        mm = b->getMat();
        g->useGraphicsState(gs);
        s->SetMat4("model_mat", &mm);
        g->render_no_geo_update();
    }
}

void RBodyScene3::setGravity(f32 g) {
    this->g = vec3(0,g,0);
}