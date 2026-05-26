#pragma once
// ICTAEngine.h
#ifndef ICTAENGINE_H
#define ICTAENGINE_H

#include"IEventEngine.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include "qcstructs.h"

// 定义策略配置信息的结构体
struct Strategy {
    std::string strategyName;
    std::string vtSymbol;
    std::string className;
    bool operator<(const Strategy& other) const {
        return std::tie(strategyName, vtSymbol, className) <
            std::tie(other.strategyName, other.vtSymbol, other.className);
    }
};

//订单聚合结构体
struct AggregatedOrder {
    std::vector<std::shared_ptr<Event_Order>> subOrders;
    int totalVolume = 0;
    int tradedVolume = 0;
    std::string status = STATUS_UNKNOWN;
    Strategy strategy;
};

typedef std::function<void(std::shared_ptr<Event>)> TASK;
// 前向声明 StrategyTemplate 类
class StrategyTemplate;

// CTA 引擎接口
class ICTAEngine {
public:
    virtual ~ICTAEngine() = default;

    // 策略所需函数
    virtual std::vector<std::string> sendOrder(bool bStopOrder, const std::string& symbol, const std::string& strDirection,
        const std::string& strOffset, double price, double volume, StrategyTemplate* Strategy) = 0;

    virtual void cancelrealOrder(const std::string& orderID, const std::string& gatewayName) = 0;
    virtual void cancelOrder(const std::string& orderID, const std::string& gatewayName) = 0;
    virtual void cancelAllOrder(const std::string& strStragetyName) = 0;
    /* virtual void cancel_local_stop_order(const std::string& orderID) = 0;*/

    virtual void writeCtaLog(const std::string& msg) = 0;

    virtual void PutEvent(const std::shared_ptr<Event>& e) = 0;

    virtual std::vector<TickData> loadTick(const std::string& symbol, int days) = 0;
    virtual std::vector<BarData> loadBar(const std::string& symbol, int days) = 0;
    virtual void RegEvent(std::string eventType, TASK task) = 0;
    virtual std::shared_ptr<Event_Contract> getContract(const std::string& symbol) = 0;
    virtual std::map<std::string, std::shared_ptr<Event_Position>> getPosition(const std::string& symbol) = 0;
    virtual std::shared_ptr<Event_Account> getAccount(const std::string& account) = 0;
    virtual std::map<Strategy, std::map<std::string, double>> getaccountConfigs() = 0;
    // 修改配置的方法
    virtual void updateAccountVolume(const Strategy& strategy, const std::string& accountId, int volume) = 0;
    virtual void updateStrategyVolumes(const Strategy& strategy, double multiplier) = 0;
    virtual void checkAndResendOrders(const std::string& orderID, int maxRetries = 3) = 0;

    // 添加恢复手数的虚函数
    virtual void restoreStrategyVolumes(const Strategy& strategy) = 0;
};

#endif // ICTAENGINE_H
