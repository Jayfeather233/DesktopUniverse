#pragma once

#include <string>
#include <utility>

// void getTDBWithERFA(int *iy, int *im, int *id, double *fd);
void getTDBWithUTC(int *iy, int *im, int *id, int *ihmin, double *fsec);
void setQueryDayOffset(int delta_month);
std::pair<std::string, std::string> get_query_days();