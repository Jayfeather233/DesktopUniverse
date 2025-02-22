#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include "celestial.hpp"
#include <jsoncpp/json/json.h>
#include <eigen3/Eigen/Core>
namespace fs = std::filesystem;
std::string download_file_JPL(const std::string &COMMAND);
bool decode_JPL_result(const std::string &content, std::string &name, std::string &desc, std::string &csv, double &GM, Eigen::Vector3d &radius);
std::string readfile(const fs::path &file_path, const std::string &default_content = "");
void writefile(const fs::path file_path, const std::string &content, bool is_append = false);
std::fstream openfile(const fs::path file_path, const std::ios_base::openmode mode);
std::vector<state> get_csv(const std::string &COMMAND, struct tm *utc_time);
Json::Value string_to_json(const std::string &str);