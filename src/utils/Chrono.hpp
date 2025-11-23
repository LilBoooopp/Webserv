#pragma once
#include <ctime>
#include <iomanip>
#include <sstream>
#include <sys/time.h>
#include <string>

std::string getTimestamp();
std::string formatTime(time_t t);