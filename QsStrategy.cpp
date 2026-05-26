#include "pch.h"
#include "QsStrategy.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

// ============================================================================
// FollowConfigManager 实现
// ============================================================================

FollowConfigManager::FollowConfigManager(const std::string& configPath)
    : m_configPath(configPath) {
}

bool FollowConfigManager::loadConfig() {
    try {
        std::ifstream configFile(m_configPath);
        if (!configFile.is_open()) {
            createDefaultConfig();
            return saveConfig();
        }

        json configJson;
        configFile >> configJson;

        // 解析主账户
        m_config.masterAccount = configJson.value("master_account", "071749");

        // 解析跟随者（直接存储系数）
        if (configJson.contains("followers")) {
            m_config.followers.clear();
            for (const auto& [accountId, followerJson] : configJson["followers"].items()) {
                double coefficient = followerJson.value("coefficient", 1.0);
                if (coefficient > 0.0) {  // 只保存系数>0的账户
                    m_config.followers[accountId] = coefficient;
                }
            }
        }

        // 解析全局设置
        if (configJson.contains("settings")) {
            auto& settingsJson = configJson["settings"];
            m_config.settings.priceAdjustmentTicks = settingsJson.value("price_adjustment_ticks", 5);
            m_config.settings.minPositionChange = settingsJson.value("min_position_change", 1);
            m_config.settings.maxFollowRatio = settingsJson.value("max_follow_ratio", 10.0);
            m_config.settings.minFollowRatio = settingsJson.value("min_follow_ratio", 0.1);
            m_config.settings.maxConsecutiveErrors = settingsJson.value("max_consecutive_errors", 5);
            m_config.settings.circuitBreakDuration = settingsJson.value("circuit_break_duration", 5);
        }

        updateFileTimestamp();
        return true;

    }
    catch (const std::exception&) {
        createDefaultConfig();
        return false;
    }
}

bool FollowConfigManager::saveConfig() {
    try {
        json configJson;

        // 保存主账户
        configJson["master_account"] = m_config.masterAccount;

        // 保存跟随者（只保存系数>0的）
        json followersJson;
        for (const auto& [accountId, coefficient] : m_config.followers) {
            if (coefficient > 0.0) {
                followersJson[accountId] = {
                    {"coefficient", coefficient}
                };
            }
        }
        configJson["followers"] = followersJson;

        // 保存全局设置
        configJson["settings"] = {
            {"price_adjustment_ticks", m_config.settings.priceAdjustmentTicks},
            {"min_position_change", m_config.settings.minPositionChange},
            {"max_follow_ratio", m_config.settings.maxFollowRatio},
            {"min_follow_ratio", m_config.settings.minFollowRatio},
            {"max_consecutive_errors", m_config.settings.maxConsecutiveErrors},
            {"circuit_break_duration", m_config.settings.circuitBreakDuration}
        };

        // 写入文件
        std::ofstream configFile(m_configPath);
        configFile << configJson.dump(4);

        updateFileTimestamp();
        return true;

    }
    catch (const std::exception&) {
        return false;
    }
}

bool FollowConfigManager::isConfigModified() const {
    try {
        auto currentTime = std::filesystem::last_write_time(m_configPath);
        auto currentTimePoint = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            currentTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        return currentTimePoint != m_config.lastModified;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool FollowConfigManager::reloadIfModified() {
    if (isConfigModified()) {
        return loadConfig();
    }
    return true;
}

bool FollowConfigManager::setCoefficient(const std::string& accountId, double coefficient) {
    if (coefficient <= 0.0) {
        m_config.followers.erase(accountId);  // 系数<=0就删除
    }
    else {
        m_config.followers[accountId] = coefficient;
    }
    return saveConfig();
}

bool FollowConfigManager::disableAccount(const std::string& accountId) {
    return setCoefficient(accountId, 0.0);
}

bool FollowConfigManager::enableAccount(const std::string& accountId, double coefficient) {
    return setCoefficient(accountId, coefficient);
}

bool FollowConfigManager::setMasterAccount(const std::string& accountId) {
    m_config.masterAccount = accountId;
    return saveConfig();
}

std::string FollowConfigManager::getConfigSummary() const {
    std::stringstream ss;
    ss << "[跟单配置]\n";
    ss << "主账户: " << m_config.masterAccount << "\n";
    ss << "跟随账户: " << m_config.followers.size() << "个\n\n";

    if (m_config.followers.empty()) {
        ss << "无跟随者\n";
    }
    else {
        ss << "跟随者列表:\n";
        for (const auto& [accountId, coefficient] : m_config.followers) {
            ss << "  " << accountId << " - 系数: " << coefficient << "\n";
        }
    }

    return ss.str();
}

void FollowConfigManager::createDefaultConfig() {
    m_config = FollowConfig{};
    m_config.masterAccount = "071749";
    m_config.followers["229574"] = 1.0;  // 默认系数1.0
}

void FollowConfigManager::updateFileTimestamp() {
    try {
        auto ftime = std::filesystem::last_write_time(m_configPath);
        m_config.lastModified = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    }
    catch (const std::exception&) {
        m_config.lastModified = std::chrono::system_clock::now();
    }
}

// ============================================================================
// 构造函数和析构函数
// ============================================================================

QsStrategy::QsStrategy(ICTAEngine* ctaEngine, const std::string& strategyName, const std::string& symbol, const std::string& className) :
    StrategyTemplate(ctaEngine, strategyName, symbol, className),
    m_followStatus(FollowStatus::NORMAL),
    m_isFollowEnabled(true),
    m_lastMasterNetPosition(0),
    m_tickCounter(0),
    m_consecutiveErrors(0),
    m_circuitBreakDuration(5),
    m_hasUpdatedVolume(false),
    m_size(0)
{
    // 初始化配置管理器
    m_configManager = std::make_unique<FollowConfigManager>("Strategy/follow_config.json");

    // 加载配置
    if (!m_configManager->loadConfig()) {
        m_ctaEngine->writeCtaLog("[配置错误]跟单配置加载失败，将使用默认配置");
    }

    // 从配置文件加载风险控制参数
    loadRiskConfigFromFile();

    // 初始化组件
    setupBarGenerator();
    registerEvents();
    initializeStrategy();

    // 输出跟单配置
    logFollowConfig();

    m_ctaEngine->writeCtaLog("[初始化]QsStrategy策略已初始化，合约：" + symbol);
}

QsStrategy::~QsStrategy()
{
    m_ctaEngine->writeCtaLog("[析构]QsStrategy策略已销毁，交易品种：" + m_symbol);
}

// ============================================================================
// 基础策略接口实现
// ============================================================================

void QsStrategy::updateSetting()
{
    // 可以在这里更新策略参数
}

void QsStrategy::putEvent()
{
    StrategyTemplate::putEvent();
}

void QsStrategy::onInit()
{
    StrategyTemplate::onInit();
    m_ctaEngine->writeCtaLog("[策略初始化]" + m_strategyName + " 初始化完成");
}

void QsStrategy::onStart()
{
    StrategyTemplate::onStart();
    m_isFollowEnabled = true;
    m_followStatus = FollowStatus::NORMAL;
    resetDailyCounters();
    m_ctaEngine->writeCtaLog("[策略启动]" + m_strategyName + " 已启动");
}

void QsStrategy::onStop()
{
    StrategyTemplate::onStop();
    m_isFollowEnabled = false;
    m_followStatus = FollowStatus::PAUSED;
    m_ctaEngine->writeCtaLog("[策略停止]" + m_strategyName + " 已停止");
}

// ============================================================================
// 市场数据处理
// ============================================================================

void QsStrategy::onTick(const TickData& tick)
{
    last_tick = tick;
    m_BarGenerate->updateTick(tick);
    m_tickCounter++;

    // 定期检查订单状态
    if (m_tickCounter >= MAX_TICK_COUNT) {
        m_tickCounter = 0;
        checkOrdersInOnTick();
    }

    // 执行跟单策略
    executeFollowStrategy();
}

void QsStrategy::onBar(const BarData& data)
{
    m_BarGenerate->updateBar(data);
}

void QsStrategy::on_5min_bar(const BarData& data)
{
    m_ArrayManager->update_bar(data);
}

// ============================================================================
// 跟单核心逻辑
// ============================================================================

void QsStrategy::executeFollowStrategy()
{
    std::lock_guard<std::mutex> lock(m_followMutex);

    try {
        // 1. 基础验证
        if (last_tick.lastprice <= 0) {
            return;
        }

        const auto& config = m_configManager->getConfig();

        // 2. 检查是否有跟随者
        auto enabledFollowers = config.getEnabledFollowers();
        if (enabledFollowers.empty()) {
            return; // 没有跟随者，不需要跟单
        }

        // 3. 获取主账户数据
        std::string masterAccountId = config.masterAccount;
        auto masterAccount = m_ctaEngine->getAccount(masterAccountId);
        auto masterPositions = m_ctaEngine->getPosition(masterAccountId);

        if (!masterAccount || masterPositions.empty()) {
            return;
        }

        double masterBalance = masterAccount->balance;
        if (masterBalance <= 0) {
            return;
        }

        // 4. 数据一致性检查
        if (!validateAccountData(masterAccountId) || !validatePositionData(masterPositions)) {
            m_consecutiveErrors++;
            if (m_consecutiveErrors >= m_riskConfig.maxConsecutiveErrors) {
                handleCircuitBreak();
            }
            return;
        }

        // 5. 获取主账户持仓并计算净持仓
        std::string longPosKey = m_symbol + "2";
        std::string shortPosKey = m_symbol + "3";

        int masterLongVolume = 0;
        int masterShortVolume = 0;

        auto masterLongPos = masterPositions.find(longPosKey);
        if (masterLongPos != masterPositions.end() && masterLongPos->second) {
            masterLongVolume = static_cast<int>(masterLongPos->second->position);
        }

        auto masterShortPos = masterPositions.find(shortPosKey);
        if (masterShortPos != masterPositions.end() && masterShortPos->second) {
            masterShortVolume = static_cast<int>(masterShortPos->second->position);
        }

        int masterNetPosition = masterLongVolume - masterShortVolume;

        // 6. 检查净持仓变化阈值
        int netPositionChange = std::abs(masterNetPosition - m_lastMasterNetPosition);
        if (netPositionChange < m_riskConfig.minNetPositionChange) {
            return;
        }

        // 7. 计算各账户需要的调整操作
        std::vector<AccountAdjustment> adjustments;
        calculateAccountAdjustments(masterAccountId, masterAccount, masterNetPosition, adjustments);

        if (adjustments.empty()) {
            return;
        }

        // 8. 记录跟单详情
        logFollowDetails(adjustments);

        // 9. 使用配置中的价格调整参数
        double currentPrice = last_tick.lastprice;
        double priceAdjustment = config.settings.priceAdjustmentTicks;

        if (m_contract) {
            priceAdjustment = config.settings.priceAdjustmentTicks * m_contract->priceTick;
        }

        double buyPrice = currentPrice + priceAdjustment;
        double sellPrice = currentPrice - priceAdjustment;

        // 10. 执行分组操作
        executeFollowOperations(adjustments, buyPrice, sellPrice);

        // 11. 更新跟单触发器
        updateFollowTrigger(masterNetPosition);
        m_consecutiveErrors = 0;

        m_ctaEngine->writeCtaLog("[跟单成功]" + m_symbol +
            " 主账户净持仓:" + std::to_string(masterNetPosition) +
            " 影响账户:" + std::to_string(adjustments.size()) + "个");

    }
    catch (const std::exception& e) {
        m_consecutiveErrors++;
        m_ctaEngine->writeCtaLog("[跟单异常]" + std::string(e.what()));

        if (m_consecutiveErrors >= m_riskConfig.maxConsecutiveErrors) {
            handleCircuitBreak();
        }
    }
}

void QsStrategy::calculateAccountAdjustments(const std::string& masterAccountId,
    const std::shared_ptr<Event_Account>& masterAccount,
    int masterNetPosition,
    std::vector<AccountAdjustment>& adjustments)
{
    // 检查配置更新
    checkAndUpdateConfig();

    const auto& config = m_configManager->getConfig();

    // 检查主账户是否匹配
    if (masterAccountId != config.masterAccount) {
        return; // 主账户不匹配，不跟单
    }

    // 获取启用的跟随者
    auto enabledFollowers = config.getEnabledFollowers();
    if (enabledFollowers.empty()) {
        return; // 没有跟随者，不跟单
    }

    // 🔍 添加调试日志
    std::stringstream ss;
    ss << "[跟单调试]主账户:" << config.masterAccount << " 跟随者列表:";
    for (const auto& [accountId, coefficient] : enabledFollowers) {
        ss << " " << accountId << "(" << coefficient << ")";
    }
    m_ctaEngine->writeCtaLog(ss.str());

    std::string longPosKey = m_symbol + "2";
    std::string shortPosKey = m_symbol + "3";

    adjustments.clear();
    double masterBalance = masterAccount->balance;

    // 遍历跟随账户
    for (const auto& [accountId, coefficient] : enabledFollowers) {

        // 获取子账户数据
        auto subAccount = m_ctaEngine->getAccount(accountId);
        if (!subAccount) {
            continue;
        }

        double subBalance = subAccount->balance;
        if (subBalance <= 0) {
            continue;
        }

        auto subPositions = m_ctaEngine->getPosition(accountId);

        // 获取当前持仓
        int currentLongVolume = 0;
        int currentShortVolume = 0;

        auto subLongPos = subPositions.find(longPosKey);
        if (subLongPos != subPositions.end() && subLongPos->second) {
            currentLongVolume = static_cast<int>(subLongPos->second->position);
        }

        auto subShortPos = subPositions.find(shortPosKey);
        if (subShortPos != subPositions.end() && subShortPos->second) {
            currentShortVolume = static_cast<int>(subShortPos->second->position);
        }

        // 计算当前净持仓
        int currentNetPosition = currentLongVolume - currentShortVolume;

        // 🎯 核心计算：资金比例 × 系数
        double fundRatio = subBalance / masterBalance;
        double adjustedRatio = fundRatio * coefficient;

        // 风险控制：检查调整后的跟单比例
        if (!checkFollowRatio(adjustedRatio)) {
            m_ctaEngine->writeCtaLog("[风险控制]账户" + accountId +
                " 跟单比例超限:" + std::to_string(adjustedRatio) +
                " (资金比例:" + std::to_string(fundRatio) +
                " 系数:" + std::to_string(coefficient) + ")");
            continue;
        }

        int targetNetPosition = static_cast<int>(std::floor(adjustedRatio * masterNetPosition));

        // 检查变化是否足够大
        int netPositionDiff = targetNetPosition - currentNetPosition;
        if (std::abs(netPositionDiff) < m_riskConfig.minNetPositionChange) {
            continue;
        }

        AccountAdjustment adjustment;
        adjustment.accountId = accountId;
        adjustment.fundRatio = fundRatio;
        adjustment.currentNetPosition = currentNetPosition;
        adjustment.targetNetPosition = targetNetPosition;

        // 计算具体操作
        calculateSpecificOperations(currentLongVolume, currentShortVolume,
            targetNetPosition, adjustment);

        adjustments.push_back(adjustment);

        // 记录跟单计算详情
        m_ctaEngine->writeCtaLog("[跟单计算]账户" + accountId +
            " 资金比例:" + std::to_string(fundRatio) +
            " 系数:" + std::to_string(coefficient) +
            " 最终比例:" + std::to_string(adjustedRatio) +
            " 净持仓:" + std::to_string(currentNetPosition) + "->" + std::to_string(targetNetPosition));
    }
}

void QsStrategy::calculateSpecificOperations(int currentLong, int currentShort,
    int targetNet, AccountAdjustment& adjustment)
{
    if (targetNet > 0) {
        // 目标是净多头持仓
        // 先平掉所有空头
        if (currentShort > 0) {
            adjustment.closeShortVolume = currentShort;
        }

        // 然后调整多头持仓到目标值
        int targetLong = targetNet;
        if (currentLong < targetLong) {
            adjustment.openLongVolume = targetLong - currentLong;
        }
        else if (currentLong > targetLong) {
            adjustment.closeLongVolume = currentLong - targetLong;
        }
    }
    else if (targetNet < 0) {
        // 目标是净空头持仓
        // 先平掉所有多头
        if (currentLong > 0) {
            adjustment.closeLongVolume = currentLong;
        }

        // 然后调整空头持仓到目标值
        int targetShort = -targetNet;  // 转为正数
        if (currentShort < targetShort) {
            adjustment.openShortVolume = targetShort - currentShort;
        }
        else if (currentShort > targetShort) {
            adjustment.closeShortVolume = currentShort - targetShort;
        }
    }
    else {
        // 目标净持仓为0，平掉所有持仓
        if (currentLong > 0) {
            adjustment.closeLongVolume = currentLong;
        }
        if (currentShort > 0) {
            adjustment.closeShortVolume = currentShort;
        }
    }
}

void QsStrategy::executeFollowOperations(const std::vector<AccountAdjustment>& adjustments,
    double buyPrice, double sellPrice)
{
    try {
        Strategy strategy{ m_strategyName, m_symbol, m_className };

        // 获取账户配置（使用安全方式）
        std::map<std::string, double> accountVolumes;
        try {
            accountVolumes = m_ctaEngine->getaccountConfigs().at(strategy);
        }
        catch (const std::out_of_range&) {
            m_ctaEngine->writeCtaLog("[跟单跳过]找不到策略配置: " +
                m_strategyName + "|" + m_symbol + "|" + m_className);
            return;
        }

        // 备份原始配置 - 修正类型转换
        std::map<std::string, int> originalConfig;
        for (const auto& [key, value] : accountVolumes) {
            originalConfig[key] = static_cast<int>(value);
        }

        // 1. 先执行所有平仓操作

        // 平空头
        std::map<std::string, int> closeShortNeeds;
        for (const auto& adj : adjustments) {
            if (adj.closeShortVolume > 0) {
                closeShortNeeds[adj.accountId] = adj.closeShortVolume;
            }
        }
        if (!closeShortNeeds.empty()) {
            executeOperation("买入平空", closeShortNeeds, originalConfig, strategy, buyPrice,
                [this](double price, int volume) {
                    return buycover(price, volume, false);
                });
        }

        // 平多头
        std::map<std::string, int> closeLongNeeds;
        for (const auto& adj : adjustments) {
            if (adj.closeLongVolume > 0) {
                closeLongNeeds[adj.accountId] = adj.closeLongVolume;
            }
        }
        if (!closeLongNeeds.empty()) {
            executeOperation("卖出平多", closeLongNeeds, originalConfig, strategy, sellPrice,
                [this](double price, int volume) {
                    return sell(price, volume, false);
                });
        }

        // 2. 再执行所有开仓操作

        // 开多头
        std::map<std::string, int> openLongNeeds;
        for (const auto& adj : adjustments) {
            if (adj.openLongVolume > 0) {
                openLongNeeds[adj.accountId] = adj.openLongVolume;
            }
        }
        if (!openLongNeeds.empty()) {
            executeOperation("买入开多", openLongNeeds, originalConfig, strategy, buyPrice,
                [this](double price, int volume) {
                    return buy(price, volume, false);
                });
        }

        // 开空头
        std::map<std::string, int> openShortNeeds;
        for (const auto& adj : adjustments) {
            if (adj.openShortVolume > 0) {
                openShortNeeds[adj.accountId] = adj.openShortVolume;
            }
        }
        if (!openShortNeeds.empty()) {
            executeOperation("卖出开空", openShortNeeds, originalConfig, strategy, sellPrice,
                [this](double price, int volume) {
                    return sellshort(price, volume, false);
                });
        }

        // 恢复原始配置
        for (const auto& [accountId, volume] : originalConfig) {
            m_ctaEngine->updateAccountVolume(strategy, accountId, static_cast<int>(volume));
        }

    }
    catch (const std::exception& e) {
        m_ctaEngine->writeCtaLog("[操作执行异常]" + std::string(e.what()));
        throw;
    }
}

void QsStrategy::executeOperation(const std::string& operationName,
    const std::map<std::string, int>& accountNeeds,
    const std::map<std::string, int>& originalConfig,
    const Strategy& strategy,
    double price,
    std::function<std::vector<std::string>(double, int)> orderFunction)
{
    try {
        // 获取主账户ID
        const auto& config = m_configManager->getConfig();
        std::string masterAccountId = config.masterAccount;

        // 1. 更新配置：需要操作的账户设置手数，主账户设置为0，其他设为0
        int totalVolume = 0;
        for (const auto& [accountId, volume] : originalConfig) {
            int newVolume = 0;

            if (accountId == masterAccountId) {
                // 🛡️ 主账户强制设置为0，不参与跟单交易
                newVolume = 0;
                m_ctaEngine->writeCtaLog("[跟单保护]主账户" + accountId + "设置为0手");
            }
            else {
                // 跟随账户按需设置
                auto it = accountNeeds.find(accountId);
                newVolume = (it != accountNeeds.end()) ? it->second : 0;

                // 风险控制检查
                if (newVolume > 0 && !checkRiskLimits(accountId, newVolume, price)) {
                    m_ctaEngine->writeCtaLog("[风险控制]账户" + accountId + "操作被拒绝:" + operationName);
                    newVolume = 0;
                }
            }

            m_ctaEngine->updateAccountVolume(strategy, accountId, static_cast<int>(newVolume));

            // 只有非主账户才计入总手数
            if (accountId != masterAccountId) {
                totalVolume += newVolume;
            }
        }

        if (totalVolume <= 0) {
            return;
        }

        // 2. 执行订单
        std::vector<std::string> orderIds = orderFunction(price, totalVolume);

        // 3. 更新交易计数（排除主账户）
        for (const auto& [accountId, volume] : accountNeeds) {
            if (volume > 0 && accountId != masterAccountId) {
                m_dailyTradeCount[accountId]++;
            }
        }

        // 4. 记录结果
        if (!orderIds.empty()) {
            m_ctaEngine->writeCtaLog("[跟单操作]" + operationName +
                " 总手数:" + std::to_string(totalVolume) +
                " 订单:" + std::to_string(orderIds.size()) + "个" +
                " (主账户" + masterAccountId + "已设置为0手)");
        }

    }
    catch (const std::exception& e) {
        m_ctaEngine->writeCtaLog("[操作异常]" + operationName + " " + std::string(e.what()));
        throw;
    }
}

void QsStrategy::updateFollowTrigger(int masterNetPosition)
{
    m_lastFollowTime = std::chrono::steady_clock::now();
    m_lastMasterNetPosition = masterNetPosition;
}

// ============================================================================
// 风险控制
// ============================================================================

bool QsStrategy::checkRiskLimits(const std::string& accountId, int volume, double price)
{
    // 检查日交易次数限制
    if (m_dailyTradeCount[accountId] >= m_riskConfig.maxDailyTrades) {
        return false;
    }

    // 检查单次交易资金比例
    auto account = m_ctaEngine->getAccount(accountId);
    if (account && m_contract) {
        double accountBalance = account->balance;
        double contractSize = static_cast<double>(m_contract->size);
        double tradeValue = volume * price * contractSize;
        double tradeRatio = tradeValue / accountBalance;
        if (tradeRatio > m_riskConfig.maxSingleTradeRatio) {
            return false;
        }
    }

    return true;
}

bool QsStrategy::checkFollowRatio(double ratio)
{
    return ratio >= m_riskConfig.minFollowRatio && ratio <= m_riskConfig.maxFollowRatio;
}

void QsStrategy::updateRiskMetrics()
{
    // 更新风险指标
}

void QsStrategy::handleCircuitBreak()
{
    m_followStatus = FollowStatus::CIRCUIT_BREAK;
    m_circuitBreakTime = std::chrono::steady_clock::now();
    m_ctaEngine->writeCtaLog("[熔断启动]连续错误次数达到阈值，暂停跟单 " +
        std::to_string(m_circuitBreakDuration) + " 分钟");
}

void QsStrategy::resetCircuitBreak()
{
    auto now = std::chrono::steady_clock::now();
    auto timeSinceBreak = std::chrono::duration_cast<std::chrono::minutes>(now - m_circuitBreakTime);

    if (timeSinceBreak.count() >= m_circuitBreakDuration) {
        m_followStatus = FollowStatus::NORMAL;
        m_consecutiveErrors = 0;
        m_ctaEngine->writeCtaLog("[熔断解除]跟单功能已恢复");
    }
}

// ============================================================================
// 数据一致性检查
// ============================================================================

bool QsStrategy::validateAccountData(const std::string& accountId)
{
    auto account = m_ctaEngine->getAccount(accountId);
    if (!account) return false;

    return account->balance > 0;
}

bool QsStrategy::validatePositionData(const std::map<std::string, std::shared_ptr<Event_Position>>& positions)
{
    // 检查关键持仓数据的有效性
    for (const auto& [key, position] : positions) {
        if (position) {
            if (position->position < 0) {
                return false; // 持仓数量不应该为负数
            }
        }
    }
    return true;
}

// ============================================================================
// 订单管理
// ============================================================================

void QsStrategy::checkOrdersInOnTick()
{
    std::lock_guard<std::mutex> lock(m_orderMutex);

    // 检查熔断状态
    if (m_followStatus == FollowStatus::CIRCUIT_BREAK) {
        resetCircuitBreak();
        return;
    }

    for (auto& pair : m_activeOrders) {
        auto orderEvent = pair.second;

        std::string orderID = orderEvent->orderID;
        std::string status = orderEvent->status;

        if (m_ordersPendingCancellation.find(orderID) != m_ordersPendingCancellation.end()) {
            continue;
        }

        if (status == STATUS_ALLTRADED || status == STATUS_CANCELLED) {
            continue;
        }
        else if (status == STATUS_REJECTED) {
            if (orderEvent->frontID < MAX_RETRIES) {
                orderEvent->frontID++;
                resendOrder(orderEvent);
            }
        }
        else if (status == STATUS_NOTRADED) {
            if (orderEvent->frontID < MAX_RETRIES) {
                cancelOrder(orderID, "");
                m_ordersPendingCancellation[orderID] = orderEvent;
                m_ctaEngine->writeCtaLog("请求撤单，订单ID：" + orderID);
            }
        }
        else if (status == STATUS_PARTTRADED) {
            m_ctaEngine->checkAndResendOrders(pair.first, MAX_RETRIES);
        }
    }
}

void QsStrategy::resendOrder(std::shared_ptr<Event_Order> orderEvent)
{
    double newPrice = last_tick.lastprice;

    std::string direction = orderEvent->direction;
    if (direction == DIRECTION_LONG) {
        newPrice = newPrice + 5 * (m_contract ? m_contract->priceTick : 1.0);
    }
    else if (direction == DIRECTION_SHORT) {
        newPrice = newPrice - 5 * (m_contract ? m_contract->priceTick : 1.0);
    }

    int remainingVolume = static_cast<int>(orderEvent->totalVolume - orderEvent->tradedVolume);
    std::vector<std::string> newOrderIDv = m_ctaEngine->sendOrder(false, orderEvent->symbol,
        direction, orderEvent->offset, newPrice, remainingVolume, this);

    if (!newOrderIDv.empty()) {
        std::string newOrderID = newOrderIDv[0];
        orderEvent->orderID = newOrderID;
        orderEvent->price = newPrice;
        orderEvent->status = STATUS_UNKNOWN;
        m_activeOrders[newOrderID] = orderEvent;
        m_ctaEngine->writeCtaLog("重新发送订单，新的订单ID：" + newOrderID);
    }
}

void QsStrategy::trackOrder(const std::shared_ptr<Event_Order>& orderEvent)
{
    std::lock_guard<std::mutex> lock(m_orderMutex);
    m_activeOrders[orderEvent->orderID] = orderEvent;
    m_tickCounter = 0;
}

void QsStrategy::removeOrder(const std::string& orderID)
{
    std::lock_guard<std::mutex> lock(m_orderMutex);
    m_activeOrders.erase(orderID);
    m_ordersPendingCancellation.erase(orderID);
}

void QsStrategy::clearorder()
{
    std::lock_guard<std::mutex> lock(m_orderMutex);
    m_activeOrders.clear();
    m_ordersPendingCancellation.clear();
}

// ============================================================================
// 交易函数
// ============================================================================

std::vector<std::string> QsStrategy::buy(double price, int volume, bool bStopOrder)
{
    std::vector<std::string> orderIDv;
    if (trading) {
        orderIDv = m_ctaEngine->sendOrder(bStopOrder, m_symbol, DIRECTION_LONG, OFFSET_OPEN, price, volume, this);

        for (const auto& orderID : orderIDv) {
            auto orderEvent = std::make_shared<Event_Order>();
            orderEvent->symbol = m_symbol;
            orderEvent->orderID = orderID;
            orderEvent->direction = DIRECTION_LONG;
            orderEvent->offset = OFFSET_OPEN;
            orderEvent->price = price;
            orderEvent->totalVolume = volume;
            orderEvent->tradedVolume = 0;
            orderEvent->status = STATUS_UNKNOWN;
            orderEvent->gatewayname = gatewayname;
            orderEvent->frontID = 0;
            trackOrder(orderEvent);
        }
    }
    return orderIDv;
}

std::vector<std::string> QsStrategy::sell(double price, int volume, bool bStopOrder)
{
    std::vector<std::string> orderIDv;
    if (trading) {
        orderIDv = m_ctaEngine->sendOrder(bStopOrder, m_symbol, DIRECTION_SHORT, OFFSET_CLOSE, price, volume, this);

        for (const auto& orderID : orderIDv) {
            auto orderEvent = std::make_shared<Event_Order>();
            orderEvent->symbol = m_symbol;
            orderEvent->orderID = orderID;
            orderEvent->direction = DIRECTION_SHORT;
            orderEvent->offset = OFFSET_CLOSE;
            orderEvent->price = price;
            orderEvent->totalVolume = volume;
            orderEvent->tradedVolume = 0;
            orderEvent->status = STATUS_UNKNOWN;
            orderEvent->gatewayname = gatewayname;
            orderEvent->frontID = 0;
            trackOrder(orderEvent);
        }
    }
    return orderIDv;
}

std::vector<std::string> QsStrategy::sellshort(double price, int volume, bool bStopOrder)
{
    std::vector<std::string> orderIDv;
    if (trading) {
        orderIDv = m_ctaEngine->sendOrder(bStopOrder, m_symbol, DIRECTION_SHORT, OFFSET_OPEN, price, volume, this);

        for (const auto& orderID : orderIDv) {
            auto orderEvent = std::make_shared<Event_Order>();
            orderEvent->symbol = m_symbol;
            orderEvent->orderID = orderID;
            orderEvent->direction = DIRECTION_SHORT;
            orderEvent->offset = OFFSET_OPEN;
            orderEvent->price = price;
            orderEvent->totalVolume = volume;
            orderEvent->tradedVolume = 0;
            orderEvent->status = STATUS_UNKNOWN;
            orderEvent->gatewayname = gatewayname;
            orderEvent->frontID = 0;
            trackOrder(orderEvent);
        }
    }
    return orderIDv;
}

std::vector<std::string> QsStrategy::buycover(double price, int volume, bool bStopOrder)
{
    std::vector<std::string> orderIDv;
    if (trading) {
        orderIDv = m_ctaEngine->sendOrder(bStopOrder, m_symbol, DIRECTION_LONG, OFFSET_CLOSE, price, volume, this);

        for (const auto& orderID : orderIDv) {
            auto orderEvent = std::make_shared<Event_Order>();
            orderEvent->symbol = m_symbol;
            orderEvent->orderID = orderID;
            orderEvent->direction = DIRECTION_LONG;
            orderEvent->offset = OFFSET_CLOSE;
            orderEvent->price = price;
            orderEvent->totalVolume = volume;
            orderEvent->tradedVolume = 0;
            orderEvent->status = STATUS_UNKNOWN;
            orderEvent->gatewayname = gatewayname;
            orderEvent->frontID = 0;
            trackOrder(orderEvent);
        }
    }
    return orderIDv;
}

// ============================================================================
// 事件处理
// ============================================================================

void QsStrategy::onOrder(const std::shared_ptr<Event_Order>& e)
{
    std::shared_ptr<Event_Order> orderEvent;

    {
        std::lock_guard<std::mutex> lock(m_orderMutex);
        std::string orderID = e->orderID;
        auto it = m_activeOrders.find(orderID);
        if (it != m_activeOrders.end()) {
            orderEvent = it->second;
        }
        else {
            auto cancelIt = m_ordersPendingCancellation.find(orderID);
            if (cancelIt != m_ordersPendingCancellation.end()) {
                orderEvent = cancelIt->second;
            }
            else {
                return;
            }
        }
    }

    orderEvent->status = e->status;
    orderEvent->tradedVolume = e->tradedVolume;

    if (e->status == STATUS_ALLTRADED) {
        removeOrder(e->orderID);
    }
    else if (e->status == STATUS_CANCELLED) {
        std::lock_guard<std::mutex> lock(m_orderMutex);
        m_activeOrders.erase(e->orderID);
        if (orderEvent->frontID < MAX_RETRIES) {
            orderEvent->frontID++;
            resendOrder(orderEvent);
            m_ordersPendingCancellation.erase(e->orderID);
        }
    }
}

void QsStrategy::onTrade(const std::shared_ptr<Event_Trade>& e)
{
    // 跟单策略不需要特殊的成交处理
}

void QsStrategy::onStopOrder(const std::shared_ptr<Event_StopOrder>& e)
{
    // 处理停止单事件
}

void QsStrategy::onOpener(const std::shared_ptr<Event>& e)
{
    // 处理集合竞价事件
}

void QsStrategy::onCallall(const std::shared_ptr<Event>& e)
{
    // 处理全局调用事件
}

void QsStrategy::onStarting(const std::shared_ptr<Event>& e)
{
    // 处理策略启动事件
}

// ============================================================================
// 文件处理
// ============================================================================

void QsStrategy::handleFile(std::shared_ptr<Event> e)
{
    try {
        auto fileEvent = std::static_pointer_cast<Event_File>(e);
        Strategy strategy{ m_strategyName, m_symbol, m_className };

        m_ctaEngine->restoreStrategyVolumes(strategy);

        if (!fileEvent) {
            m_ctaEngine->writeCtaLog("[错误] handleFile 中的事件类型无效");
            return;
        }

        // 使用正确的小写成员名称访问文件事件数据
        std::string fileSymbol = fileEvent->symbol;
        std::string fileStrategyName = fileEvent->strategyName;
        std::string direction = fileEvent->direction;
        int pos = fileEvent->size;
        double filePrice = fileEvent->price;

        if (getProductID(fileSymbol) != getProductID(m_symbol) || fileStrategyName != m_strategyName) {
            return;
        }

        if (!m_contract) {
            m_ctaEngine->writeCtaLog("[错误]" + m_symbol + "合约信息为空,不发送订单");
            return;
        }

        const bool hasValidTick = (last_tick.upperLimit > 0 && last_tick.lowerLimit > 0);
        const bool isPriceInRange = hasValidTick &&
            (filePrice <= last_tick.upperLimit && filePrice >= last_tick.lowerLimit);

        const double basePrice = (last_tick.lastprice > 0) ? last_tick.lastprice : filePrice;
        const double price = isPriceInRange ? filePrice : basePrice;

        double priceAdjustment = 5.0;
        if (m_contract) {
            priceAdjustment = 5 * m_contract->priceTick;
        }

        const double buyPrice = std::max(basePrice + priceAdjustment, price);
        const double sellPrice = std::min(basePrice - priceAdjustment, price);

        if (pos != 0) {
            if (!m_hasUpdatedVolume) {
                m_ctaEngine->updateStrategyVolumes(strategy, static_cast<double>(pos));
                m_hasUpdatedVolume = true;
            }

            auto strategyAccountVolumes = m_ctaEngine->getaccountConfigs();
            double totalVolume = 0;
            auto it = strategyAccountVolumes.find(strategy);
            if (it != strategyAccountVolumes.end()) {
                for (const auto& [accountId, volume] : it->second) {
                    totalVolume += volume;
                }
            }
            m_size = static_cast<int>(totalVolume);
        }

        // 执行交易指令
        if (direction == "buy2buycover") {
            buycover(buyPrice, m_size);
            buy(buyPrice, m_size);
            m_ctaEngine->writeCtaLog("[文件交易]" + m_symbol + " 平空开多: " + std::to_string(m_size) + "手");
        }
        else if (direction == "sellshort2sell") {
            sell(sellPrice, m_size);
            sellshort(sellPrice, m_size);
            m_ctaEngine->writeCtaLog("[文件交易]" + m_symbol + " 平多开空: " + std::to_string(m_size) + "手");
        }
        else if (direction == "sellshort") {
            sellshort(sellPrice, m_size);
            m_ctaEngine->writeCtaLog("[文件交易]" + m_symbol + " 开空: " + std::to_string(m_size) + "手");
        }
        else if (direction == "buycover") {
            buycover(buyPrice, m_size);
            m_ctaEngine->writeCtaLog("[文件交易]" + m_symbol + " 平空: " + std::to_string(m_size) + "手");
        }
        else if (direction == "buy") {
            buy(buyPrice, m_size);
            m_ctaEngine->writeCtaLog("[文件交易]" + m_symbol + " 开多: " + std::to_string(m_size) + "手");
        }
        else if (direction == "sell") {
            sell(sellPrice, m_size);
            m_ctaEngine->writeCtaLog("[文件交易]" + m_symbol + " 平多: " + std::to_string(m_size) + "手");
        }

        m_hasUpdatedVolume = false;
    }
    catch (const std::exception& ex) {
        m_ctaEngine->writeCtaLog("[文件处理异常]" + std::string(ex.what()));
    }
}

// ============================================================================
// 监控和日志
// ============================================================================

void QsStrategy::logFollowDetails(const std::vector<AccountAdjustment>& adjustments)
{
    std::stringstream ss;
    ss << "[跟单详情]涉及账户:" << adjustments.size() << "个\n";

    for (const auto& adj : adjustments) {
        // 获取账户的系数信息
        double coefficient = m_configManager->getConfig().getCoefficient(adj.accountId);

        ss << "  账户:" << adj.accountId
            << " 资金比例:" << std::fixed << std::setprecision(3) << adj.fundRatio
            << " 系数:" << coefficient
            << " 调整比例:" << (adj.fundRatio * coefficient)
            << " 净持仓:" << adj.currentNetPosition << "->" << adj.targetNetPosition;

        if (adj.closeShortVolume > 0) ss << " 平空:" << adj.closeShortVolume;
        if (adj.closeLongVolume > 0) ss << " 平多:" << adj.closeLongVolume;
        if (adj.openLongVolume > 0) ss << " 开多:" << adj.openLongVolume;
        if (adj.openShortVolume > 0) ss << " 开空:" << adj.openShortVolume;
        ss << "\n";
    }

    m_ctaEngine->writeCtaLog(ss.str());
}

void QsStrategy::logRiskMetrics()
{
    std::stringstream ss;
    ss << "[风险指标]跟单状态:" << (int)m_followStatus
        << " 连续错误:" << m_consecutiveErrors
        << " 日交易计数:" << m_dailyTradeCount.size() << "个账户";

    m_ctaEngine->writeCtaLog(ss.str());
}

void QsStrategy::logFollowConfig()
{
    const auto& config = m_configManager->getConfig();
    auto enabledFollowers = config.getEnabledFollowers();

    std::stringstream ss;
    ss << "[跟单配置]" << m_symbol << ":\n";
    ss << "  主账户: " << config.masterAccount << "\n";
    ss << "  跟随账户: " << enabledFollowers.size() << "个\n";

    for (const auto& [accountId, coefficient] : enabledFollowers) {
        ss << "    " << accountId
            << " - 系数: " << coefficient << "\n";
    }

    if (enabledFollowers.empty()) {
        ss << "    (无启用的跟随者)\n";
    }

    m_ctaEngine->writeCtaLog(ss.str());
}

// ============================================================================
// 配置管理
// ============================================================================

void QsStrategy::loadRiskConfigFromFile()
{
    const auto& config = m_configManager->getConfig();
    const auto& settings = config.settings;

    // 转换为旧的风险配置结构
    m_riskConfig.maxFollowRatio = settings.maxFollowRatio;
    m_riskConfig.minFollowRatio = settings.minFollowRatio;
    m_riskConfig.maxDailyTrades = 1000; // 使用默认值
    m_riskConfig.maxConsecutiveErrors = settings.maxConsecutiveErrors;
    m_riskConfig.maxSingleTradeRatio = 0.5; // 使用默认值
    m_riskConfig.minNetPositionChange = settings.minPositionChange;

    m_circuitBreakDuration = settings.circuitBreakDuration;
}

void QsStrategy::checkAndUpdateConfig()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastConfigCheck);

    // 每30秒检查一次配置更新
    if (elapsed.count() >= 30) {
        if (m_configManager->reloadIfModified()) {
            loadRiskConfigFromFile();
            m_ctaEngine->writeCtaLog("[配置更新]" + m_symbol + " 跟单配置已重新加载");
            logFollowConfig();
        }
        m_lastConfigCheck = now;
    }
}

// ============================================================================
// 动态配置接口
// ============================================================================

bool QsStrategy::setAccountCoefficient(const std::string& accountId, double coefficient)
{
    bool result = m_configManager->setCoefficient(accountId, coefficient);
    if (result) {
        if (coefficient > 0) {
            m_ctaEngine->writeCtaLog("[系数设置]账户" + accountId +
                " 系数:" + std::to_string(coefficient));
        }
        else {
            m_ctaEngine->writeCtaLog("[禁用跟单]账户" + accountId);
        }
        logFollowConfig();
    }
    return result;
}

bool QsStrategy::disableAccount(const std::string& accountId)
{
    return setAccountCoefficient(accountId, 0.0);
}

bool QsStrategy::enableAccount(const std::string& accountId, double coefficient)
{
    return setAccountCoefficient(accountId, coefficient);
}

bool QsStrategy::setMasterAccount(const std::string& accountId)
{
    bool result = m_configManager->setMasterAccount(accountId);
    if (result) {
        m_ctaEngine->writeCtaLog("[主账户变更]新主账户:" + accountId);
        logFollowConfig();
    }
    return result;
}

std::string QsStrategy::getMasterAccountId() const
{
    return m_configManager->getConfig().masterAccount;
}

bool QsStrategy::isStrategyEnabled() const
{
    return !m_configManager->getConfig().getEnabledFollowers().empty();
}

std::string QsStrategy::getConfigSummary() const
{
    return m_configManager->getConfigSummary();
}

// ============================================================================
// 辅助函数
// ============================================================================

void QsStrategy::setupBarGenerator()
{
    // 定义两个函数对象，用于处理不同的bar数据
    ON_Functional on_func1, on_fun2;
    on_func1 = std::bind(&QsStrategy::onBar, this, std::placeholders::_1);
    on_fun2 = std::bind(&QsStrategy::on_5min_bar, this, std::placeholders::_1);

    // 创建BarGenerator和ArrayManager对象
    m_BarGenerate = std::make_unique<BarGenerator>(on_func1, 5, on_fun2, MINUTE);
    m_ArrayManager = std::make_unique<ArrayManager>(350);
}

void QsStrategy::registerEvents()
{
    // 注册事件处理函数
    m_ctaEngine->RegEvent(EVENT_OPENER, [this](const std::shared_ptr<Event>& e) {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        onOpener(e);
    });
    m_ctaEngine->RegEvent(EVENT_CALL, [this](const std::shared_ptr<Event>& e) {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        onCallall(e);
    });
    m_ctaEngine->RegEvent(EVENT_START, [this](const std::shared_ptr<Event>& e) {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        onStarting(e);
    });
    m_ctaEngine->RegEvent(EVENT_FILE, [this](const std::shared_ptr<Event>& e) {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        handleFile(e);
    });
}

void QsStrategy::initializeStrategy()
{
    // 初始化策略数据
    m_strategydata->insertvar("pos", "0");

    // 初始化时间戳
    m_lastFollowTime = std::chrono::steady_clock::now();
    m_lastConfigCheck = std::chrono::steady_clock::now();
}

void QsStrategy::resetDailyCounters()
{
    m_dailyTradeCount.clear();
    m_ctaEngine->writeCtaLog("[日重置]交易计数器已重置");
}

double QsStrategy::calculateAvailableMargin(const std::string& accountId)
{
    auto account = m_ctaEngine->getAccount(accountId);
    if (account) {
        return account->available;
    }
    return 0.0;
}

RiskControlConfig QsStrategy::convertToRiskConfig(const GlobalSettings& globalSettings)
{
    RiskControlConfig riskConfig;
    riskConfig.maxFollowRatio = globalSettings.maxFollowRatio;
    riskConfig.minFollowRatio = globalSettings.minFollowRatio;
    riskConfig.maxDailyTrades = 1000;
    riskConfig.maxConsecutiveErrors = globalSettings.maxConsecutiveErrors;
    riskConfig.maxSingleTradeRatio = 0.5;
    riskConfig.minNetPositionChange = globalSettings.minPositionChange;
    return riskConfig;
}

// ============================================================================
// 导出函数
// ============================================================================

extern "C" __declspec(dllexport) StrategyTemplate* createStrategy(ICTAEngine* engine, const char* name, const char* symbol, const char* className)
{
    return new QsStrategy(engine, name, symbol, className);
}
