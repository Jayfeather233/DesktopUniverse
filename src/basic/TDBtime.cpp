#include "TDBtime.hpp"

#include <chrono>

#include <iomanip>
#include <ostream>
#include <fmt/core.h>

// #include "erfa.h"
// void getTDBWithERFA(int *iy, int *im, int *id, double *fd)
// {
//     std::time_t t = std::time(0);
//     std::tm *tm_utc = std::gmtime(&t);

//     // 1. Convert UTC to TT
//     double utc1, utc2;
//     eraDtf2d("UTC", tm_utc->tm_year + 1900, tm_utc->tm_mon + 1,
//              tm_utc->tm_mday, tm_utc->tm_hour, tm_utc->tm_min,
//              tm_utc->tm_sec, &utc1, &utc2);

//     double tai1, tai2;
//     eraUtctai(utc1, utc2, &tai1, &tai2);

//     double tt1, tt2;
//     eraTaitt(tai1, tai2, &tt1, &tt2);

//     double ut = (tm_utc->tm_hour + tm_utc->tm_min / 60.0 + tm_utc->tm_sec / 3600.0) / 24.0;

//     // 4. Compute TDB-TT correction
//     // 
//     double dtr = eraDtdb(tt1, tt2, ut, elong, u, v);

//     // 5. Convert TT to TDB
//     double tdb1, tdb2;
//     eraTttdb(tt1, tt2, dtr, &tdb1, &tdb2);

//     eraJd2cal(tdb1, tdb2, iy, im, id, fd);
// }

double getTDBsec()
{
    auto now = std::chrono::system_clock::now();

    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    milliseconds -= 1735660800000;
    milliseconds += 212602449600000;
    milliseconds += 69183; // round number for UTC->TDB

    return milliseconds / 1000.0;
}

void getTDBWithUTC(int *iy, int *im, int *id, int *ihmin, double *fsec)
{
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    milliseconds += 69183; // round number for UTC->TDB
    
    // Convert to std::time_t and tm for date part
    std::time_t t = milliseconds / 1000;
    std::tm *tm_utc = std::gmtime(&t);
    
    // Extract date parts
    *iy = tm_utc->tm_year + 1900;
    *im = tm_utc->tm_mon + 1;
    *id = tm_utc->tm_mday;
    *ihmin = tm_utc->tm_hour * 60 + tm_utc->tm_min;
    
    // Calculate the fractional day
    double frac_sec = (milliseconds % 1000) / 1000.0 + tm_utc->tm_sec;
    *fsec = frac_sec;
}

static int dmon = 0;

std::pair<std::string, std::string> get_query_days()
{
    // Get current time
    std::time_t t = std::time(0);
    std::tm *tm = std::gmtime(&t);
    tm->tm_sec += 69;
    tm->tm_mon += dmon;
    std::mktime(tm);

    // Set the day to the first of the current month
    tm->tm_mday = 1;

    // Format the date to "YYYY-MM-DD"
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d");

    tm->tm_mon += 1;
    std::mktime(tm);
    std::ostringstream oss2;
    oss2 << std::put_time(tm, "%Y-%m-%d");

    return std::make_pair(oss.str(), oss2.str());
}

void setQueryDayOffset(int delta_month) {
    dmon = delta_month;
}