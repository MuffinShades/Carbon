#include "Camera.hpp"

void Camera::computeDirections() {
    const vec3 globalUp = {0.0f, 1.0f, 0.0f};

    this->lookDir = vec3::Normalize(this->target - this->pos);

    this->right = vec3::Normalize(vec3::CrossProd(this->lookDir, globalUp));
    this->up = vec3::Normalize(vec3::CrossProd(this->right, this->lookDir));

    this->lookMatrix = mat4::LookAt(
        this->pos, 
        this->pos + this->lookDir, 
        this->up
    );
}

void Camera::setPos(vec3 p, bool changeTarget) {
    if (changeTarget) {
        const vec3 dp = this->pos - p;
        this->target = this->target + dp;
    }

    this->pos = p;
    this->computeDirections();
}

void Camera::move(vec3 dis, bool changeTarget) {
    if (changeTarget) {
        this->target = this->target + dis;
    }

    this->pos = this->pos + dis;
    this->computeDirections();
}

mat4 Camera::getLookMatrix() const {
    return this->lookMatrix;
}

vec3 Camera::getPos() const {
    return this->pos;
}

vec3 Camera::getTarget() const {
    return this->target;
}

vec3 Camera::getLookDirection() const {
    return this->lookDir;
}

void Camera::setTarget(vec3 target) {
    this->target = target;
    this->computeDirections();
}

Camera::Camera(vec3 pos) {
    this->setPos(pos, true);
}

Camera::Camera(vec3 pos, vec3 target) {
    this->pos = pos;
    this->setTarget(target);
}

vec3 ControllableCamera::getRotation() const {
    return vec3(this->pitch, this->yaw, this->roll);
}

f32 ControllableCamera::getYaw() const {
    return this->yaw;
}

f32 ControllableCamera::getPitch() const {
    return this->pitch;
}

f32 ControllableCamera::getRoll() const {
    return this->roll;
}

void ControllableCamera::setYaw(f32 yaw) {
    this->yaw = yaw;
    this->vUpdate();
}

void ControllableCamera::setPitch(f32 pitch) {
    this->pitch = pitch;
    this->vUpdate();
}

void ControllableCamera::setRoll(f32 roll) {
    this->roll = roll;
    this->vUpdate();
}

void ControllableCamera::changeYaw(f32 dyaw) {
    this->yaw += dyaw;
    this->vUpdate();
}

void ControllableCamera::changePitch(f32 pitch) {
    this->pitch += pitch;

    if (this->pitch > 89.9f)
        this->pitch = 89.9f;

    if (this->pitch < -89.9f)
        this->pitch = -89.9f;

    this->vUpdate();
}

void ControllableCamera::changeRoll(f32 roll) {
    this->roll += roll;
    this->vUpdate();
}

void ControllableCamera::vUpdate() {
    f64 yr = mu_rad(this->yaw);
    f64 pr = mu_rad(this->pitch);

    const f32 pc = cosf(pr);

    this->target = this->pos + vec3::Normalize({
        cosf(yr) * pc,
        sinf(pr),
        sinf(yr) * pc
    });

    this->computeDirections();
}