
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <eigen3/Eigen/Core>

#include "celestial.hpp"
#include "glm/glm.hpp"

class object
{
public:
    virtual void genBuffer() = 0;
    virtual void render() = 0;
};

class sphere_object : public object
{
public:
    sphere_object(float radius = 1.0f, unsigned int sectorCount = 36, unsigned int stackCount = 18);
    virtual void genBuffer() override;
    virtual void render() override;

private:
    float radius;
    unsigned int sectorCount, stackCount;
    GLuint vao, vbo, ebo;
    unsigned int indicesCount;
};

class trajectory : public object {
public:
    trajectory();
    void set_trajectories(const std::vector<celestial>& bodies);
    virtual void genBuffer();
    virtual void render();
    void setCenter(const celestial &center);
    const celestial & getCenter();
    void setID(double traj_id);

private:
    GLuint vao, vbo, colorVbo;
    std::vector<GLdouble> trajectoryVertices; // Store the trajectory vertices
    std::vector<GLuint> trajectoryIndices; // Store the trajectory vertices
    std::vector<GLfloat> trajectoryColors; // Store color per segment

    celestial const *center;
    double id;
};