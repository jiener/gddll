#pragma once

#include "pch.h"
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <functional>
#include <ta_func.h>
#include "ICTAEngine.h"

typedef std::function<void(BarData)> ON_Functional;
std::string getProductID(const std::string& symbol);

/**
 * 时间处理辅助类
 */
class TimeHelper {
private:
    static void parseTime(const std::string& timeStr, int& hour, int& minute, int& second);
    static void parseDate(const std::string& dateStr, int& year, int& month, int& day);

public:
    static bool compareDateTime(const std::string& date1, const std::string& time1,
        const std::string& date2, const std::string& time2);
    static bool isSameMinute(const std::string& time1, const std::string& time2);
    static int getMinute(const std::string& timeStr);
    static std::string formatToMinute(const std::string& timeStr);
};

/**
 * 数据管理类
 */
class ArrayManager {
public:
    explicit ArrayManager(int iSize = 100);
    ~ArrayManager() = default;

    void update_bar(const BarData& barData);

    // 获取数据数组的方法 - 使用引用返回
    const std::vector<double>& Get_newprice_array() const { return newprice_array; }
    const std::vector<double>& Get_openprice_array() const { return openprice_array; }
    const std::vector<double>& Get_closeprice_array() const { return closeprice_array; }
    const std::vector<double>& Get_highprice_array() const { return highprice_array; }
    const std::vector<double>& Get_lowprice_array() const { return lowprice_array; }
    const std::vector<double>& Get_volume_array() const { return volume_array; }
    const std::vector<double>& Get_openinterest_array() const { return openinterest_array; }

    // 技术指标计算方法
    std::map<std::string, double> boll(int iWindow, int iDev);
    double* sma(int n);
    double cci(int iWindow);
    double atr(int iWindow);
    double* ema(int iWindow);

private:
    int m_iSize;
    int m_iCount = 0;
    bool m_iInit = false;

    // 价格和成交量数据
    std::vector<double> newprice_array;
    std::vector<double> openprice_array;
    std::vector<double> closeprice_array;
    std::vector<double> highprice_array;
    std::vector<double> lowprice_array;
    std::vector<double> volume_array;
    std::vector<double> openinterest_array;
};

/**
 * K线生成器类
 */
class BarGenerator {
public:
    BarGenerator(ON_Functional onBar_Func, int iWindow,
        ON_Functional onWindowBar_FUNC, Interval iInterval);
    ~BarGenerator() = default;

    void updateTick(const TickData& tickData);
    void updateBar(const BarData& barData);

private:
    std::unique_ptr<BarData> m_Bar;
    std::unique_ptr<BarData> m_windowBar;
    std::unique_ptr<TickData> m_lastTick;
    std::unique_ptr<BarData> m_lastBar;

    ON_Functional m_onBar_Func;
    ON_Functional m_onWindowBar_FUNC;
    bool m_bWindowFinished = false;
    Interval m_interval;
    int interval_count = 0;
    int m_iWindow;
};