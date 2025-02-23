#include "celestial.hpp"

#include <fmt/core.h>

bool operator<(const celestial &a, const celestial &b)
{
    return a.id < b.id;
}

state operator-(const state &a, const state &b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z, a.vx - b.vx, a.vy - b.vy, a.vz - b.vz};
}
state operator+(const state &a, const state &b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z, a.vx + b.vx, a.vy + b.vy, a.vz + b.vz};
}

state getBodyState(double secs, const celestial &body)
{
    const auto &trajectory = body.trajectory;

    if (trajectory.empty())
    {
        return {0, 0, 0, 0, 0, 0};
    }

    // Check if the requested time is before the first trajectory point or after the last one
    if (secs <= 0)
    {
        return trajectory.front();
    }
    if (secs >= trajectory.size() * 60)
    {
        return trajectory.back();
    }

    // Find the two points surrounding the time `secs`
    size_t i = static_cast<size_t>((secs + 30) / 60);

    // The two points for interpolation
    state start = trajectory[i - 1];
    state end = trajectory[i];

    // Interpolate between start and end
    double t = (secs - i * 60) / 60; // Time fraction between start and end

    state interpolated;
    interpolated.x = start.x + t * (end.x - start.x);
    interpolated.y = start.y + t * (end.y - start.y);
    interpolated.z = start.z + t * (end.z - start.z);

    interpolated.vx = start.vx + t * (end.vx - start.vx);
    interpolated.vy = start.vy + t * (end.vy - start.vy);
    interpolated.vz = start.vz + t * (end.vz - start.vz);

    return interpolated;
}

glm::f64mat4 getModelMat(const celestial &body, double traj_id){
    glm::f64mat4 m = glm::f64mat4(1.0); // Identity matrix
    state bodystate = getBodyState(traj_id, body);
    m = glm::translate(m, glm::f64vec3(bodystate.x, bodystate.y, bodystate.z));
    return glm::scale(m, glm::f64vec3(body.radiusX, body.radiusY, body.radiusZ));
}

void calculateTr(std::vector<celestial> &all_bodies, const celestial &ref)
{
    for (auto &u : all_bodies)
    {
        u.tr_trajectory.clear();
        for (size_t i = 0; i < ref.trajectory.size(); ++i)
        {
            u.tr_trajectory.push_back(u.trajectory[i] - ref.trajectory[i]);
        }
        fmt::print("size: {}\n", u.tr_trajectory.size());
    }
}