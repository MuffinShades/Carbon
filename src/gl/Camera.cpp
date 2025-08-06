#include "Camera.hpp"

void Camera::computeDirections() {
    const vec3 globalUp = {0.0f, 1.0f, 0.0f};

    this->lookDir = vec3::Normalize(this->target - this->pos);
    this->right = vec3::CrossProd(globalUp, this->lookDir);
    this->up = vec3::CrossProd(this->lookDir, this->right);
    this->lookMatrix = mat4::LookAt(this->right, this->up, this->lookDir);
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

    if (this->pitch > mu_pi)
        this->pitch = mu_pi;

    if (this->pitch < -mu_pi)
        this->pitch = -mu_pi;

    this->vUpdate();
}

void ControllableCamera::changeRoll(f32 roll) {
    this->roll += roll;
    this->vUpdate();
}

void ControllableCamera::vUpdate() {
    const f32 pc = cosf(this->pitch);
    this->target = this->pos + vec3(
        cosf(this->yaw) * pc,
        sin(this->pitch),
        sinf(this->yaw) * pc
    );
    this->computeDirections();
}