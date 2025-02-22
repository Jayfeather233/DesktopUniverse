#include "JPL_util.hpp"
#include "TDBtime.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <fmt/core.h>
#include <regex>
#include <eigen3/Eigen/Core>
#include <chrono>

static const double G = 6.67430e-11; // 6.67430 (± 0.00015) x 10-11 kg-1 m3 s-2	2018 CODATA recommended values

static const std::string TEMPLATE = "\
!$$SOF\n\
MAKE_EPHEM=YES\n\
COMMAND='{}'\n\
EPHEM_TYPE=VECTORS\n\
CENTER='500@0'\n\
START_TIME='{}'\n\
STOP_TIME='{}'\n\
STEP_SIZE='1 MINUTES'\n\
VEC_TABLE='2'\n\
REF_SYSTEM='ICRF'\n\
REF_PLANE='ECLIPTIC'\n\
VEC_CORR='NONE'\n\
CAL_TYPE='G'\n\
OUT_UNITS='KM-S'\n\
VEC_LABELS='YES'\n\
VEC_DELTA_T='NO'\n\
CSV_FORMAT='YES'\n\
OBJ_DATA='YES'\n\
!$$EOF\
";

std::string download_file_JPL(const std::string &COMMAND)
{
    auto tt = get_query_days();
    std::string file_content = fmt::format(TEMPLATE, COMMAND, tt.first, tt.second);
    std::ofstream ofs("input_file.txt");
    ofs << file_content;
    ofs.flush();
    ofs.close();
    if (system("curl -s -F format=text -F input=@input_file.txt https://ssd.jpl.nasa.gov/api/horizons_file.api > results.txt"))
    {
        fmt::print("Error when downloading file for {}, quiting...", COMMAND);
        exit(0);
    }
    std::ifstream ifs("results.txt");
    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string u(oss.str());
    return u;
}

bool decode_JPL_result(const std::string &content, std::string &name, std::string &desc, std::string &csv, double &GM, Eigen::Vector3d &radius)
{
    bool is_big = false;
    std::istringstream iss(content);
    std::ostringstream oss_desc, oss_csv;
    std::string tmp;

    bool is_csv = false, is_desc = true;

    while (!iss.eof())
    {
        std::getline(iss, tmp);
        if (tmp == "$$SOE")
        {
            is_csv = true;
            is_desc = false;
            oss_csv << "JDTDB, Calendar Date (TDB), X, Y, Z, VX, VY, VZ, " << std::endl;
        }
        else if (tmp == "$$EOE")
        {
            break;
        }
        else if (is_csv)
        {
            oss_csv << tmp << std::endl;
        }
        else if (tmp.find("Calendar Date (TDB)") == tmp.npos && is_desc)
        {
            oss_desc << tmp << std::endl;
            size_t u, v, v2;
            if ((u = tmp.find("Target body name:")) != tmp.npos)
            {
                v = v2 = tmp.find(")", u);
                while (v2 != tmp.npos)
                {
                    v = v2;
                    v2 = tmp.find(")", v2 + 1);
                }
                name = tmp.substr(u, v - u + 1).substr(18);
            }
            std::smatch match;

            std::regex re(R"(GM\s*\(km\^3/s\^2\)\s*=\s*([+-]?\d*\.\d+|\d+))");
            if (std::regex_search(tmp, match, re))
            {
                double a = std::stod(match[1]);
                GM = a * 1e9;
                is_big = true;
            }
            std::regex re2(R"(GM,\s*km\^3/s\^2\s*=\s*([+-]?\d*\.\d+|\d+))");
            if (!is_big && std::regex_search(tmp, match, re2))
            {
                double a = std::stod(match[1]);
                GM = a * 1e9;
                is_big = true;
            }
            std::regex re2_2(R"(GM\s*\(planet\)\s*km\^3/s\^2\s*=\s*([+-]?\d*\.\d+|\d+))");
            if (!is_big && std::regex_search(tmp, match, re2_2))
            {
                double a = std::stod(match[1]);
                GM = a * 1e9;
                is_big = true;
            }
            std::regex re2_1(R"(GM\s*=\s*([+-]?\d*\.\d+|\d+))");
            if (!is_big && std::regex_search(tmp, match, re2_1))
            {
                double a = std::stod(match[1]);
                GM = a * 1e9;
                is_big = true;
            }

            std::regex re3(R"(Mass\s*\(10\^([+-]?\d+)\s*kg\s*\)\s*=\s*([+-]?\d*\.\d+|\d+)\s*\(10\^([+-]?\d+)\))");
            if (!is_big && std::regex_search(tmp, match, re3))
            {
                int exponent_mass = std::stoi(match[1]);
                double mass_value = std::stod(match[2]);
                int exponent_secondary = std::stoi(match[3]);
                GM = mass_value * std::pow(10, exponent_mass + exponent_secondary) * G;
                is_big = true;
            }

            std::regex re4(R"(Mean\s*radius\s*\(km\)\s*=\s*([+-]?\d*\.\d+|\d+)(\s*\+\-\s*([+-]?\d*\.\d+|\d+))?)");
            if (std::regex_search(tmp, match, re4))
            {
                // Extract the main value (radius)
                double rd = std::stod(match[1])*1e3;
                radius = Eigen::Vector3d(rd, rd, rd);
            }

            std::regex re5(R"(Vol\.\s*mean\s*radius,\s*km\s*=\s*([+-]?\d*\.\d+|\d+)(\s*\+\-\s*([+-]?\d*\.\d+|\d+))?)");
            if (std::regex_search(tmp, match, re5))
            {
                // Extract the value (radius)
                double rd = std::stod(match[1])*1e3;
                radius = Eigen::Vector3d(rd, rd, rd);
            }
            std::regex re5_1(R"(Vol\.\s*mean\s*radius\s*\(km\)\s*=\s*([+-]?\d*\.\d+|\d+)(\s*\+\-\s*([+-]?\d*\.\d+|\d+))?)");
            if (std::regex_search(tmp, match, re5_1))
            {
                // Extract the value (radius)
                double rd = std::stod(match[1])*1e3;
                radius = Eigen::Vector3d(rd, rd, rd);
            }
            
            std::regex re6(R"(Vol\.\s*Mean\s*Radius\s*\(km\)\s*=\s*([+-]?\d*\.\d+|\d+)\s*\+\-\s*([+-]?\d*\.\d+|\d+))");
            if (std::regex_search(tmp, match, re6))
            {
                // Extract the value (radius)
                double rd = std::stod(match[1])*1e3;
                radius = Eigen::Vector3d(rd, rd, rd);
            }

            std::regex re7(R"(Radius\s*\(km\)\s*=\s*([+-]?\d*\.\d+|\d+)\s*x\s*([+-]?\d*\.\d+|\d+)\s*x\s*([+-]?\d*\.\d+|\d+))");
            if (std::regex_search(tmp, match, re7)) {
                // Extract the three values
                double value1 = std::stod(match[1])*1e3;
                double value2 = std::stod(match[2])*1e3;
                double value3 = std::stod(match[3])*1e3;
                radius = Eigen::Vector3d(value1, value2, value3);
            }
            std::regex re7_1(R"(Radius\s*\(km\)\s*=\s*([+-]?\d*\.\d+|\d+)\s*\+\-\s*([+-]?\d*\.\d+|\d+))");
            if (std::regex_search(tmp, match, re7_1)) {
                // Extract the three values
                double rd = std::stod(match[1])*1e3;
                radius = Eigen::Vector3d(rd, rd, rd);
            }
            std::regex re7_2(R"(Radius\s*\(km,\s*IAU2015\)\s*=\s*([+-]?\d*\.\d+|\d+)\s*\+\-\s*([+-]?\d*\.\d+|\d+))");
            if (std::regex_search(tmp, match, re7_2)) {
                // Extract the three values
                double rd = std::stod(match[1])*1e3;
                radius = Eigen::Vector3d(rd, rd, rd);
            }
            std::regex re8(R"(RAD\s*=\s*([+-]?\d*\.\d+|\d+))");
            if (std::regex_search(tmp, match, re8))
            {
                double rd = std::stod(match[1])*1e3;
                radius = Eigen::Vector3d(rd, rd, rd);
            }
        }
        else
        {
            is_desc = false;
        }
    }
    desc = oss_desc.str();
    csv = oss_csv.str();
    return is_big;
}

std::fstream openfile(const fs::path file_path,
                      const std::ios_base::openmode mode)
{
    if (!fs::exists(file_path))
    {
        fs::path path(file_path);
        fs::create_directories(path.parent_path());
    }
    std::fstream file(file_path, mode);
    if (file.is_open())
    {
        return file;
    }
    else
    {
        throw(file_path.string() + ": open file failed").c_str();
    }
}

std::string readfile(const fs::path &file_path,
                     const std::string &default_content)
{
    std::ifstream afile;
    afile.open(file_path, std::ios::in);

    if (afile.is_open())
    {
        std::string ans, line;
        while (!afile.eof())
        {
            getline(afile, line);
            ans += line + "\n";
        }
        afile.close();
        return ans;
    }
    else
    {
        try
        {
            std::fstream ofile = openfile(file_path, std::ios::out);
            ofile << default_content;
            ofile.flush();
            ofile.close();
            return default_content;
        }
        catch (...)
        {
            return "";
        }
    }
}

void writefile(const fs::path file_path, const std::string &content,
               bool is_append)
{
    std::fstream ofile;
    try
    {
        ofile = openfile(file_path, is_append ? std::ios::app : std::ios::out);
    }
    catch (...)
    {
    }
    ofile << content;
    ofile.flush();
    ofile.close();
}