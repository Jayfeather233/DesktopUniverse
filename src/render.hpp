#pragma once

#include "camera3D.hpp"
#include "shader.hpp"
#include "object.hpp"

class renderer {
public:
    renderer();
    void initOGL();
    ~renderer();
    void renderLoop();

    void setWindowSize(int width, int height);
    
    void setCenterBody(size_t i);
    void setFocusBody(size_t i);

    ogl::Camera3D &getCam3d();
private:
    int SCR_WIDTH = 1280, SCR_HEIGHT = 720;
    GLFWwindow *window;
    std::shared_ptr<ogl::Program> shaderProgram_body, shaderProgram_traj, shaderProgram_flat, shaderProgram_text;
    ogl::Camera3D cam3d;
    
    sphere_object so;
    trajectory tr;
    overlayUI oUI;
    FTtext ft;
    std::vector<celestial> bodies_csv;

    int focusID, centerID;

    void renderGUI(double TDB);
    void render3D(double TDB);
};