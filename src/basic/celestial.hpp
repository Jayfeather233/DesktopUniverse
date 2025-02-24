#pragma once

#include <string>
#include <vector>

#include "glm/ext.hpp"

#define MINUTE_INTERVAL 1 // 1440 for a day

struct state
{
    double JDTDB;
    double x, y, z;
    double vx, vy, vz;
};
state operator-(const state &a, const state &b);
state operator+(const state &a, const state &b);

struct celestial {
    std::string id;
    std::string name;
    std::string desc;
    double GM;
    double radiusX, radiusY, radiusZ;
    std::vector<state> trajectory;
    std::vector<state> tr_trajectory; // releted to reference frame
};

bool operator < (const celestial &a, const celestial &b);

state getBodyState(double secs, const celestial& body);
glm::f64mat4 getModelMat(const celestial &body, double traj_id);

void calculateTr(std::vector<celestial> &all_bodies, const celestial &ref);