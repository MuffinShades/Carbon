#include <iostream>
#include "../vec.hpp"
#include "../msutil.hpp"
#include "../mat.hpp"

class Camera {
protected:
    mat4 lookMatrix;
    vec3 pos = {0.0f, 0.0f, 0.0f};
    vec3 target = {0.0f, 0.0f, 0.0f};
    vec3 lookDir = {0.0f, 0.0f, 0.0f};
    vec3 up, right;

    void computeDirections();
public:
    mat4 getLookMatrix() const;
    vec3 getPos() const;
    vec3 getTarget() const;
    vec3 getLookDirection() const;
    /*
    
    changeTarget -> if true target will update to be in front of the camera the same way it was before
                 -> if false the camera will rotate to still focus on target
    
    */
    void setPos(vec3 p, bool changeTarget = false);
    void move(vec3 dis, bool changeTarget = false);
    void setTarget(vec3 target);

    Camera(){}
    Camera(vec3 pos);
    Camera(vec3 pos, vec3 target);
};

class ControllableCamera : Camera {
protected:
    f32 yaw = -mu_pi, pitch = 0.0f, roll = 0.0f;

    void vUpdate();
public:
    using Camera::getLookMatrix;
    using Camera::getPos;

    vec3 getRotation() const;

    f32 getYaw() const;
    f32 getPitch() const;
    f32 getRoll() const;

    void setYaw(f32 yaw);
    void setPitch(f32 pitch);
    void setRoll(f32 roll);

    void changeYaw(f32 dyaw);
    void changePitch(f32 pitch);
    void changeRoll(f32 roll);

    ControllableCamera(){}
};