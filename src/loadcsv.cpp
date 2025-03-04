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

std::vector<state> istream2statebin(std::istream &is)
{
    std::vector<state> ret;
    while (is.peek() != EOF)
    {
        state s;
        is.read(reinterpret_cast<char *>(&s), sizeof(state));
        ret.push_back(s);
    }
    return ret;
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
        s.JDTDB = std::stod(token);

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

void save_csvbin(const std::vector<state> &ret, const std::string &filename)
{
    std::ofstream
        ofile(filename, std::ios::binary);
    for (const auto &s : ret)
    {
        ofile.write(reinterpret_cast<const char *>(&s), sizeof(s));
    }
}

void convert_csv(std::string filename)
{
    std::ifstream file(filename);
    auto ret = istream2state(file);
    save_csvbin(ret, filename + ".bin");
}

std::vector<state> get_state_from_content(const std::string &COMMAND, const std::string &content, const std::string &filename){
    std::string desc, csv, name;
    double GM = 0.0;
    bool is_big;
    Eigen::Vector3d radius(1, 1, 1);
    is_big = decode_JPL_result(content, name, desc, csv, GM, radius);
    Json::Value J = string_to_json(readfile("./data/meta.json"));
    if (is_big && !J["celestial"].isMember(COMMAND))
    {
        J["celestial"][COMMAND]["name"] = name;
        J["celestial"][COMMAND]["desc"] = desc;
        J["celestial"][COMMAND]["GM"] = GM;
        for (const auto &u : radius)
            J["celestial"][COMMAND]["radius"].append(u);
    }
    else if (!is_Barycenter(COMMAND) && !J["comet"].isMember(COMMAND))
    {
        J["comet"][COMMAND]["name"] = name;
        J["comet"][COMMAND]["desc"] = desc;
        for (const auto &u : radius)
            J["comet"][COMMAND]["radius"].append(u);
    }
    else if (is_Barycenter(COMMAND) && !J["barycenter"].isMember(COMMAND))
    {
        J["barycenter"][COMMAND]["name"] = name;
        J["barycenter"][COMMAND]["desc"] = desc;
    }
    writefile("./data/meta.json", J.toStyledString());
    std::istringstream iss(csv);
    auto ret = istream2state(iss);
    save_csvbin(ret, filename + ".bin");
    return ret;
}

std::vector<state> get_csv_from_JPL(const std::string &COMMAND, const std::string &filename_csv)
{
    std::string content = download_file_JPL(COMMAND);
    return get_state_from_content(COMMAND, content, filename_csv);
}

std::vector<state> get_csv_from_file(const std::string &COMMAND, const std::string &filename_txt, const std::string &filename_csv)
{
    std::ifstream ifs(filename_txt);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string content = oss.str();
    return get_state_from_content(COMMAND, content, filename_csv);
}

std::vector<state> _get_csv(const std::string &COMMAND, struct tm *utc_time)
{
    if (!fs::exists("./data"))
    {
        fs::create_directory("./data");
    }
    // time_t now = time(NULL);
    // struct tm *utc_time = gmtime(&now);
    utc_time->tm_sec += 69;
    std::mktime(utc_time);

    int tmy = utc_time->tm_year + 1900;
    int tmm = utc_time->tm_mon + 1;

    if (!fs::exists(fmt::format("./data/{}_{}/", tmy, tmm)))
    {
        fs::create_directory(fmt::format("./data/{}_{}/", tmy, tmm));
    }

    std::string filename = fmt::format("./data/{}_{}/{}.csv.bin", tmy, tmm, to_filename(COMMAND));
    if (fs::exists(filename))
    {
        std::fstream f = openfile(filename, std::ios_base::in | std::ios::binary);
        return istream2statebin(f);
    }
    else if (fs::exists(filename = fmt::format("./data/{}_{}/{}.csv", tmy, tmm, to_filename(COMMAND))))
    {
        std::fstream f = openfile(filename, std::ios_base::in);
        convert_csv(filename);
        fs::remove(filename);
        return get_csv(COMMAND, utc_time);
    }
    else if (fs::exists(filename = fmt::format("./data/{}_{}/{}.txt", tmy, tmm, to_filename(COMMAND))))
    {
        fmt::print("Missing csv for {}, ", COMMAND);
        fmt::print("found data file, restoring...\n");
        auto ret = get_csv_from_file(COMMAND, filename, fmt::format("./data/{}_{}/{}.csv", tmy, tmm, to_filename(COMMAND)));
        // fs::remove(filename);
        return ret;
    }
    else
    {
        filename = fmt::format("./data/{}_{}/{}.csv", tmy, tmm, to_filename(COMMAND));
        fmt::print("Missing csv for {}, ", COMMAND);
        fmt::print("downloading...\n");
        std::vector<state> ret = get_csv_from_JPL(COMMAND, filename);
        // fs::remove("./results.txt");
        fs::rename("./results.txt", fmt::format("./data/{}_{}/{}.txt", tmy, tmm, to_filename(COMMAND)));
        return ret;
    }
}

std::vector<state> get_csv(const std::string &COMMAND, struct tm *utc_time) {
    auto ret = _get_csv(COMMAND, utc_time);
    ret.pop_back();
    return ret;
}