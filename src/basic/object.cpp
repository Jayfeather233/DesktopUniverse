#include "object.hpp"

#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <eigen3/Eigen/Core>
#include <fmt/core.h>

#include <ft2build.h>
#include FT_FREETYPE_H

template<glm::length_t L, typename T, glm::qualifier Q>
T calculateAngle(const glm::vec<L, T, Q>& vec1, const glm::vec<L, T, Q>& vec2) {
    // Normalize the vectors
    glm::vec<L, T, Q> normVec1 = glm::normalize(vec1);
    glm::vec<L, T, Q> normVec2 = glm::normalize(vec2);

    // Calculate the dot product
    T dotProduct = glm::dot(normVec1, normVec2);

    // Clamp the value between -1 and 1 to avoid floating-point precision errors
    dotProduct = glm::clamp(dotProduct, static_cast<T>(-1.0), static_cast<T>(1.0));

    // Calculate the angle in radians
    T angle = acos(dotProduct);

    // Optionally, convert to degrees
    // float angleInDegrees = glm::degrees(angle);
    return angle; // in radians
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

void object::setUniform(std::shared_ptr<ogl::Program> s) {}

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
            float red = (x / radius + 1.0f) / 2.0f;   // Color gradient based on x position
            float green = (y / radius + 1.0f) / 2.0f; // Color gradient based on y position
            float blue = (z / radius + 1.0f) / 2.0f;  // Color gradient based on z position
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

void sphere_object::render(std::shared_ptr<ogl::Program> s)
{
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

trajectory::trajectory() : vao(0), vbo(0), colorVbo(0), center(nullptr), id(0.0) {}

// Called to set or update the trajectories
void trajectory::set_trajectories(const std::vector<celestial> &bodies)
{
    // Clear old trajectory data
    trajectoryVertices.clear();

    // Process each celestial body and generate its trajectory
    for (const auto &body : bodies)
    {
        if (body.radiusX > 0.0)
        {
            size_t las = (id/60);
            for (size_t i = las; i < body.tr_trajectory.size(); ++i)
            { // t min
                // fmt::print("{}\n", i);
                const auto &start = body.tr_trajectory[las];
                const auto &end = body.tr_trajectory[i];
                
                const double two_pi = glm::pi<double>() * 2.0;
                if (calculateAngle(glm::dvec3(start.vx, start.vy, start.vz), glm::dvec3(end.vx, end.vy, end.vz)) < two_pi/40 /*&& i - las < 300*/)
                continue;

                las = i;

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
            if (las == static_cast<size_t>(id/60)) {
                const auto &start = body.tr_trajectory[las];
                const auto &end = *(body.tr_trajectory.end()-1);
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
    }

    fmt::print("gen buffer: {}\n", trajectoryVertices.size()/3);
    // Re-generate buffers every time trajectories are updated
    genBuffer();
}

// Generate OpenGL buffer objects
void trajectory::genBuffer()
{
    // Clean up old buffers if needed
    if (vao != 0)
    {
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
    glVertexAttribLPointer(0, 3, GL_DOUBLE, sizeof(GLdouble) * 3, (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
    glBufferData(GL_ARRAY_BUFFER, trajectoryColors.size() * sizeof(GLfloat), trajectoryColors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (void *)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // Unbind VAO
    GLenum er = glGetError();
    if (er != 0)
    {
        fmt::print("gen err: {}\n", er);
    }
}

// Render the trajectory lines
void trajectory::render(std::shared_ptr<ogl::Program> s)
{
    glLineWidth(2.0f);

    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, trajectoryVertices.size());

    GLenum er = glGetError();
    if (er != 0)
    {
        fmt::print("err: {}\n", er);
    }
}

void trajectory::setCenter(const celestial &c)
{
    center = &c;
}

const celestial &trajectory::getCenter()
{
    return *center;
}

void trajectory::setID(double traj_id)
{
    id = traj_id;
}

void overlayUI::genBuffer()
{
    numSegments = 10;
    float radius = 1.0f;

    std::vector<GLfloat> circleVertices;
    for (int i = 0; i < numSegments; i++)
    {
        float theta = 2.0f * 3.1415926f * float(i) / float(numSegments);
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        circleVertices.push_back(x);
        circleVertices.push_back(y);
        circleVertices.push_back(0.001f); // Assuming 2D circle, z = 0

        circleVertices.push_back(0.9f);
        circleVertices.push_back(0.9f);
        circleVertices.push_back(0.9f); // color
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    // Create the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(GLfloat), circleVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (void *)(sizeof(GLfloat) * 3));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // Unbind VAO
    GLenum er = glGetError();
    if (er != 0)
    {
        fmt::print("gen err: {}\n", er);
    }
}
void overlayUI::setUniform(std::shared_ptr<ogl::Program> s)
{
    s->setUniform("textColor", glm::vec3(0.9f, 0.9f, 0.9f));
}
void overlayUI::render(std::shared_ptr<ogl::Program> s)
{
    glBindVertexArray(vao);
    for (const auto &label : labels)
    {
        if (label.is_show)
        {
            // fmt::print("label: {}\n", label.label);
            auto m = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(label.x, label.y, 0.0f)), glm::vec3(8, 8, 1));
            s->setUniform("model", m);
            glDrawArrays(GL_LINE_LOOP, 0, numSegments);
        }
    }
    glBindVertexArray(0);
}
void overlayUI::render_text(int width, int height)
{
    for (const auto &label : labels)
    {
        if (label.is_show)
        {
            // fmt::print("render: {} {}\n", ((label.x+32)*2.0)/width-1.0, ((label.y-8)*2.0)/height-1.0);
            ft.RenderText(label.label, ((label.x+16)*2.0)/width-1.0, ((label.y-4)*2.0)/height-1.0, 1.0f/width, 1.0f/height);
        }
    }
}
void overlayUI::update_position(const std::vector<celestial> &bodies, double traj_id, const ogl::Camera3D &cam, int width, int height)
{
    labels.clear();
    for (const auto &body : bodies)
    {
        glm::f64vec4 pos = cam.GetProjectionMat() * cam.GetViewMatrix() * getModelMat(body, traj_id) * glm::f64vec4(0.0, 0.0, 0.0, 1.0);
        if (pos.z < 0)
        {
            continue;
        }
        pos = pos / pos.z;
        // fmt::print("name: {}\npos: {} {} {} {}\n", body.name, pos.x, pos.y, pos.z, pos.w);
        labels.push_back({static_cast<GLint>((pos.x+1)/2*width), static_cast<GLint>((pos.y+1)/2*height), body.GM, body.name, true});
        // fmt::print("pos_pb: {} {}\n", static_cast<GLint>((pos.x+1)/2*width + 32), static_cast<GLint>((pos.y+1)/2*height - 8));
    }
    update_labels(width, height);
}

void overlayUI::update_labels(int width, int height)
{
    for (size_t i = 0; i < labels.size(); ++i)
    {
        for (size_t j = i + 1; j < labels.size(); ++j)
        {
            GLint diffX = abs(labels[i].x - labels[j].x), diffY = abs(labels[i].y - labels[j].y);
            if (diffY < 30 && diffX <= 100)
            {
                if (labels[i].mass < labels[j].mass)
                {
                    labels[i].is_show = false;
                }
                else
                {
                    labels[j].is_show = false;
                }
            }
        }
    }
}

void FTtext::init()
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        fmt::print("ERROR::FREETYPE: Could not init FreeType Library\n");
    }

    FT_Face face;
    if (FT_New_Face(ft, "fonts/inter_light.ttf", 0, &face))
    {
        fmt::print("ERROR::FREETYPE: Failed to load font\n");
    }
    FT_Set_Pixel_Sizes(face, 0, 32);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    for (unsigned char c = 0; c < 128; c++)
    {
        // load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer);
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x};
        Characters.insert(std::pair<char, Character>(c, character));
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
void FTtext::RenderText(std::string text, float x, float y, float scaleX, float scaleY)
{
    // activate corresponding render state
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scaleX;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scaleY;

        float w = ch.Size.x * scaleX;
        float h = ch.Size.y * scaleY;
        // update VBO for each character
        float vertices[6][4] = {
            {xpos, ypos + h, 0.0f, 0.0f},
            {xpos, ypos, 0.0f, 1.0f},
            {xpos + w, ypos, 1.0f, 1.0f},

            {xpos, ypos + h, 0.0f, 0.0f},
            {xpos + w, ypos, 1.0f, 1.0f},
            {xpos + w, ypos + h, 1.0f, 0.0f}};
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scaleX; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}