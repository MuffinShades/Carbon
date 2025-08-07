#include <iostream>
#include "../msutil.hpp"
#include "../gl/Camera.hpp"
#include "../vec.hpp"
#include "../gl/window.hpp"

class Player {
private:
    const f32 speed = 0.05f;
    const f32 sense = 0.5f;
    ControllableCamera cam = ControllableCamera();

    struct {
        u32 f = GLFW_KEY_W,
            b = GLFW_KEY_S,
            l = GLFW_KEY_A,
            r = GLFW_KEY_D;
    } controls;
    
    struct {
        bool f = false, b = false, l = false, r = false;
    } mov;

    bool kActive(Window *w, u32 k) {
        i32 state = glfwGetKey(w->wHandle, k);

        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }
public:
    Player(vec3 pos) {
        this->cam.setPos(pos, true);
    }

    ControllableCamera *getCam() {
        return &this->cam;
    }

    void mouseUpdate(f32 dmx, f32 dmy) {
        this->cam.changePitch(-dmy * sense);
        this->cam.changeYaw(dmx * sense);
    }

    void tick(Window *w) {
        mov.f = this->kActive(w, this->controls.f);
        mov.b = this->kActive(w, this->controls.b);
        mov.l = this->kActive(w, this->controls.l);
        mov.r = this->kActive(w, this->controls.r);

        //std::cout << "Mov: " << mov.f << " " << mov.b << " " << mov.l << " " << mov.r << std::endl;


        if (mov.f) this->cam.move(this->cam.getLookDirection() * speed, true);
        if (mov.b) this->cam.move(this->cam.getLookDirection() * -speed, true);
        if (mov.r) this->cam.move(vec3::CrossProd(this->cam.getLookDirection(), this->cam.getUp()) * speed, true);
        if (mov.l) this->cam.move(vec3::CrossProd(this->cam.getLookDirection(), this->cam.getUp()) * -speed, true);
    }
};