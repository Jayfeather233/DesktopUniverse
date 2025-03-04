#include <fmt/core.h>
#include <cstdlib>

#define GLM_ENABLE_EXPERIMENTAL
#include "render.hpp"

renderer r;

bool test_curl()
{
    if (system("curl --version"))
    {
        fmt::print("No cURL found! please install it first.");
        return false;
    }
    return true;
}

int main()
{
    fmt::print("DesktopUniSim\n");
    if (!test_curl())
    {
        return 1;
    }
    r.initOGL();
    r.renderLoop();
    return 0;
}
