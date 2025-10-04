#include <iostream>
#include "../mat.hpp"
#include "../vec.hpp"
#include "../msutil.hpp"
#include "../math/quaternion.hpp"
#include "../gl/mesh.hpp"
#include "../gl/Camera.hpp"
#include "../gl/graphics.hpp"
#include "Body.hpp"

class RigidBody3 {
private:
    mat4 m_mat, r_mat;
    graphicsState gs;

    //br is bounding radius of a sphere that surrounds the post-scaled mesh
    //imass value of 0 will lock the object's position
    f32 mass, imass = 0.0f, volume, br = 0.0f;

    vec3 p, v, a, av, aa, obj_center, center;
    Quat4 rot;
    vec3 torque;
    mat4 o_iI, o_i;
    mat4 iI;

    void adjustITensor();
public:
    f32 x, y, z;

    f32 boundingRadius();

    RigidBody3() {};
    RigidBody3(Mesh *m, f32 density, Material material);

    graphicsState makeGraphicsState();
    void tick(f32 dt);
    mat4 getMat();
    bool inView(Camera* cam);
    void applyImpulse(Force F);
    void addForce(Force F);
};

class RBodyScene3 {
private:
    vec3 g;
    graphicsState sgs;
    std::vector<RigidBody3> objs;

    void collisionChecks();
    void collisionCheckStep2(RigidBody3 rb1, RigidBody3 rb2);
    void collisionResolve(RigidBody3 rb1, RigidBody3 rb2, vec3 c_norm);
public:
    RBodyScene3() {}
    void addBody(RigidBody3 rb);
    void tick(f32 dt);
    void render(graphics* g, Camera *cam);
};