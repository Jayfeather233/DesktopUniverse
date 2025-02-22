#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <fmt/core.h>
#include <jsoncpp/json/json.h>
#include "celestial.hpp"

#include "JPL_util.hpp"

Json::Value string_to_json(const std::string &str)
{
    Json::Value root;
    Json::Reader re;
    bool isok = re.parse(str, root);
    if (!isok)
    {
        ; // TODO
    }
    return root;
}

std::string to_filename(const std::string &filename)
{
    std::string uu = filename;
    static char invalid_char[] = {"/<>:\"\\|?*"};
    for (char u : invalid_char)
    {
        std::replace(uu.begin(), uu.end(), u, '_');
    }
    return uu;
}

std::vector<state> istream2state(std::istream &is, bool read_header = true)
{
    std::string line;
    if (read_header)
        std::getline(is, line);
    std::vector<state> ret;

    while (std::getline(is, line))
    {
        std::stringstream ss(line);
        std::string token;
        state s;

        // Skip JDTDB column (first column), we don't need it
        std::getline(ss, token, ','); // JDTDB

        // Skip Calendar Date (TDB) column (second column), we don't need it
        std::getline(ss, token, ','); // Calendar Date (TDB)

        // Read X, Y, Z, VX, VY, VZ, in unit km
        std::getline(ss, token, ',');
        s.x = std::stod(token) * 1000;
        std::getline(ss, token, ',');
        s.z = -std::stod(token) * 1000;
        std::getline(ss, token, ',');
        s.y = std::stod(token) * 1000;
        std::getline(ss, token, ',');
        s.vx = std::stod(token) * 1000;
        std::getline(ss, token, ',');
        s.vz = -std::stod(token) * 1000;
        std::getline(ss, token, ',');
        s.vy = std::stod(token) * 1000;

        // Add the state to the vector
        ret.push_back(s);
    }
    return ret;
}

std::vector<state> get_csv_from_JPL(const std::string &COMMAND, const std::string &filename)
{
    std::string desc, csv, name;
    double GM = 0.0;
    bool is_big;
    Eigen::Vector3d radius(1, 1, 1);
    {
        std::string content = download_file_JPL(COMMAND);
        is_big = decode_JPL_result(content, name, desc, csv, GM, radius);
    }
    Json::Value J = string_to_json(readfile("./data/meta.json"));
    if (is_big)
    {
        J["celestial"][COMMAND]["name"] = name;
        J["celestial"][COMMAND]["desc"] = desc;
        J["celestial"][COMMAND]["GM"] = GM;
        for (const auto &u : radius)
            J["celestial"][COMMAND]["radius"].append(u);
    }
    else
    {
        J["comet"][COMMAND]["name"] = name;
        J["comet"][COMMAND]["desc"] = desc;
        for (const auto &u : radius)
            J["comet"][COMMAND]["radius"].append(u);
    }
    writefile("./data/meta.json", J.toStyledString());
    writefile(filename, csv);
    std::istringstream iss(csv);
    return istream2state(iss);
}

std::vector<state> get_csv_from_file(const std::string &COMMAND, const std::string &filename_txt, const std::string &filename_csv)
{
    std::string desc, csv, name;
    double GM = 0.0;
    bool is_big;
    Eigen::Vector3d radius(1, 1, 1);
    {
        std::ifstream ifs(filename_txt);
        std::ostringstream oss;
        oss << ifs.rdbuf();
        std::string content = oss.str();
        is_big = decode_JPL_result(content, name, desc, csv, GM, radius);
    }
    Json::Value J = string_to_json(readfile("./data/meta.json"));
    if (is_big && !J["celestial"].isMember(COMMAND))
    {
        J["celestial"][COMMAND]["name"] = name;
        J["celestial"][COMMAND]["desc"] = desc;
        J["celestial"][COMMAND]["GM"] = GM;
        for (const auto &u : radius)
            J["celestial"][COMMAND]["radius"].append(u);
    }
    else if (!J["comet"].isMember(COMMAND))
    {
        J["comet"][COMMAND]["name"] = name;
        J["comet"][COMMAND]["desc"] = desc;
        for (const auto &u : radius)
            J["comet"][COMMAND]["radius"].append(u);
    }
    writefile("./data/meta.json", J.toStyledString());
    writefile(filename_csv, csv);
    std::istringstream iss(csv);
    return istream2state(iss);
}

std::vector<state> get_csv(const std::string &COMMAND, struct tm *utc_time)
{
    // time_t now = time(NULL);
    // struct tm *utc_time = gmtime(&now);
    utc_time->tm_sec += 69;
    std::mktime(utc_time);

    int tmy = utc_time->tm_year + 1900;
    int tmm = utc_time->tm_mon + 1;

    std::string filename = fmt::format("./data/{}_{}/{}.csv", tmy, tmm, to_filename(COMMAND));
    try
    {
        std::fstream f = openfile(filename, std::ios_base::in);
        return istream2state(f);
    }
    catch (...)
    {
        fmt::print("Missing csv for {}, ", COMMAND);
        if (fs::exists(fmt::format("./data/{}_{}/{}.txt", tmy, tmm, to_filename(COMMAND))))
        {
            fmt::print("found data file, restoring...\n");
            return get_csv_from_file(COMMAND, fmt::format("./data/{}_{}/{}.txt", tmy, tmm, to_filename(COMMAND)), filename);
        }
        else
        {
            fmt::print("downloading...\n");
            std::vector<state> ret = get_csv_from_JPL(COMMAND, filename);
            fs::rename("./results.txt", fmt::format("./data/{}_{}/{}.txt", tmy, tmm, to_filename(COMMAND)));
            return ret;
        }
    }
}