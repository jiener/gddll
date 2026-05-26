#include "StrategyTemplate.h"
#include <cstdlib>
#include <string>
#include <memory>
#include "ICTAEngine.h"
#include "pch.h"
#include <string>
#include <memory>
#include <mutex>
#include "StrategyTemplate.h"
#include <vector>
#include <string>
#include <memory>
#include <mutex>

StrategyTemplate::StrategyTemplate(ICTAEngine* ctaEngine, const std::string& strategyName, const std::string& symbol, const std::string& className)
    : m_ctaEngine(ctaEngine),
    gatewayname("CTP"),
    inited(false),
    trading(false),
    m_Pos(0),
    initDays(90),  // 加载历史数据的天数
    m_strategyName(strategyName),
    m_className(className),
    m_symbol(symbol),
    m_strategydata(std::make_unique<StrategyData>())
{
    m_strategydata->insertvar("pos", "0");
    m_ctaEngine->RegEvent(EVENT_UPDATESTRATEGY, std::bind(&StrategyTemplate::updateGUI, this, std::placeholders::_1));

}

StrategyTemplate::~StrategyTemplate()
{
    // m_strategydata 将自动析构，无需手动 delete
}

void StrategyTemplate::sync_data()
{
    
}

void StrategyTemplate::updateSetting()
{

}

void StrategyTemplate::setPos(int pos)
{
    m_Pos = pos;
    m_strategydata->setvar("pos", std::to_string(pos));  // 变量表中的 pos 也需要更新
}

void StrategyTemplate::changeposmap(const std::string& symbol, double pos)
{
    std::lock_guard<std::mutex> lock(m_Pos_mapmtx);
    m_pos_map[symbol] = pos;
}

// 初始化
void StrategyTemplate::onInit()
{
    std::string strategyname = m_strategyName;
    m_ctaEngine->writeCtaLog("策略初始化 " + strategyname);

    // 加载历史数据
    std::vector<BarData> datalist = loadBar(m_symbol, initDays);
    if (datalist.empty())
    {
        m_ctaEngine->writeCtaLog("无法加载历史数据，初始化失败");
        return;
    }

    // 获取最后一个收盘价
    m_barclose = datalist.back().close;

    // 获取合约信息
    m_contract = m_ctaEngine->getContract(m_symbol);
    if (!m_contract)
    {
        m_ctaEngine->writeCtaLog("错误：无法获取合约信息：" + m_symbol);
        return;
    }


    // 运行回测历史数据
    for (const auto& bar : datalist)
    {
        onBar(bar);
    }

    inited = true;
}

// 开始
void StrategyTemplate::onStart()
{
    std::string strategyname = m_strategyName;
    m_ctaEngine->writeCtaLog(strategyname + "，策略已启动");
    trading = true;
    putEvent();
}

// 停止
void StrategyTemplate::onStop()
{
    std::string strategyname = m_strategyName;
    //m_ctaEngine->writeCtaLog(strategyname + "，策略停止");
    trading = false;
    putEvent();
}

// 给参数赋值
void StrategyTemplate::checkparam(const std::string& paramname, const std::string& paramvalue)
{
    m_strategydata->insertparam(paramname, paramvalue);
}

// 更新参数的值
void StrategyTemplate::updateParam(const std::string& paramname, const std::string& paramvalue)
{
    m_strategydata->setparam(paramname, paramvalue);
}

// 更新变量的值
void StrategyTemplate::updateVar(const std::string& paramname, const std::string& paramvalue)
{
    m_strategydata->setvar(paramname, paramvalue);
}

// TICK
void StrategyTemplate::onTick(const TickData& Tick)
{
    putEvent();
}

// BAR
void StrategyTemplate::onBar(const BarData& Bar)
{
    putEvent();
}

// 报单回调
void StrategyTemplate::onOrder(const std::shared_ptr<Event_Order>& e)
{
    putEvent();
}

// 成交回调
void StrategyTemplate::onTrade(const std::shared_ptr<Event_Trade>& e)
{
    putEvent();
}

void StrategyTemplate::onStopOrder(const std::shared_ptr<Event_StopOrder>& e)
{
    putEvent();
}

// 做多
std::vector<std::string> StrategyTemplate::buy(double price, int volume, bool bStopOrder)
{
    return sendOrder(bStopOrder, DIRECTION_LONG, OFFSET_OPEN, price, volume);
}

// 平多
std::vector<std::string> StrategyTemplate::sell(double price, int volume, bool bStopOrder)
{
    return sendOrder(bStopOrder, DIRECTION_SHORT, OFFSET_CLOSE, price, volume);
}

// 做空
std::vector<std::string> StrategyTemplate::sellshort(double price, int volume, bool bStopOrder)
{
    return sendOrder(bStopOrder, DIRECTION_SHORT, OFFSET_OPEN, price, volume);
}

// 平空
std::vector<std::string> StrategyTemplate::buycover(double price, int volume, bool bStopOrder)
{
    return sendOrder(bStopOrder, DIRECTION_LONG, OFFSET_CLOSE, price, volume);
}

// 总报单开平函数公用
std::vector<std::string> StrategyTemplate::sendOrder(bool bStopOrder, const std::string& strDirection, const std::string& strOffset, double price, double volume)
{
    std::vector<std::string> orderIDv;
    if (trading)
    {
        std::string symbol = m_symbol;
        orderIDv = m_ctaEngine->sendOrder(bStopOrder, symbol, strDirection, strOffset, price, volume, this);
        {
            std::lock_guard<std::mutex> lock(m_orderlistmtx);
            m_orderlist.insert(m_orderlist.end(), orderIDv.begin(), orderIDv.end());
        }
    }
    return orderIDv;
}

// 撤掉这个策略所有单
void StrategyTemplate::cancelAllOrder()
{
    m_ctaEngine->cancelAllOrder(m_strategyName);
}

// 撤交易所的单
void StrategyTemplate::cancelOrder(const std::string& orderID, const std::string& gatewayname)
{
    m_ctaEngine->cancelOrder(orderID, gatewayname);
}

// 读取历史数据
std::vector<TickData> StrategyTemplate::loadTick(const std::string& symbol, int days)
{
    return m_ctaEngine->loadTick(symbol, days);
}

std::vector<BarData> StrategyTemplate::loadBar(const std::string& symbol, int days)
{
    return m_ctaEngine->loadBar(symbol, days);
}

// 获取参数的值
std::string StrategyTemplate::getparam(const std::string& paramname) const
{
    return m_strategydata->getparam(paramname);
}

// 更新参数到界面
void StrategyTemplate::putEvent()
{
    auto e = std::make_shared<Event_UpdateStrategy>();
    e->parammap = m_strategydata->getallparam();
    e->varmap = m_strategydata->getallvar();
    e->strategyname = m_strategyName;
    m_ctaEngine->PutEvent(e);
}

void StrategyTemplate::putGUI()
{
    // 推送到 GUI
    auto e = std::make_shared<Event_LoadStrategy>();
    e->parammap = m_strategydata->getallparam();
    e->varmap = m_strategydata->getallvar();
    e->strategyname = m_strategyName;
    m_ctaEngine->PutEvent(e);
}

//更新界面参数
void StrategyTemplate::updateGUI(std::shared_ptr<Event> e)
{
    

}

void StrategyTemplate::checkSymbol(const std::string& symbolname)
{
    // 读取
    changeposmap(symbolname, 0);
}

int StrategyTemplate::getpos() const
{
    return m_Pos;
}

// StrategyData 类的实现
void StrategyData::insertparam(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    m_parammap[key] = value;
}

void StrategyData::insertvar(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    m_varmap[key] = value;
}

std::string StrategyData::getparam(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(m_mtx);
    auto it = m_parammap.find(key);
    if (it != m_parammap.end())
    {
        return it->second;
    }
    else
    {
        return "Null";
    }
}

void StrategyData::setparam(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    m_parammap[key] = value;
}

void StrategyData::setvar(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    m_varmap[key] = value;
}

std::string StrategyData::getvar(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(m_mtx);
    auto it = m_varmap.find(key);
    if (it != m_varmap.end())
    {
        return it->second;
    }
    else
    {
        return "Null";
    }
}

std::map<std::string, std::string> StrategyData::getallparam() const
{
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_parammap;
}

std::map<std::string, std::string> StrategyData::getallvar() const
{
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_varmap;
}
