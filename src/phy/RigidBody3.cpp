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
    rb->br = 2;

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
    this->force = this->force + F.F;
    this->torque = this->torque + vec3::CrossProd(F.pos - this->center, F.F);

    //
    std::cout << "--Force Added--" << std::endl;
    std::cout << "f: " << this->force.x << " " << this->force.y << " " << this->force.z << std::endl;
    std::cout << "t: " << this->torque.x << " " << this->torque.y << " " << this->torque.z << std::endl;
    std::cout << "IMASS: " << this->imass << " " << this->mass << " " << this->volume << " " << this->density << std::endl;
}

void RigidBody3::addSimpleForce(vec3 F) {
    this->force = this->force + F;
}

void RigidBody3::applySimpleImpulse(vec3 J) {
    this->v = this->v + J * this->imass;
}

void RigidBody3::applyImpulse(Force J) {
    this->applySimpleImpulse(J.F);
    const vec3 relPos = J.pos - this->center;
    this->av = this->av + this->iI * vec3::CrossProd(relPos, J.F);
}

void RigidBody3::adjustITensor() {
    this->iI = this->o_iI * this->r_mat;
}

void RigidBody3::tick(f32 dt) {
    constexpr f32 damp_fac = 0.02f;
    const f32 damp = powf(damp_fac, dt);

    //angular calculations
    this->adjustITensor();

    this->aa = this->iI * this->torque;
    this->av = this->av + this->aa * dt;

    vec3 i_av = this->av * dt * 0.5f;

    this->rot = this->rot + Quat4(i_av.x, i_av.y, i_av.z, 0.0f) * this->rot;
    this->rot.normalize();

    this->r_mat = this->rot.toRotMatrix();

    //linear calculations
    this->a = this->force * this->imass;
    this->v = this->v + (this->a * dt);
    this->p = this->p + (this->v * dt);

    //position adjustments
    this->center = this->obj_center + this->p;

    this->x = this->p.x;
    this->y = this->p.y;
    this->z = this->p.z;

    //zero out all forces and torques and apply velocity damping
    this->torque = vec3(0,0,0);
    this->force = vec3(0,0,0);

    this->v = this->v * damp; 
    this->av = this->av * damp;

    //create final model matrix
    this->m_mat = mat4::CreateTranslationMatrix(this->p) * this->r_mat * this->central_trans_mat * this->s_mat;
}

bool RigidBody3::inView(Camera* cam) {
    if (!cam) return false;

    return true;
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

void RigidBody3::setPos(vec3 posf) {
    this->p = posf;
}

//resolve collision between 2 bodies
void RBodyScene3::collisionResolve(RigidBody3* rb1, RigidBody3* rb2, vec3 c_pos, vec3 c_norm) {
    //TODO: calculute the impulse vector thing
    f32 J = 0.0f;
    

    //apply impulses
    vec3 Fj = c_norm * J;

    rb1->applyImpulse({
        .pos = c_pos,
        .F = c_norm * J
    });

    rb2->applyImpulse({
        .pos = c_pos,
        .F = c_norm * -J
    });
}

/*

Projects a 3d point onto a 3d normal which returns the value
which can then be used as the min / max for the sat

*/
f32 proj_v2n(vec3 vert, vec3 normal) {
    return vec3::DotProd(vert, normal);
}

f32 proj_v2n_arr(const f32 *v, vec3 normal) {
    return v[0] * normal.x + v[1] * normal.y + v[2] * normal.z;
}

struct _pproj {
    bool err = false;
    f32 min_dot = INFINITY, max_dot = -INFINITY;
    vec3 min, max;
};

const vec3 global_up = vec3(0.0f,1.0f,0.0f);

_pproj proj_body_on_normal(RigidBody3 *rb, vec3 n) {
    if (!rb)
        return {.err=true};

    //goofy thing
    const vec3 a = n, b = vec3::CrossProd(n, global_up), c = vec3::CrossProd(a,b);
    const f32 n_dis_dat[16] = {
        a.x, b.x, c.x, 0.0f,
        a.y, b.y, c.y, 0.0f,
        a.z, b.z, c.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    mat4 rb_mat = rb->getMat();
    mat4 n_dis_proj = mat4((f32*) n_dis_dat);

    //mesh
    Mesh *msh = rb->mesh;

    if (!msh)
        return {.err=true};

    const size_t sz = msh->size();
    const Vertex *verts = msh->data();

    size_t i;
    f32 min_dis = INFINITY, max_dis = -INFINITY, sqr_dis;
    _pproj r;
    vec3 dis_vec, pos_vec;
    f32 *_posf;

    vec3 min_vec, max_vec;

    for (i = 0; i < sz; ++i) {
        _posf = (f32*) verts[i].posf;

        //set the pos vec
        pos_vec.x = _posf[0];
        pos_vec.y = _posf[1];
        pos_vec.z = _posf[2];

        //matrix transforms
        pos_vec = rb_mat * pos_vec;
        dis_vec = n_dis_proj * pos_vec;

        //get distance from axis / plane
        sqr_dis = dis_vec.lenSqr();

        if (sqr_dis < min_dis) {
            min_dis = sqr_dis;
            r.min_dot = vec3::DotProd(pos_vec, n);
            r.min = pos_vec;
        }
        
        if (sqr_dis > max_dis) {
            max_dis = sqr_dis;
            r.max_dot = vec3::DotProd(pos_vec, n);
            r.max = pos_vec;
        }
    }

    return r;
}

constexpr size_t DEF_COLLISION_NORMAL_BUF_SZ = 0xfff;

void RBodyScene3::collisionCheckStep2(RigidBody3 *rb1, RigidBody3 *rb2) {
    //sat
    bool c = false;

    //buffer checks and stuff
    const size_t rb1_n_faces = rb1->mesh->getNTriangleFaces(), 
                 rb2_n_faces = rb2->mesh->getNTriangleFaces(),
                 total_n_faces = rb1_n_faces + rb2_n_faces;

    if (!this->checkNormalBuffer) {
        this->checkNormalBufferSize = DEF_COLLISION_NORMAL_BUF_SZ;
        this->checkNormalBuffer = new vec3[this->checkNormalBufferSize];
    }

    if (this->checkNormalBufferSize < total_n_faces) {
        vec3 *o_buffer = this->checkNormalBuffer;
        this->checkNormalBuffer = new vec3[total_n_faces];
        in_memcpy(this->checkNormalBuffer, o_buffer, this->checkNormalBufferSize * sizeof(vec3));
        delete[] o_buffer; //delete the old unused buffer
    }

    //copy over normals
    vec3 *n_inter_buf;

    n_inter_buf = const_cast<vec3*>(rb1->mesh->getStoredTriangleBasedNormals());
    in_memcpy(this->checkNormalBuffer              , n_inter_buf, rb1_n_faces * sizeof(vec3));

    n_inter_buf = const_cast<vec3*>(rb2->mesh->getStoredTriangleBasedNormals());
    in_memcpy(this->checkNormalBuffer + rb1_n_faces, n_inter_buf, rb2_n_faces * sizeof(vec3));

    //TODO: add the cross product between edges!!!
    //TODO: add a function in mesh to compute triangle normals

    vec3 lp_axis, lp_pos, n;
    f32 lp_mag = -INFINITY;

    size_t i;

    for (i = 0; i < total_n_faces; ++i) {
        n = this->checkNormalBuffer[i];
        
        //TODO: when projecting first get  the min/max poins on the axis and then project them to get the range!!!
        _pproj r1 = proj_body_on_normal(rb1, n),
               r2 = proj_body_on_normal(rb2, n);

        //std::cout << r1.min_dot << " " << r1.max_dot << " " << r2.min_dot << " " << r2.max_dot << std::endl;

        if (
            r1.min_dot <= r2.min_dot && r1.max_dot >= r2.min_dot
        ) {
            const f32 p = r2.min_dot - r1.max_dot; //penetration
            //axis is normal

            if (p > lp_mag) {
                lp_axis = n;
                lp_mag = p;
                lp_pos = r1.max + lp_axis * lp_mag;
            }
        } else if (
            r2.min_dot <= r1.min_dot && r2.max_dot >= r1.min_dot
        ) {
            const f32 p = r1.min_dot - r2.max_dot; //penetration
            //axis is -normal
            
            if (p > lp_mag) {
                lp_axis = n * -1.0f;
                lp_mag = p;
                lp_pos = r1.min + lp_axis * lp_mag;
            }
        } else
            return; //no collision occured
    }

    collisionResolve(rb1, rb2, lp_pos, lp_axis * lp_mag);
}

void RBodyScene3::collisionChecks() {
    size_t i,j;

    const size_t nObjs = this->objs.size();

    RigidBody3 *rb1, *rb2;

    f32 dx,dy,dz,r_dis,a_dis;

    for (i = 0; i < nObjs; ++i) {
        rb1 = this->objs[i];
        for (j = i + 1; j < nObjs; ++j) {
            rb2 = this->objs[j];

            dx = rb2->x - rb1->x;
            dy = rb2->y - rb1->y;
            dz = rb2->z - rb1->z;

            a_dis = dx*dx + dy*dy + dz*dz;

            r_dis = rb2->boundingRadius() + rb1->boundingRadius();   
            r_dis *= r_dis;

            //check bounding radius
            if (a_dis >= r_dis)
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

    this->collisionChecks();
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