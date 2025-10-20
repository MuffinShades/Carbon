#include <iostream>
#include "../mat.hpp"
#include "../vec.hpp"
#include "../msutil.hpp"
#include "../math/quaternion.hpp"
#include "../gl/mesh.hpp"
#include "../gl/Camera.hpp"
#include "../gl/graphics.hpp"
#include "Body.hpp"

enum class rb_simple_type {
    cuboid,
    sphere,
    cone,
    non_simple
};

class RigidBody3 {
private:
    //TODO: change to shared pointer
    Mesh *mesh = nullptr;

    /*
    
    m_mat -> model matrix
    r_mat -> body rotaion matrix
    s_mat -> body scaling matrix
    central_trans_mat -> translation for rotation about the object's origin
    
    */
    mat4 m_mat = mat4(1), r_mat = mat4(1), s_mat = mat4(1), central_trans_mat = mat4();
    graphicsState gs;

    bool made_gs = false;

    //br is bounding radius of a sphere that surrounds the post-scaled mesh
    //imass value of 0 will lock the object's position
    f64 mass = 0.0f, imass = 0.0f, volume, br = 0.0f, density;

    vec3 p = {0,0,0}, v = {0,0,0}, a = {0,0,0}, av = {0,0,0}, aa = {0,0,0}, obj_center, center;
    Quat4 rot;
    vec3 torque = vec3(0,0,0), force = vec3(0,0,0);
    mat4 o_iI, o_I;
    mat4 iI = mat4(1);

    void adjustITensor();
public:
    f32 x, y, z;

    f32 boundingRadius();

    RigidBody3() {};
    RigidBody3(Mesh *m, f32 density, Material material);
    RigidBody3(enum class rb_simple_type s_ty, vec3 dim, f32 density, Material material);

    void makeGraphicsState(graphics *g);
    graphicsState *getGraphicsState(graphics *g);
    void tick(f32 dt);
    mat4 getMat();
    bool inView(Camera* cam);
    void applyImpulse(Force J);
    void addForce(Force F);
    void addSimpleForce(vec3 f);
    void applySimpleImpulse(vec3 J);

    friend void rigid_pre_compute(RigidBody3 *rb, enum class rb_simple_type s_ty, void *simple_properties);
    friend class RBodyScene3;
    friend struct _pproj proj_body_on_normal(RigidBody3 *rb, vec3 n);
};

class RBodyScene3 {
private:
    vec3 g = {0,0,0};
    graphicsState sgs;
    std::vector<RigidBody3*> objs;

    vec3 *checkNormalBuffer = nullptr;
    size_t checkNormalBufferSize = 0;

    void collisionChecks();
    void collisionCheckStep2(RigidBody3* rb1, RigidBody3* rb2);
    void collisionResolve(RigidBody3* rb1, RigidBody3* rb2, vec3 c_pos, vec3 c_norm);
public:
    RBodyScene3() {}
    void setGravity(f32 g);
    void addBody(RigidBody3 *rb);
    void tick(f32 dt);
    void render(graphics* g, Camera *cam);
};