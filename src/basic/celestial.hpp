#pragma once

#include <string>
#include <vector>

#define MINUTE_INTERVAL 1 // 1440 for a day

struct state
{
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

void calculateTr(std::vector<celestial> &all_bodies, const celestial &ref);