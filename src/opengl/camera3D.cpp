#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GL/glew.h>

#include "camera3D.hpp"
#include "shader.hpp"
#include "celestial.hpp"

template<typename T>
GLM_FUNC_QUALIFIER glm::mat<4, 4, T, glm::defaultp> my_inf_perspective(T fovy, T aspect, T zNear)
{
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    glm::mat<4, 4, T, glm::defaultp> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = - static_cast<T>(1);
    Result[2][3] = - static_cast<T>(1);
    Result[3][2] = - (static_cast<T>(2) * zNear);
    return Result;
}

static const float SPEED = 2.5f;
static const float SENSITIVITY = 0.1f;
static const float ZOOM = 45.0f;
// constructor with vectors
ogl::Camera3D::Camera3D(glm::f64vec3 position, glm::f64vec3 up, float yaw, float pitch, glm::f64vec3 lkat)
    : Front(glm::f64vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM), LookAt(lkat)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    distance = 100;
    updateCameraVectors();
}

// returns the view matrix calculated using Euler Angles and the LookAt Matrix
glm::f64mat4 ogl::Camera3D::GetViewMatrix() const { return viewMat; }

glm::f64mat4 ogl::Camera3D::GetProjectionMat() const
{
    // return glm::perspective(glm::radians(Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 1000.0f); // std::numeric_limits<float>::infinity()
    return projectMat; // std::numeric_limits<float>::infinity()
}

void ogl::Camera3D::SetUniform(std::shared_ptr<ogl::Program> p)
{
    p->setUniform("view3D", GetViewMatrix());
    p->setUniform("projection", GetProjectionMat());
}

// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void ogl::Camera3D::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch) {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void ogl::Camera3D::ProcessMouseScroll(float yoffset)
{
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 60.0f)
        Zoom = 60.0f;
    updateCameraVectors();
}
void ogl::Camera3D::addDistance(float u)
{
    distance *= u;
    updateCameraVectors();
}
void ogl::Camera3D::setLookAt(glm::f64vec3 lkat) {
    LookAt = lkat;
    updateCameraVectors();
}
void ogl::Camera3D::updateSCRratio(uint32_t SCR_WIDTH, uint32_t SCR_HEIGHT){
    scr_ratio = (float)SCR_WIDTH / (float)SCR_HEIGHT;
    updateCameraVectors();
}

// calculates the front vector from the Camera's (updated) Euler Angles
void ogl::Camera3D::updateCameraVectors()
{
    // calculate the new Front vector
    glm::f64vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    Right =
        glm::normalize(glm::cross(Front, WorldUp)); // normalize the vectors, because their length gets closer to 0 the
                                                    // more you look up or down which results in slower movement.
    Up = glm::normalize(glm::cross(Right, Front));

    Position = Front * (double)(-distance) + LookAt;
    viewMat = glm::lookAt(Position, LookAt, Up);
    // projectMat = glm::perspective(glm::radians((double)Zoom), scr_ratio, 0.01, 1e8);
    projectMat = my_inf_perspective(glm::radians((double)Zoom), scr_ratio, 0.01);
    moved = true;
    // glm::vec4 Pos2 = GetViewMatrix() * glm::vec4(Front, 1.0);
    // printf("3D: %.2f %.2f %.2f\n", Position.x, Position.y, Position.z);
    // printf("    %.2f %.2f %.2f\n", Pos2.x, Pos2.y, Pos2.z);
}