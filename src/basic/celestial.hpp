#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

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

state getBodyState(double TDB, const celestial& body);
glm::f64mat4 getModelMat(const celestial &body, double TDB);

void calculateTr(std::vector<celestial> &all_bodies, const celestial &ref);

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