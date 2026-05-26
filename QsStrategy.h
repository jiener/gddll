#pragma once
#include "pch.h"
#include "StrategyTemplate.h"
#include "ICtaEngine.h"
#include <nlohmann/json.hpp>
#include "qcstructs.h"
#include "tools.h"
#include <chrono>
#include <atomic>
#include <functional>
#include <fstream>
#include <filesystem>

#pragma warning(push)
#pragma warning(disable: 4251)

#ifdef QSBDSTRATEGY_EXPORTS
#define QSBDSTRATEGY_API __declspec(dllexport)
#else
#define QSBDSTRATEGY_API __declspec(dllimport)
#endif

using json = nlohmann::ordered_json;

// 账户调整结构体
struct AccountAdjustment {
    std::string accountId;
    int closeShortVolume = 0;    // 需要平空的手数
    int closeLongVolume = 0;     // 需要平多的手数
    int openLongVolume = 0;      // 需要开多的手数
    int openShortVolume = 0;     // 需要开空的手数
    double fundRatio = 0.0;      // 资金比例
    int currentNetPosition = 0;   // 当前净持仓
    int targetNetPosition = 0;    // 目标净持仓
};

// 跟单状态枚举
enum class FollowStatus {
    NORMAL,      // 正常
    PAUSED,      // 暂停
    ERR,         // 错误
    CIRCUIT_BREAK // 熔断
};

// 风险控制配置
struct RiskControlConfig {
    double maxFollowRatio = 2.0;        // 最大跟单比例
    double minFollowRatio = 0.1;        // 最小跟单比例
    int maxDailyTrades = 1000;           // 单日最大交易次数
    int maxConsecutiveErrors = 5;        // 最大连续错误次数
    double maxSingleTradeRatio = 0.5;    // 单次交易最大资金比例
    int minNetPositionChange = 1;        // 最小净持仓变化阈值
};

// 全局设置
struct GlobalSettings {
    int priceAdjustmentTicks = 5;        // 价格调整跳数
    int minPositionChange = 1;           // 最小持仓变化
    double maxFollowRatio = 10.0;        // 最大跟单比例
    double minFollowRatio = 0.1;         // 最小跟单比例
    int maxConsecutiveErrors = 5;        // 最大连续错误
    int circuitBreakDuration = 5;        // 熔断持续时间（分钟）
};

// 极简跟单配置
struct FollowConfig {
    std::string masterAccount;           // 主账户
    std::map<std::string, double> followers;  // 跟随者 key=账户ID, value=系数
    GlobalSettings settings;             // 全局设置

    // 配置文件信息
    std::string configFilePath;
    std::chrono::system_clock::time_point lastModified;

    // 获取账户系数（0表示不跟单）
    double getCoefficient(const std::string& accountId) const {
        auto it = followers.find(accountId);
        return (it != followers.end()) ? it->second : 0.0;
    }

    // 检查账户是否启用跟单（系数>0表示启用）
    bool isAccountEnabled(const std::string& accountId) const {
        return getCoefficient(accountId) > 0.0;
    }

    // 获取所有启用的跟随者
    std::vector<std::pair<std::string, double>> getEnabledFollowers() const {
        std::vector<std::pair<std::string, double>> result;
        for (const auto& [accountId, coefficient] : followers) {
            if (coefficient > 0.0) {
                result.emplace_back(accountId, coefficient);
            }
        }
        return result;
    }
};

// 跟单配置管理器
class FollowConfigManager {
public:
    FollowConfigManager(const std::string& configPath = "Strategy/follow_config.json");

    // 配置文件操作
    bool loadConfig();
    bool saveConfig();
    bool isConfigModified() const;
    bool reloadIfModified();

    // 获取配置
    const FollowConfig& getConfig() const { return m_config; }

    // 动态配置接口
    bool setCoefficient(const std::string& accountId, double coefficient);
    bool disableAccount(const std::string& accountId);
    bool enableAccount(const std::string& accountId, double coefficient = 1.0);
    bool setMasterAccount(const std::string& accountId);

    // 配置信息
    std::string getConfigSummary() const;

private:
    FollowConfig m_config;
    std::string m_configPath;

    void createDefaultConfig();
    void updateFileTimestamp();
};

class QSBDSTRATEGY_API QsStrategy : public StrategyTemplate
{
public:
    QsStrategy(ICTAEngine* ctaEngine, const std::string& strategyName, const std::string& symbol, const std::string& className);
    ~QsStrategy();

    // 基础策略接口
    void onInit() override;
    void onStart() override;
    void onStop() override;
    void updateSetting() override;
    void putEvent() override;

    // 市场数据处理
    void onTick(const TickData& Tick) override;
    void onBar(const BarData& Bar) override;
    void on_5min_bar(const BarData& data);

    // 交易事件处理
    void onOrder(const std::shared_ptr<Event_Order>& e) override;
    void onTrade(const std::shared_ptr<Event_Trade>& e) override;
    void onStopOrder(const std::shared_ptr<Event_StopOrder>& e) override;

    // 自定义事件处理
    void onOpener(const std::shared_ptr<Event>& e);
    void onCallall(const std::shared_ptr<Event>& e);
    void onStarting(const std::shared_ptr<Event>& e);
    void handleFile(std::shared_ptr<Event> e);

    // 重写交易函数
    std::vector<std::string> buy(double price, int volume, bool bStopOrder = false) override;
    std::vector<std::string> sell(double price, int volume, bool bStopOrder = false) override;
    std::vector<std::string> sellshort(double price, int volume, bool bStopOrder = false) override;
    std::vector<std::string> buycover(double price, int volume, bool bStopOrder = false) override;

    // 跟单核心功能
    void executeFollowStrategy();
    void calculateAccountAdjustments(const std::string& masterAccountId,
        const std::shared_ptr<Event_Account>& masterAccount,
        int masterNetPosition,
        std::vector<AccountAdjustment>& adjustments);
    void calculateSpecificOperations(int currentLong, int currentShort,
        int targetNet, AccountAdjustment& adjustment);
    void executeFollowOperations(const std::vector<AccountAdjustment>& adjustments,
        double buyPrice, double sellPrice);
    void executeOperation(const std::string& operationName,
        const std::map<std::string, int>& accountNeeds,
        const std::map<std::string, int>& originalConfig,
        const Strategy& strategy,
        double price,
        std::function<std::vector<std::string>(double, int)> orderFunction);

    // 订单管理
    void checkOrdersInOnTick();
    void resendOrder(std::shared_ptr<Event_Order> orderEvent);
    void trackOrder(const std::shared_ptr<Event_Order>& orderEvent);
    void removeOrder(const std::string& orderID);
    void clearorder();

    // 风险控制
    bool checkRiskLimits(const std::string& accountId, int volume, double price);
    bool checkFollowRatio(double ratio);
    void updateRiskMetrics();
    void handleCircuitBreak();
    void resetCircuitBreak();

    // 频率控制
    void updateFollowTrigger(int masterNetPosition);

    // 数据一致性检查
    bool validateAccountData(const std::string& accountId);
    bool validatePositionData(const std::map<std::string, std::shared_ptr<Event_Position>>& positions);

    // 监控和日志
    void logFollowDetails(const std::vector<AccountAdjustment>& adjustments);
    void logRiskMetrics();
    void logFollowConfig();

    // 配置管理
    void loadRiskConfigFromFile();
    void checkAndUpdateConfig();

    // 动态配置接口
    bool setAccountCoefficient(const std::string& accountId, double coefficient);
    bool disableAccount(const std::string& accountId);
    bool enableAccount(const std::string& accountId, double coefficient = 1.0);
    bool setMasterAccount(const std::string& accountId);

    // 状态查询
    std::string getMasterAccountId() const;
    bool isStrategyEnabled() const;
    std::string getConfigSummary() const;

public:
    std::unique_ptr<BarGenerator> m_BarGenerate;
    std::unique_ptr<ArrayManager> m_ArrayManager;

private:
    // 市场数据
    TickData last_tick;

    // 跟单配置管理器
    std::unique_ptr<FollowConfigManager> m_configManager;
    std::chrono::steady_clock::time_point m_lastConfigCheck;

    // 订单管理
    std::unordered_map<std::string, std::shared_ptr<Event_Order>> m_activeOrders;
    std::mutex m_orderMutex;
    std::map<std::string, std::shared_ptr<Event_Order>> m_ordersPendingCancellation;

    // 跟单控制
    FollowStatus m_followStatus;
    std::atomic<bool> m_isFollowEnabled;
    std::mutex m_followMutex;

    // 频率控制
    std::chrono::steady_clock::time_point m_lastFollowTime;
    int m_lastMasterNetPosition;
    int m_tickCounter;

    // 风险控制
    RiskControlConfig m_riskConfig;
    std::map<std::string, int> m_dailyTradeCount; // 每个账户的日交易次数
    int m_consecutiveErrors;
    std::chrono::steady_clock::time_point m_circuitBreakTime;
    int m_circuitBreakDuration; // 熔断持续时间（分钟）

    // 配置参数
    const int MAX_TICK_COUNT = 20;          // 每20个tick检查一次订单
    const int MAX_RETRIES = 5;              // 最大重试次数

    // 文件处理
    bool m_hasUpdatedVolume;
    int m_size;

    // 私有辅助方法
    void setupBarGenerator();
    void registerEvents();
    void initializeStrategy();
    void resetDailyCounters();
    double calculateAvailableMargin(const std::string& accountId);
    RiskControlConfig convertToRiskConfig(const GlobalSettings& globalSettings);
};

extern "C" {
    __declspec(dllexport) StrategyTemplate* createStrategy(ICTAEngine* engine, const char* name, const char* symbol, const char* className);
}

#pragma warning(pop)