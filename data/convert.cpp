#include <iostream>
#include <fstream>
#include <filesystem>

#include "../src/basic/celestial.hpp"

namespace fs = std::filesystem;

int main() {
    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.is_directory()) {
            std::cout << entry.path() << std::endl;
            for (const auto& csv_file : fs::directory_iterator(entry.path())) {
                if (csv_file.path().extension() != ".csv") {
                    continue;
                }
                std::ifstream file(csv_file.path());
                std::ofstream ofile(csv_file.path().string() + ".bin", std::ios::binary);
                std::string line;
                std::getline(file, line);
                while (std::getline(file, line))
                {
                    std::stringstream ss(line);
                    std::string token;
                    state st;

                    std::getline(ss, token, ','); // JDTDB
                    st.JDTDB = std::stod(token);

                    // Skip Calendar Date (TDB) column (second column), we don't need it
                    std::getline(ss, token, ','); // Calendar Date (TDB)

                    // Read X, Y, Z, VX, VY, VZ, in unit km
                    std::getline(ss, token, ',');
                    st.x = std::stod(token) * 1000;
                    std::getline(ss, token, ',');
                    st.z = -std::stod(token) * 1000;
                    std::getline(ss, token, ',');
                    st.y = std::stod(token) * 1000;
                    std::getline(ss, token, ',');
                    st.vx = std::stod(token) * 1000;
                    std::getline(ss, token, ',');
                    st.vz = -std::stod(token) * 1000;
                    std::getline(ss, token, ',');
                    st.vy = std::stod(token) * 1000;

                    ofile.write(reinterpret_cast<const char*>(&st), sizeof(st));
            
                }
                // break;
            }
        }
    }
}