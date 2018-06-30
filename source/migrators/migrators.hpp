#include <utility>
#include <tuple>

void ss1xMigrate(void* null);
//Current mod, whether it's done or not
std::pair<int, bool> ss1xretrieveinfo();

void ss2xMigrate(void* null);
//Current mod being moved, total folder count, and whether it's done or not
std::tuple<int, int, bool> ss2xretriveinfo();