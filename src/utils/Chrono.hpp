#pragma once
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/time.h>

std::string getTimestamp();
std::string formatTime(time_t t);
unsigned long now_ms();