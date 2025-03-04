#include "render.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "TDBtime.hpp"
#include "JPL_util.hpp"

static renderer *callback_r = nullptr;

static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    callback_r->setWindowSize(width, height);
}
static void processInput(GLFWwindow *window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
    {
        callback_r->getCam3d().ProcessMouseMovement(0.0f, 1000.0f * deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
    {
        callback_r->getCam3d().ProcessMouseMovement(0.0f, -1000.0f * deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
    {
        callback_r->getCam3d().ProcessMouseMovement(-1000.0f * deltaTime, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
    {
        callback_r->getCam3d().ProcessMouseMovement(1000.0f * deltaTime, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
    {
        callback_r->getCam3d().addDistance(1 + deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    {
        callback_r->getCam3d().addDistance(1 - deltaTime);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) { callback_r->getCam3d().ProcessMouseScroll(yoffset); }
static void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    static double lastX = 0, lastY = 0;
    static bool firstMouse = true;
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    fmt::print("xoffset: {}, yoffset: {}\n", xoffset, yoffset);
    fmt::print("xpos: {}, ypos: {}\n", xpos, ypos);
    callback_r->getCam3d().ProcessMouseMovement(xoffset, yoffset);
}

ogl::Camera3D &renderer::getCam3d() { return cam3d; }
void renderer::setWindowSize(int width, int height)
{
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    cam3d.updateSCRratio(width, height);

    shaderProgram_flat->bind();
    glm::mat4 m = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    shaderProgram_flat->setUniform("projection", m);
}

renderer::renderer() {
    callback_r = this;
    centerID = -1;
    focusID = -1;

    int st = -3, en = 0;
    
    for (int unn = st; unn <= en; ++unn)
    {
        fmt::print("start preload\n");
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
            if (unn == st) {
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
        fmt::print("One month loaded.\n");
    }

    for(auto& body : bodies_csv) {
        for(auto& u:body.trajectory) {
            u.JDTDB *= 86400;
        }
    }
    fmt::print("CSV Read\n");
    // initOGL();
    // fmt::print("Renderer Initialized\n");
}

void renderer::initOGL() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "DUS", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // glfwSetCursorPosCallback(window, mouse_callback);
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
    // glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LESS);
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
    shaderProgram_text->bind();
    ft.init();
    fmt::print("Buffer Generated.\n");
}

renderer::~renderer(){
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void renderer::renderGUI(double TDB) {
    // Create a combo box to select celestial body
    if (ImGui::BeginCombo("Center Body", centerID == -1 ? "None" : bodies_csv[centerID].name.c_str()))
    {
        for (size_t i = 0; i < bodies_csv.size(); ++i)
        {
            bool isSelected = (i == centerID);
            if (ImGui::Selectable(bodies_csv[i].name.c_str(), isSelected))
            {
                if (centerID != i)
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
    if (ImGui::BeginCombo("Focus Body", focusID == -1 ? "None" : bodies_csv[focusID].name.c_str()))
    {
        for (size_t i = 0; i < bodies_csv.size(); ++i)
        {
            bool isSelected = (i == focusID);
            if (ImGui::Selectable(bodies_csv[i].name.c_str(), isSelected))
            {
                if (focusID != i)
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
    ImGui::Text("%s", focusID == -1 ? "" : bodies_csv[focusID].desc.c_str());

    if (focusID != -1)
    {
        state bs = getBodyState(TDB, bodies_csv[focusID]);
        cam3d.setLookAt(glm::f64vec3(bs.x, bs.y, bs.z));
    }
}

void renderer::render3D(double TDB){
    shaderProgram_body->bind();
    cam3d.SetUniform(shaderProgram_body);
    for (const auto &body : bodies_csv)
    {
        if (body.radiusX > 0){
            shaderProgram_body->setUniform("model", getModelMat(body, TDB));
            so.render(shaderProgram_body);

            // glm::f64vec4 pr = cam3d.GetProjectionMat() * cam3d.GetViewMatrix() * getModelMat(body, TDB) * glm::f64vec4(0.0, 0.0, 0.0, 1.0);
            // double depth = log(float(pr.z))/100;
            // pr = glm::f64vec4(pr.x/pr.z, pr.y/pr.z, depth, 1.0);
            // fmt::print("body: {} ", body.name);
            // printVec(pr);
        }
    }

    shaderProgram_traj->bind();
    cam3d.SetUniform(shaderProgram_traj);
    if (&tr.getCenter() != nullptr)
    {
        state st = getBodyState(TDB, tr.getCenter());
        shaderProgram_traj->setUniform("offset", glm::dvec4(st.x, st.y, st.z, 0.0));
    }
    tr.setID(TDB);
    tr.render(shaderProgram_traj);

    shaderProgram_flat->bind();
    oUI.update_position(bodies_csv, TDB, cam3d, SCR_WIDTH, SCR_HEIGHT);
    oUI.render(shaderProgram_flat);
    shaderProgram_text->bind();
    oUI.setUniform(shaderProgram_text);
    oUI.render_text(ft, SCR_WIDTH, SCR_HEIGHT);
    shaderProgram_text->release();
}

void renderer::renderLoop() {
    double lastTime = glfwGetTime();
    const int targetFPS = 75;
    const double frameDuration = 1.0 / targetFPS;

    // double ddd = 0;
    tr.setID(getTDBsec());
    for(size_t i = 0; i < bodies_csv.size(); ++i) {
        if (bodies_csv[i].id == "10") {
            setCenterBody(i);
            setFocusBody(i);
            break;
        }
    }
    size_t ddd = 0;
    double tdelta = 0;

    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        tdelta += deltaTime;
        // if (deltaTime < frameDuration) {
        //     std::this_thread::sleep_for(
        //         std::chrono::milliseconds(static_cast<int64_t>(1000 * (frameDuration - deltaTime))));
        // }
        if (ddd == 10) {
            fmt::print("fps: {}\n", 10.0/tdelta);
            ddd = 0;
            tdelta = 0;
        } else {
            ++ddd;
        }
        double TDB = getTDBsec();
        lastTime = currentTime;

        processInput(window, deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // fmt::print("before render all\n");
        // render objects
        render3D(TDB);
        // shaderProgram->release();
        // fmt::print("before render GUI\n");
        renderGUI(TDB);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void renderer::setCenterBody(size_t i) {
    centerID = i;
    calculateTr(bodies_csv, bodies_csv[centerID]);
    fmt::print("traj calculated\n");
    tr.set_trajectories(bodies_csv);
    tr.setCenter(bodies_csv[centerID]);
    fmt::print("traj reset\n");
    fmt::print("center {}\n", bodies_csv[centerID].name);
}

void renderer::setFocusBody(size_t i) {
    focusID = i;
    if(bodies_csv[focusID].radiusX > 2.0) cam3d.distance = bodies_csv[focusID].radiusX * 5;
    fmt::print("focus {}\n", bodies_csv[focusID].name);
}
