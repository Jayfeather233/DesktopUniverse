#pragma once

#include <glm/glm.hpp>

#include <GL/glew.h>

#include "shader.hpp"

namespace ogl {
// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific
// input methods
enum Camera3D_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for
// use in OpenGL
class Camera3D {
public:
    // camera Attributes
    glm::f64vec3 Position;
    glm::f64vec3 LookAt;

    // generated states
    glm::f64vec3 Front;
    glm::f64vec3 Up;
    glm::f64vec3 Right;
    glm::f64vec3 WorldUp;
    glm::f64mat4 viewMat;
    glm::f64mat4 projectMat;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    double scr_ratio;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    double distance;

    bool moved;

    // constructor with vectors
    Camera3D(glm::f64vec3 position = glm::f64vec3(0.0), glm::f64vec3 up = glm::f64vec3(0.0, 1.0, 0.0), float yaw = 0.0f,
             float pitch = 0.0f, glm::f64vec3 lkat = glm::f64vec3(0.0));

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::f64mat4 GetViewMatrix() const;

    glm::f64mat4 GetProjectionMat() const;

    void SetUniform(std::shared_ptr<ogl::Program> p);

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset);
    void addDistance(float u);
    void setLookAt(glm::f64vec3 lkat);
    void updateSCRratio(uint32_t SCR_WIDTH, uint32_t SCR_HEIGHT);

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();
};
} // namespace ogl