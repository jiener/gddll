#ifndef JSTOOLS_H
#define JSTOOLS_H

#include <string>
#include <sstream>
#include <chrono>
#include <vector>
#include <regex>
#include <iomanip>
#include <ctime>

class Utils {
public:
    static std::string getCurrentSystemTime() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        struct tm timeinfo;
        localtime_s(&timeinfo, &in_time_t);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        return std::string(buffer);
    }

    static std::string getCurrentSystemDate() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        struct tm timeinfo;
        localtime_s(&timeinfo, &in_time_t);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
        return std::string(buffer);
    }

    static std::string booltostring(bool var) {
        return var ? "true" : "false";
    }

    static bool stringtobool(const std::string& str) {
        return str == "true";
    }

    static std::string doubletostring(double var) {
        std::ostringstream ss;
        ss << var;
        return ss.str();
    }

    static time_t getsystemunixdatetime(const std::string& time, const std::string& type) {
        struct tm tm = {};
        int milliseconds = 0;
        sscanf_s(time.c_str(), "%d:%d:%d.%d", &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &milliseconds);

        time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm today;
        localtime_s(&today, &now);

        tm.tm_year = today.tm_year;
        tm.tm_mon = today.tm_mon;
        tm.tm_mday = today.tm_mday;

        time_t result = mktime(&tm);

        if (type == "s") {
            return result;
        }
        else if (type == "ms") {
            return result * 1000 + milliseconds;
        }
        return 0;
    }

    static time_t timetounixtimestamp(int hour, int minute, int seconds) {
        time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = minute;
        timeinfo.tm_sec = seconds;
        return mktime(&timeinfo);
    }

    static std::vector<std::string> split(const std::string& str, const std::string& pattern) {
        std::vector<std::string> result;
        size_t start = 0, end = 0;
        while ((end = str.find(pattern, start)) != std::string::npos) {
            result.push_back(str.substr(start, end - start));
            start = end + pattern.length();
        }
        result.push_back(str.substr(start));
        return result;
    }

    static std::string regMySymbol(const std::string& symbol) {
        return std::regex_replace(symbol, std::regex("\\d"), "");
    }

    static bool isnum(const std::string& s) {
        return !s.empty() && s.find_first_not_of("0123456789.") == std::string::npos;
    }

    static int getWeedDay(const std::string& date = "") {
        struct tm timeinfo = {};
        if (date.empty()) {
            time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            localtime_s(&timeinfo, &now);
        }
        else {
            sscanf_s(date.c_str(), "%d-%d-%d", &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday);
            timeinfo.tm_year -= 1900;
            timeinfo.tm_mon -= 1;
        }
        mktime(&timeinfo);  // This call adjusts the tm structure if needed
        int wday = timeinfo.tm_wday;
        return (wday == 0) ? 7 : wday;
    }
};

#endif