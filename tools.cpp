#include "pch.h"
#include "tools.h"
#include <algorithm>


std::string getProductID(const std::string& symbol) {
    std::string productID;

    for (char c : symbol) {
        // 遇到数字就停止提取
        if (std::isdigit(c)) {
            break;
        }

        // 只提取字母并转换为大写
        if (std::isalpha(c)) {
            productID += std::toupper(c);
        }
    }

    return productID;
}

//----------------------- TimeHelper 实现 -----------------------
void TimeHelper::parseTime(const std::string& timeStr, int& hour, int& minute, int& second) {
    hour = std::stoi(timeStr.substr(0, 2));
    minute = std::stoi(timeStr.substr(3, 2));
    second = std::stoi(timeStr.substr(6, 2));
}

void TimeHelper::parseDate(const std::string& dateStr, int& year, int& month, int& day) {
    year = std::stoi(dateStr.substr(0, 4));
    month = std::stoi(dateStr.substr(4, 2));
    day = std::stoi(dateStr.substr(6, 2));
}

bool TimeHelper::compareDateTime(const std::string& date1, const std::string& time1,
    const std::string& date2, const std::string& time2) {
    if (date1 != date2) return date1 < date2;
    return time1 < time2;
}

bool TimeHelper::isSameMinute(const std::string& time1, const std::string& time2) {
    return time1.substr(0, 5) == time2.substr(0, 5);
}

int TimeHelper::getMinute(const std::string& timeStr) {
    return std::stoi(timeStr.substr(3, 2));
}

std::string TimeHelper::formatToMinute(const std::string& timeStr) {
    return timeStr.substr(0, 5) + ":00";
}

//----------------------- ArrayManager 实现 -----------------------
ArrayManager::ArrayManager(int iSize) : m_iSize(iSize) {
    newprice_array.resize(m_iSize);
    openprice_array.resize(m_iSize);
    closeprice_array.resize(m_iSize);
    highprice_array.resize(m_iSize);
    lowprice_array.resize(m_iSize);
    volume_array.resize(m_iSize);
    openinterest_array.resize(m_iSize);
}

void ArrayManager::update_bar(const BarData& barData) {
    if (!m_iInit) {
        openprice_array[m_iCount] = barData.open;
        closeprice_array[m_iCount] = barData.close;
        highprice_array[m_iCount] = barData.high;
        lowprice_array[m_iCount] = barData.low;
        volume_array[m_iCount] = barData.volume;
        openinterest_array[m_iCount] = barData.openInterest;

        if (m_iCount == 0) {
            newprice_array[m_iCount] = 0;
        }
        else {
            newprice_array[m_iCount - 1] = barData.close;
            newprice_array[m_iCount] = barData.close;
        }
    }
    else {
        // 移动数据
        for (int i = 0; i < m_iSize - 1; i++) {
            openprice_array[i] = openprice_array[i + 1];
            closeprice_array[i] = closeprice_array[i + 1];
            highprice_array[i] = highprice_array[i + 1];
            lowprice_array[i] = lowprice_array[i + 1];
            volume_array[i] = volume_array[i + 1];
            openinterest_array[i] = openinterest_array[i + 1];
            newprice_array[i] = newprice_array[i + 1];
        }

        // 更新最新数据
        openprice_array[m_iSize - 1] = barData.open;
        closeprice_array[m_iSize - 1] = barData.close;
        highprice_array[m_iSize - 1] = barData.high;
        lowprice_array[m_iSize - 1] = barData.low;
        volume_array[m_iSize - 1] = barData.volume;
        openinterest_array[m_iSize - 1] = barData.openInterest;
        newprice_array[m_iSize - 1] = barData.close;
        newprice_array[m_iSize - 2] = barData.close;
    }

    m_iCount++;
    if (m_iCount >= m_iSize) {
        m_iInit = true;
    }
}

double* ArrayManager::sma(int n) {
    static double outReal[100];
    double inReal[100];
    int startIdx = 0;
    int endIdx = m_iSize - 1;
    int outBegIdx = 0;
    int outNBElement = 0;

    for (int i = 0; i < m_iSize; i++) {
        inReal[i] = closeprice_array[i];
    }

    TA_SMA(startIdx, endIdx, inReal, n, &outBegIdx, &outNBElement, outReal);
    return outReal;
}

double* ArrayManager::ema(int n) {
    static double out[350];
    double inReal[350];
    double outReal[350];
    int startIdx = 0;
    int endIdx = m_iSize - 1;
    int outBegIdx;
    int outNBElement;

    for (int i = 0; i < m_iSize; i++) {
        inReal[i] = closeprice_array[i];
    }

    TA_EMA(startIdx, endIdx, inReal, n, &outBegIdx, &outNBElement, outReal);

    for (int i = 0; i < m_iSize; i++) {
        if (i >= n) {
            out[i] = outReal[i - n + 1];
        }
        else {
            out[i] = 0;
        }
    }
    return out;
}

std::map<std::string, double> ArrayManager::boll(int iWindow, int iDev) {
    std::map<std::string, double> mapBoll;
    double inReal[100];
    double outRealUpperBand[100];
    double outRealMiddleBand[100];
    double outRealLowerBand[100];
    int outBegIdx;
    int outNBElement;

    for (int i = 0; i < m_iSize; i++) {
        inReal[i] = openprice_array[i];
    }

    TA_BBANDS(0, m_iSize - 1, inReal, iWindow, iDev, iDev,
        TA_MAType_SMA, &outBegIdx, &outNBElement,
        outRealUpperBand, outRealMiddleBand, outRealLowerBand);

    mapBoll["boll_up"] = outRealUpperBand[outNBElement - 1];
    mapBoll["boll_middle"] = outRealMiddleBand[outNBElement - 1];
    mapBoll["boll_down"] = outRealLowerBand[outNBElement - 1];
    return mapBoll;
}

double ArrayManager::cci(int iWindow) {
    double inHigh[100];
    double inLow[100];
    double inClose[100];
    double outReal[100];
    int outBegIdx;
    int outNBElement;

    for (int i = 0; i < m_iSize; i++) {
        inHigh[i] = highprice_array[i];
        inLow[i] = lowprice_array[i];
        inClose[i] = closeprice_array[i];
    }

    TA_CCI(0, m_iSize - 1, inHigh, inLow, inClose, iWindow,
        &outBegIdx, &outNBElement, outReal);
    return outReal[outNBElement - 1];
}

double ArrayManager::atr(int iWindow) {
    double inHigh[100];
    double inLow[100];
    double inClose[100];
    double outReal[100];
    int outBegIdx;
    int outNBElement;

    for (int i = 0; i < m_iSize; i++) {
        inHigh[i] = highprice_array[i];
        inLow[i] = lowprice_array[i];
        inClose[i] = closeprice_array[i];
    }

    TA_ATR(0, m_iSize - 1, inHigh, inLow, inClose, iWindow,
        &outBegIdx, &outNBElement, outReal);
    return outReal[outNBElement - 1];
}

//----------------------- BarGenerator 实现 -----------------------
BarGenerator::BarGenerator(ON_Functional onBar_Func, int iWindow,
    ON_Functional onWindowBar_FUNC, Interval iInterval)
    : m_onBar_Func(onBar_Func)
    , m_onWindowBar_FUNC(onWindowBar_FUNC)
    , m_iWindow(iWindow)
    , m_interval(iInterval)
{
}

void BarGenerator::updateTick(const TickData& tickData) {
    if (tickData.lastprice == 0) return;

    if (m_lastTick && TimeHelper::compareDateTime(tickData.date, tickData.time,
        m_lastTick->date, m_lastTick->time)) {
        return;
    }

    bool bNewMinute = false;
    if (!m_Bar) {
        m_Bar = std::make_unique<BarData>();
        bNewMinute = true;
    }
    else if (!TimeHelper::isSameMinute(m_Bar->time, tickData.time)) {
        m_Bar->time = TimeHelper::formatToMinute(m_Bar->time);
        m_onBar_Func(*m_Bar);
        bNewMinute = true;
    }

    if (bNewMinute) {
        m_Bar->symbol = tickData.symbol;
        m_Bar->exchange = tickData.exchange;
        m_Bar->interval = MINUTE;
        m_Bar->open = tickData.lastprice;
        m_Bar->high = tickData.lastprice;
        m_Bar->low = tickData.lastprice;
        m_Bar->close = tickData.lastprice;
        m_Bar->date = tickData.date;
        m_Bar->time = tickData.time;
        m_Bar->datetime = tickData.date + " " + tickData.time;
        m_Bar->openInterest = tickData.openInterest;
        m_Bar->volume = 0;
    }
    else {
        m_Bar->high = std::max(m_Bar->high, tickData.highPrice);
        m_Bar->low = std::min(m_Bar->low, tickData.lowPrice);
        m_Bar->close = tickData.lastprice;
        m_Bar->openInterest = tickData.openInterest;
    }

    // 更新成交量
    if (!m_lastTick) {
        m_lastTick = std::make_unique<TickData>();
    }
    m_Bar->volume += tickData.volume;

    // 保存最新Tick
    *m_lastTick = tickData;
}

void BarGenerator::updateBar(const BarData& barData) {
    // 初始化或更新X分钟Bar
    if (!m_windowBar) {
        m_windowBar = std::make_unique<BarData>(barData);
    }
    else if (m_bWindowFinished) {
        *m_windowBar = barData;
    }
    else {
        // 更新现有windowBar的数据
        m_windowBar->low = std::min(m_windowBar->low, barData.low);
        m_windowBar->high = std::max(m_windowBar->high, barData.high);
        m_windowBar->close = barData.close;
        m_windowBar->volume += barData.volume;
        m_windowBar->openInterest = barData.openInterest;
        m_windowBar->datetime = barData.datetime;
    }

    bool m_bWindowFinished = false;
    if (m_interval == MINUTE) {
        int iMinute = TimeHelper::getMinute(barData.time);
        int f = (iMinute + 1) % m_iWindow;
        if (f == 0) {
            m_bWindowFinished = true;
        }
        else if ((m_lastBar && barData.time == "10:14:00") && m_iWindow == 30) {
            m_bWindowFinished = true;
        }
    }

    /*
    else if (m_interval == HOUR)
    {//待写
    }
    else if(m_interval == DAILY)
    {//待写}
    */

    if (m_bWindowFinished) {
        m_onWindowBar_FUNC(*m_windowBar);
        m_windowBar.reset();
    }

    if (!m_lastBar) {
        m_lastBar = std::make_unique<BarData>();
    }
    *m_lastBar = barData;
}