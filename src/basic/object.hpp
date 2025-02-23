
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <eigen3/Eigen/Core>
#include <map>
#include <utility>

#include "celestial.hpp"
#include "glm/glm.hpp"
#include "imgui.h"

#include "shader.hpp"
#include "camera3D.hpp"

class object
{
public:
    virtual void genBuffer() = 0;
    virtual void setUniform(std::shared_ptr<ogl::Program> s);
    virtual void render(std::shared_ptr<ogl::Program> s) = 0;
};

class sphere_object : public object
{
public:
    sphere_object(float radius = 1.0f, unsigned int sectorCount = 36, unsigned int stackCount = 18);
    virtual void genBuffer() override;
    virtual void render(std::shared_ptr<ogl::Program> s) override;

private:
    float radius;
    unsigned int sectorCount, stackCount;
    GLuint vao, vbo, ebo;
    unsigned int indicesCount;
};

class trajectory : public object
{
public:
    trajectory();
    void set_trajectories(const std::vector<celestial> &bodies);
    virtual void genBuffer() override;
    virtual void render(std::shared_ptr<ogl::Program> s) override;
    void setCenter(const celestial &center);
    const celestial &getCenter();
    void setID(double traj_id);

private:
    GLuint vao, vbo, colorVbo;
    std::vector<GLdouble> trajectoryVertices; // Store the trajectory vertices
    std::vector<GLuint> trajectoryIndices;    // Store the trajectory vertices
    std::vector<GLfloat> trajectoryColors;    // Store color per segment

    celestial const *center;
    double id;
};

class overlayUI : public object
{
public:
    virtual void genBuffer() override;
    virtual void setUniform(std::shared_ptr<ogl::Program> s) override;
    virtual void render(std::shared_ptr<ogl::Program> s) override;
    void render_text(int width, int height);
    void update_position(const std::vector<celestial> &bodies, double traj_id, const ogl::Camera3D &cam, int width, int height);

private:
    GLuint vao, vbo;
    struct label {
        GLint x, y;
        double mass;
        std::string label;
        bool is_show;
    };
    std::vector<label> labels;
    int numSegments;

    void update_labels(int width, int height);
};

class FTtext {
public:
    void init();
    void RenderText(std::string text, float x, float y, float scaleX, float scaleY);

private:
    struct Character {
        unsigned int TextureID;  // ID handle of the glyph texture
        glm::ivec2   Size;       // Size of glyph
        glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
        unsigned int Advance;    // Offset to advance to next glyph
    };
    
    std::map<char, Character> Characters;
    GLuint vao, vbo;
};

extern FTtext ft;