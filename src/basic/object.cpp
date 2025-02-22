#include "object.hpp"

#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <eigen3/Eigen/Core>

#include <fmt/core.h>

sphere_object::sphere_object(float radius, unsigned int sectorCount, unsigned int stackCount)
    : radius(radius), sectorCount(sectorCount), stackCount(stackCount)
{
}

void sphere_object::genBuffer()
{
    // Generate the vertices and indices for the sphere mesh
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    // Generate sphere vertices and indices
    for (unsigned int i = 0; i <= stackCount; ++i)
    {
        float stackAngle = M_PI / 2 - i * M_PI / stackCount; // Stack angle
        float radiusXY = radius * cosf(stackAngle);          // Radius at this stack
        float y = radius * sinf(stackAngle);                 // Y component

        for (unsigned int j = 0; j <= sectorCount; ++j)
        {
            float sectorAngle = j * 2 * M_PI / sectorCount; // Sector angle
            float x = radiusXY * cosf(sectorAngle);
            float z = radiusXY * sinf(sectorAngle);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            // Optional: You can add normals if needed for lighting, but I'll keep it simple here
            // Color data (using a simple color gradient based on position)
            float red = (x/radius + 1.0f) / 2.0f;   // Color gradient based on x position
            float green = (y/radius + 1.0f) / 2.0f; // Color gradient based on y position
            float blue = (z/radius + 1.0f) / 2.0f;  // Color gradient based on z position
            vertices.push_back(red);
            vertices.push_back(green);
            vertices.push_back(blue);
        }
    }

    // Generate indices for the sphere faces
    for (unsigned int i = 0; i < stackCount; ++i)
    {
        unsigned int k1 = i * (sectorCount + 1); // Current stack
        unsigned int k2 = k1 + sectorCount + 1;  // Next stack

        for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (stackCount - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    // Generate buffers for storing vertices and indices
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Unbind VAO
    glBindVertexArray(0);

    indicesCount = indices.size();
}

void sphere_object::render()
{
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

trajectory::trajectory() : vao(0), vbo(0), colorVbo(0), center(nullptr) {}

// Called to set or update the trajectories
void trajectory::set_trajectories(const std::vector<celestial>& bodies)
{
    // Clear old trajectory data
    trajectoryVertices.clear();

    // Process each celestial body and generate its trajectory
    for (const auto& body : bodies) {
        for (size_t i = 0; i < body.tr_trajectory.size() - 10; i+=10) { // t min
            // fmt::print("{}\n", i);
            const auto& start = body.tr_trajectory[i];
            const auto& end = body.tr_trajectory[i + 10];

            // Create vertex positions for the trajectory lines
            trajectoryVertices.push_back(start.x);
            trajectoryVertices.push_back(start.y);
            trajectoryVertices.push_back(start.z);

            trajectoryVertices.push_back(end.x);
            trajectoryVertices.push_back(end.y);
            trajectoryVertices.push_back(end.z);
    
            trajectoryColors.push_back(1.0f);
            trajectoryColors.push_back(0.0f);
            trajectoryColors.push_back(0.0f);
    
            trajectoryColors.push_back(1.0f);
            trajectoryColors.push_back(1.0f);
            trajectoryColors.push_back(1.0f);
        }
    }

    fmt::print("gen buffer\n");
    // Re-generate buffers every time trajectories are updated
    genBuffer();
}

// Generate OpenGL buffer objects
void trajectory::genBuffer()
{
    // Clean up old buffers if needed
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &colorVbo);
    }

    // Create new VAO and VBOs
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &colorVbo);

    glBindVertexArray(vao);

    // Create the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, trajectoryVertices.size() * sizeof(GLdouble), trajectoryVertices.data(), GL_STATIC_DRAW);
    glVertexAttribLPointer(0, 3, GL_DOUBLE, sizeof(GLdouble)*3, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
    glBufferData(GL_ARRAY_BUFFER, trajectoryColors.size() * sizeof(GLfloat), trajectoryColors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*3, (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // Unbind VAO
    GLenum er = glGetError();
    if (er != 0) {
        fmt::print("gen err: {}\n", er);
    }
}

// Render the trajectory lines
void trajectory::render()
{
    glLineWidth(2.0f);

    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, trajectoryVertices.size());

    GLenum er = glGetError();
    if (er != 0) {
        fmt::print("err: {}\n", er);
    }
}

void trajectory::setCenter(const celestial &c) {
    center = &c;
}

const celestial& trajectory::getCenter() {
    return *center;
}

void trajectory::setID(double traj_id){
    id = traj_id;
}