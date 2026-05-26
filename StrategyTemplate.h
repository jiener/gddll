#ifndef STRATEGYTEMPLATE_H
#define STRATEGYTEMPLATE_H   

// 添加警告控制
#pragma warning(push)
#pragma warning(disable: 4251)

#include <map>
#include <set>
#include <memory>
#include <atomic>
#include <vector>
#include <mutex>
#include <string>
#include "../qcstructs.h"
#include "ICTAEngine.h"
#include "ICtaEngine.h"
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#ifdef STRATEGYTEMPLATE_EXPORTS
#define STRATEGYTEMPLATE_API __declspec(dllexport)
#else
#define STRATEGYTEMPLATE_API __declspec(dllimport)
#endif

class StrategyData
{
public:
    void insertparam(const std::string& key, const std::string& value);                    // 插入参数
    void insertvar(const std::string& key, const std::string& value);                      // 插入变量
    void setparam(const std::string& key, const std::string& value);                       // 设置参数的值
    void setvar(const std::string& key, const std::string& value);                         // 设置变量的值
    std::string getparam(const std::string& key) const;                                    // 获取参数
    std::string getvar(const std::string& key) const;                                      // 获取变量
    std::map<std::string, std::string> getallparam() const;                                // 获取所有参数
    std::map<std::string, std::string> getallvar() const;                                  // 获取所有变量
private:
    mutable std::mutex m_mtx;                                                              // 变量锁，mutable 允许在 const 函数中修改
    // 下面两个表的值都是 string 格式，在写策略的时候一般参数是数值，所以需要自己做一下转换
    std::map<std::string, std::string> m_parammap;                                         // 参数列表 
    std::map<std::string, std::string> m_varmap;                                           // 变量列表
};

class __declspec(dllexport) StrategyTemplate
{
public:
    StrategyTemplate(ICTAEngine* ctaEngine, const std::string& strategyName, const std::string& symbol, const std::string& className);
    virtual ~StrategyTemplate();

    /******************************策略参数和变量*********************************************/
    // 基本参数
    std::string gatewayname;                // 网关名称，例如 "CTP"

    // 基础变量
    std::string m_symbol;                   // 交易的合约
    std::string m_exchange;                 // 合约交易所
    std::string m_strategyName;             // 策略名称
    std::string m_className;                // 策略类名

    bool inited;                            // 初始化控制
    bool trading;                           // 交易控制
    int m_Pos;                              // 仓位
    int initDays;                           // 加载历史数据的天数
    std::shared_ptr<Event_Contract> m_contract;
    int m_size;
    double m_barclose;

    /******************************CTAMANAGER控制策略***********************************************/
    // 初始化
    virtual void onInit();
    // 开始 
    virtual void onStart();
    // 停止
    virtual void onStop();
    // 需要具体的策略函数实现，将配置文件中取得的值赋值给策略中具体的变量
    virtual void updateSetting();

    // 给参数赋值
    void checkparam(const std::string& paramname, const std::string& paramvalue);

    // 更新参数和变量的值
    void updateParam(const std::string& paramname, const std::string& paramvalue);
    void updateVar(const std::string& paramname, const std::string& paramvalue);

    // 给 pos 赋值
    void checkSymbol(const std::string& symbolname);

    // 获取参数的值
    std::string getparam(const std::string& paramname) const;

    // 加载策略数据到界面
    void putGUI();

    // 更新参数到界面
    virtual void putEvent();

    // tradeevent 更新持仓
    void setPos(int pos);
    void changeposmap(const std::string& symbol, double pos);

    // 将变量数据保存到 json 文件，一般在停止策略，交易单成功，退出时保存
    void sync_data();

    /*******************************实现策略主要函数**************************************************/
    // TICK
    virtual void onTick(const TickData& Tick);
    // BAR
    virtual void onBar(const BarData& Bar);
    // 报单回调
    virtual void onOrder(const std::shared_ptr<Event_Order>& e);
    // 成交回调
    virtual void onTrade(const std::shared_ptr<Event_Trade>& e);
    // 停止单回调
    virtual void onStopOrder(const std::shared_ptr<Event_StopOrder>& e);

    // 发单函数
    // 做多，bStopOrder 表示停止单还是限价单，CTP 不支持停止单，实现了本地停止单的功能
    virtual std::vector<std::string> buy(double price, int volume, bool bStopOrder = false);
    // 平多
    virtual std::vector<std::string> sell(double price, int volume, bool bStopOrder = false);
    // 做空
    virtual std::vector<std::string> sellshort(double price, int volume, bool bStopOrder = false);
    // 平空
    virtual std::vector<std::string> buycover(double price, int volume, bool bStopOrder = false);

    // 总报单开平函数公用
    std::vector<std::string> sendOrder(bool bStopOrder, const std::string& strDirection, const std::string& strOffset, double price, double volume);

    // 撤所有单，停止策略时使用
    void cancelAllOrder();
    // 撤单函数
    void cancelOrder(const std::string& orderID, const std::string& gatewayname);

    // 获取 m_Pos
    int getpos() const;

public:
    // 读取历史数据
    std::vector<TickData> loadTick(const std::string& symbol, int days);
    std::vector<BarData> loadBar(const std::string& symbol, int days);

    // CTA 管理器
    ICTAEngine* m_ctaEngine;

    // Strategy instance callback lock. Must match the host StrategyTemplate layout.
    std::mutex m_callbackMutex;

    // Bar 推送
    int m_minute;
    int m_hour;
    BarData m_bar;

    // 加载策略数据到界面
    void updateGUI(std::shared_ptr<Event> e);

    // 参数和变量数据
    std::unique_ptr<StrategyData> m_strategydata;

    // 持仓
    std::map<std::string, double> m_pos_map;
    std::mutex m_Pos_mapmtx;

    // 订单列表
    std::vector<std::string> m_orderlist;
    std::mutex m_orderlistmtx;

    // 成交列表
    std::vector<Event_Trade> m_tradelist;
};

#pragma warning(pop)
#endif // STRATEGYTEMPLATE_H
