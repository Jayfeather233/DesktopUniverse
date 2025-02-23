#include "JPL_util.hpp"
#include "camera3D.hpp"
#include "shader.hpp"
#include "object.hpp"
#include "TDBtime.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <eigen3/Eigen/Core>
#include <fmt/core.h>

#include <set>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"

int SCR_WIDTH = 1280, SCR_HEIGHT = 720;
std::shared_ptr<ogl::Program> shaderProgram_body, shaderProgram_traj, shaderProgram_flat, shaderProgram_text;
ogl::Camera3D cam3d;

sphere_object so;
trajectory tr;
overlayUI oUI;
FTtext ft;

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    cam3d.updateSCRratio(width, height);
    
    // shaderProgram_text->bind();
    // shaderProgram_text->setUniform("projection", glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT)));
    
    shaderProgram_flat->bind();
    // glm::mat4 m = glm::mat4(1.0f);
    // m[0][0] = 1.0f/SCR_WIDTH;
    // m[1][1] = 1.0f/SCR_HEIGHT;
    glm::mat4 m = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    shaderProgram_flat->setUniform("projection", m);
}
void processInput(GLFWwindow *window, float deltaTime);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

std::vector<celestial> bodies_csv;

static int center_id = -1;
static int focus_id = -1;
void setCenterBody(size_t i) {
    center_id = i;
    calculateTr(bodies_csv, bodies_csv[center_id]);
    fmt::print("traj calculated\n");
    tr.set_trajectories(bodies_csv);
    tr.setCenter(bodies_csv[center_id]);
    fmt::print("traj reset\n");
    fmt::print("center {}\n", bodies_csv[center_id].name);
}

void setFocusBody(size_t i) {
    focus_id = i;
    if(bodies_csv[focus_id].radiusX > 0) cam3d.distance = bodies_csv[focus_id].radiusX * 5;
    fmt::print("focus {}\n", bodies_csv[focus_id].name);
}

void render_gui(double traj_id)
{
    // Create a combo box to select celestial body
    if (ImGui::BeginCombo("Center Body", center_id == -1 ? "None" : bodies_csv[center_id].name.c_str()))
    {
        for (size_t i = 0; i < bodies_csv.size(); ++i)
        {
            bool isSelected = (i == center_id);
            if (ImGui::Selectable(bodies_csv[i].name.c_str(), isSelected))
            {
                if (center_id != i)
                {
                    setCenterBody(i);
                }
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Create a combo box to select celestial body
    if (ImGui::BeginCombo("Focus Body", focus_id == -1 ? "None" : bodies_csv[focus_id].name.c_str()))
    {
        for (size_t i = 0; i < bodies_csv.size(); ++i)
        {
            bool isSelected = (i == focus_id);
            if (ImGui::Selectable(bodies_csv[i].name.c_str(), isSelected))
            {
                if (focus_id != i)
                {
                    setFocusBody(i);
                }
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::Text("%s", focus_id == -1 ? "" : bodies_csv[focus_id].desc.c_str());

    if (focus_id != -1)
    {
        state bs = getBodyState(traj_id, bodies_csv[focus_id]);
        cam3d.setLookAt(glm::f64vec3(bs.x, bs.y, bs.z));
    }
}

bool test_curl()
{
    if (system("curl --version"))
    {
        fmt::print("No cURL found! please install it first.");
        return false;
    }
    return true;
}

double secsSinceStartOfMonth()
{
    int iy, im, id, ihmin;
    double fsec;
    getTDBWithUTC(&iy, &im, &id, &ihmin, &fsec);
    // fmt::print("JDTDB: {} {} {} {} {}\n", iy, im, id, ihmin, fsec);
    // getTDBWithERFA(&iy, &im, &id, &fd);
    // fmt::print("JDTDB: {} {} {} {}\n", iy, im, id, fd);
    return (id - 1) * 86400 + ihmin * 60 + fsec;
}

template <int x, int y, typename T>
void printMatrix(const glm::mat<x, y, T, glm::highp> &matrix)
{
    for (int i = 0; i < x; ++i)
    {
        for (int j = 0; j < y; ++j)
        {
            std::cout << std::setprecision(10) << matrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

template <int x, typename T>
void printVec(const glm::vec<x, T, glm::highp> &vec)
{
    for (int i = 0; i < x; ++i)
    {
        std::cout << vec[i] << " ";
    }
    std::cout << std::endl;
}
void render_all(std::vector<celestial> bodies_csv, double traj_id)
{
    shaderProgram_body->bind();
    cam3d.SetUniform(shaderProgram_body);
    for (const auto &body : bodies_csv)
    {
        if (body.radiusX > 0){
            shaderProgram_body->setUniform("model", getModelMat(body, traj_id));
            so.render(shaderProgram_body);
        }
    }
    shaderProgram_traj->bind();
    // glDepthFunc(GL_ALWAYS);
    cam3d.SetUniform(shaderProgram_traj);
    if (&tr.getCenter() != nullptr)
    {
        state st = getBodyState(traj_id, tr.getCenter());
        shaderProgram_traj->setUniform("offset", glm::dvec4(st.x, st.y, st.z, 0.0));
    }
    tr.setID(traj_id);
    tr.render(shaderProgram_traj);
    shaderProgram_flat->bind();
    oUI.update_position(bodies_csv, traj_id, cam3d, SCR_WIDTH, SCR_HEIGHT);
    oUI.render(shaderProgram_flat);
    shaderProgram_text->bind();
    oUI.setUniform(shaderProgram_text);
    oUI.render_text(SCR_WIDTH, SCR_HEIGHT);
    // glDepthFunc(GL_LESS);
}

int main()
{
    fmt::print("DesktopUniSim\n");
    if (!test_curl())
    {
        return 1;
    }

    for (size_t unn = 0; unn < 2; ++unn)
    {
        time_t now = time(NULL);
        struct tm utc_time = *gmtime(&now);
        utc_time.tm_mon += unn;
        now = std::mktime(&utc_time);
        utc_time = *gmtime(&now);
        Json::Value J = string_to_json(readfile("./data/track_bodies.json", "[]"));
        setQueryDayOffset(unn);
        for (auto obj : J)
        { // pre load
            get_csv(obj.asString(), &utc_time);
        }
        fmt::print("preload done.\n");
        Json::Value J2 = string_to_json(readfile("./data/meta.json"));
        for (Json::Value::ArrayIndex i = 0; i<J.size(); ++i)
        {
            std::string obj = J[i].asString();
            if (unn == 0) {
                if (J2["celestial"].isMember(obj))
                {
                    bodies_csv.push_back({obj, J2["celestial"][obj]["name"].asString(), J2["celestial"][obj]["desc"].asString(), J2["celestial"][obj]["GM"].asDouble(), J2["celestial"][obj]["radius"][0].asDouble(), J2["celestial"][obj]["radius"][1].asDouble(), J2["celestial"][obj]["radius"][2].asDouble(), get_csv(obj, &utc_time)});
                }
                else if (J2["comet"].isMember(obj))
                {
                    bodies_csv.push_back({obj, J2["comet"][obj]["name"].asString(), J2["comet"][obj]["desc"].asString(), 0, J2["comet"][obj]["radius"][0].asDouble(), J2["comet"][obj]["radius"][1].asDouble(), J2["comet"][obj]["radius"][2].asDouble(), get_csv(obj, &utc_time)});
                }
                else if (J2["barycenter"].isMember(obj))
                {
                    bodies_csv.push_back({obj, J2["barycenter"][obj]["name"].asString(), J2["barycenter"][obj]["desc"].asString(), 0, -1.0, -1.0, -1.0, get_csv(obj, &utc_time)});
                }
            } else {
                auto rr = get_csv(obj, &utc_time);
                bodies_csv[i].trajectory.insert(bodies_csv[i].trajectory.end(), rr.begin()+1, rr.end());
            }
        }
    }

    // for (auto &u : bodies_csv) {
    //     u.tr_trajectory.reserve(u.trajectory.size());
    // }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "DUS", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1); // Enable vsync

    cam3d.updateSCRratio(SCR_WIDTH, SCR_HEIGHT);

    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        throw std::runtime_error("Failed to initialize GLEW: {}" +
                                 std::string(reinterpret_cast<char const *>(glewGetErrorString(err))));
    }
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    shaderProgram_body = ogl::programFromFiles("./shaders", "simple_3D.vs", "simple.fs");
    shaderProgram_traj = ogl::programFromFiles("./shaders", "simple_traj.vs", "simple.fs");
    shaderProgram_flat = ogl::programFromFiles("./shaders", "simple_flat.vs", "simple.fs");
    shaderProgram_text = ogl::programFromFiles("./shaders", "simple_text.vs", "simple_text.fs");
    shaderProgram_text->bind();
    shaderProgram_text->setUniform("projection", glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f));
    glm::mat4 m = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    shaderProgram_flat->bind();
    shaderProgram_flat->setUniform("projection", m);
    fmt::print("OpenGL Initialized\n");
    so.genBuffer();
    oUI.genBuffer();
    ft.init();
    fmt::print("Buffer Generated.\n");

    double lastTime = glfwGetTime();
    const int targetFPS = 75;
    const double frameDuration = 1.0 / targetFPS;

    double ddd = 0;
    tr.setID(secsSinceStartOfMonth());
    for(size_t i = 0; i < bodies_csv.size(); ++i) {
        if (bodies_csv[i].id == "10") {
            setCenterBody(i);
            setFocusBody(i);
            break;
        }
    }

    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        // if (deltaTime < frameDuration) {
        //     std::this_thread::sleep_for(
        //         std::chrono::milliseconds(static_cast<int64_t>(1000 * (frameDuration - deltaTime))));
        // }
        fmt::print("delta time: {}\n", deltaTime);
        // ddd += 10;
        double traj_id = secsSinceStartOfMonth() + ddd;
        // fmt::print("secsSinceStartOfMonth: {}\n", traj_id);
        lastTime = currentTime;

        processInput(window, deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // render objects
        render_all(bodies_csv, traj_id);
        // shaderProgram->release();
        render_gui(traj_id);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
    {
        cam3d.ProcessMouseMovement(0.0f, 1000.0f * deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
    {
        cam3d.ProcessMouseMovement(0.0f, -1000.0f * deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
    {
        cam3d.ProcessMouseMovement(-1000.0f * deltaTime, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
    {
        cam3d.ProcessMouseMovement(1000.0f * deltaTime, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
    {
        cam3d.addDistance(1 + deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    {
        cam3d.addDistance(1 - deltaTime);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) { cam3d.ProcessMouseScroll(yoffset); }