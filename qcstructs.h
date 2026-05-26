#ifndef	QCSTRUCTS_H
#define QCSTRUCTS_H

#include<string>
#include<iostream>
#include<vector>
#include<mutex>
#include<map>
#include"utils.hpp"
//#include<msgpack.hpp>
/*************************************/
#define PRICETYPE_LIMITPRICE "pricetypelimit"
#define PRICETYPE_MARKETPRICE "pricetypemarket"
#define PRICETYPE_FAK "FAK"
#define PRICETYPE_FOK "FOK"
#define PRICETYPE_STOP "STOP"
#define PRICETYPE_REQUEST "REQUEST_PRICE"

#define DIRECTION_NONE "directionnone"
#define DIRECTION_LONG "多"
#define DIRECTION_SHORT "空"
#define DIRECTION_UNKNOWN "未知"
#define DIRECTION_NET "directionnet"

#define OFFSET_NONE "offsetnone"
#define OFFSET_OPEN "开"
#define OFFSET_CLOSE "平"
#define OFFSET_CLOSETODAY "平今"
#define OFFSET_CLOSEYESTERDAY "平昨"
#define OFFSET_UNKNOWN "offsetunknow"

#define EXCHANGE_SHFE "SHFE"  //上海期货交易所
#define EXCHANGE_INE "INE"    //上海国际能源交易中心
#define EXCHANGE_CFFEX "CFFEX"  

#define STATUS_ALLTRADED "全成交"
#define STATUS_PARTTRADED "部分成交"
#define STATUS_NOTRADED "未成交"

#define STATUS_CANCELLED "已撤消"
#define STATUS_REJECTED "已拒单"
#define STATUS_WAITING "orderwaiting"
#define STATUS_SUBMITTING "提交中"
#define STATUS_TRIGGED "ordertrigged"
#define STATUS_REGECTED "orderrejected"
#define STATUS_UNKNOWN "未知"


#define BAR_MODE "barmode"
#define TICK_MODE "tickmode"

typedef enum
{
	MINUTE = 0,
	HOUR = 1,
	DAILY = 2,
	WEEKLY = 3,
	TICK = 4
}Interval;

enum Mode
{
	RealMode = 0,
	BacktestMode = 1
};

struct SubscribeReq
{
	std::string symbol;
	std::string exchange;
	std::string productClass;
	std::string currency;
	std::string expiry;
	double strikePrice;
	std::string optionType;

	/*MSGPACK_DEFINE(symbol, exchange, productClass, currency, expiry, strikePrice, optionType);*/
};

struct OrderReq
{
	std::string symbol;
	std::string exchange;
	double price;
	double volume;
	std::string priceType;
	std::string direction;
	std::string offset;
	std::string productClass;
	std::string currency;
	std::string expiry;
	double strikePrice;
	std::string optionType;
	std::string strateyName;

	//MSGPACK_DEFINE(symbol, exchange, price, volume, priceType, direction, offset, productClass, currency, expiry, strikePrice, optionType, strateyName);
};

struct StopOrderReq
{
	std::string symbol;
	std::string exchange;
	std::string  strategyName;
	std::string orderID;

	double price;
	double volume;
	std::string priceType;
	std::string direction;
	std::string offset;
	std::string productClass;
	std::string currency;
	std::string expiry;
	double strikePrice;
	std::string optionType;
	std::string strateyName;
};

struct CancelOrderReq
{
	std::string symbol;
	std::string exchange;
	std::string orderID;
	std::string orderSysID;
	//std::string frontID;
	//std::string sessionID;

	//MSGPACK_DEFINE(symbol, exchange, orderID, frontID, sessionID);

};

struct Portfolio_Result_Data
{

	double	maxCapital;
	double	drawdown;
	double	Winning;	//平仓全部盈利
	double	Losing;		//平仓全部亏损
	double  totalwinning; //平仓净盈亏  winning-losing
	double  lastdayTotalwinning;
	double  delta;
	int totalResult;

	double holdingwinning;//持仓盈亏
	double holdingposition;
	double holdingprice;

	double holding_and_totalwinning;	//totalwinning+holdingwinning

	double portfolio_winning;			//所有策略 合约的盈利
	//MSGPACK_DEFINE(holdingprice);
};

class BaseData  //基类
{
public:
	BaseData(std::string type) :m_datatype(type)
	{}
	std::string GetDataType()
	{
		return m_datatype;
	}
private:
	std::string m_datatype;
};

class TickData : public BaseData {
public:
	int getMinute() const {
		if (time.length() >= 8 && time[2] == ':' && time[5] == ':') {
			return std::stoi(time.substr(3, 2));
		}
		return -1;  // 返回 -1 表示无效时间
	}
	int getHour() const {
		if (time.length() >= 8 && time[2] == ':' && time[5] == ':') {
			return std::stoi(time.substr(0, 2));
		}
		return -1;  // 返回 -1 表示无效时间
	}

	TickData() : BaseData("tick"),
		lastprice(0.0),
		volume(0.0),
		openInterest(0.0),
		openPrice(0.0),
		highPrice(0.0),
		lowPrice(0.0),
		preClosePrice(0.0),
		upperLimit(0.0),
		lowerLimit(0.0),
		bidprice1(0.0),
		bidprice2(0.0),
		bidprice3(0.0),
		bidprice4(0.0),
		bidprice5(0.0),
		askprice1(0.0),
		askprice2(0.0),
		askprice3(0.0),
		askprice4(0.0),
		askprice5(0.0),
		bidvolume1(0.0),
		bidvolume2(0.0),
		bidvolume3(0.0),
		bidvolume4(0.0),
		bidvolume5(0.0),
		askvolume1(0.0),
		askvolume2(0.0),
		askvolume3(0.0),
		askvolume4(0.0),
		askvolume5(0.0)
	{}

	std::string symbol;
	std::string exchange;
	std::string gatewayname;
	double lastprice;
	double volume;
	double openInterest;
	std::string datetime;
	std::string date;
	std::string time;
	double openPrice;
	double highPrice;
	double lowPrice;
	double preClosePrice;
	double upperLimit;
	double lowerLimit;
	double bidprice1;
	double bidprice2;
	double bidprice3;
	double bidprice4;
	double bidprice5;
	double askprice1;
	double askprice2;
	double askprice3;
	double askprice4;
	double askprice5;
	double bidvolume1;
	double bidvolume2;
	double bidvolume3;
	double bidvolume4;
	double bidvolume5;
	double askvolume1;
	double askvolume2;
	double askvolume3;
	double askvolume4;
	double askvolume5;
};

class BarData : public BaseData {
public:
	int getMinute() const {  // 改为大写开头
		if (time.length() >= 8 && time[2] == ':' && time[5] == ':') {
			return std::stoi(time.substr(3, 2));
		}
		return -1;  // 返回 -1 表示无效时间
	}
	int getHour() const {    // 改为大写开头
		if (time.length() >= 8 && time[2] == ':' && time[5] == ':') {
			return std::stoi(time.substr(0, 2));
		}
		return -1;  // 返回 -1 表示无效时间
	}

	BarData() : BaseData("bar"),
		open(0.0),
		high(0.0),
		low(0.0),           // 改为0.0初始化
		close(0.0),
		openPrice(0.0),
		highPrice(0.0),
		lowPrice(0.0),      // 改为0.0初始化
		preClosePrice(0.0),
		upperLimit(0.0),
		lowerLimit(0.0),
		volume(0.0),
		openInterest(0.0)
	{}

	std::string symbol;
	std::string exchange;
	double open;
	double high;
	double low;
	double close;
	std::string date;
	std::string time;
	std::string datetime;
	double openPrice;      // 今日开
	double highPrice;      // 今日高
	double lowPrice;       // 今日低
	double preClosePrice;  // 昨收
	double upperLimit;     // 涨停
	double lowerLimit;     // 跌停
	double volume;
	double openInterest;
	Interval interval;     // 表示是分钟，小时，还是天
};

class DailyBar : public BaseData {
public:
	int getMinute() const {  // 改为大写开头
		if (time.length() >= 8 && time[2] == ':' && time[5] == ':') {
			return std::stoi(time.substr(3, 2));
		}
		return -1;  // 返回 -1 表示无效时间
	}
	int getHour() const {    // 改为大写开头
		if (time.length() >= 8 && time[2] == ':' && time[5] == ':') {
			return std::stoi(time.substr(0, 2));
		}
		return -1;  // 返回 -1 表示无效时间
	}

	DailyBar() : BaseData("dailybar"),
		open(0.0),
		high(0.0),
		low(0.0),
		close(0.0),
		volume(0.0),
		openInterest(0.0),
		unixdatetime(0)    // 这个是long long类型，用0初始化
	{}

	std::string symbol;
	std::string exchange;
	double open;
	double high;
	double low;
	double close;
	std::string date;
	std::string time;
	long long unixdatetime;  // 毫秒级UNIX时间戳
	double volume;
	double openInterest;
};

/*********************************事件宏定义**********************************************/
#define EVENT_FILE "eFile"
#define EVENT_DFILE "eDFile"
#define EVENT_QUIT "equit"//退出事件
#define EVENT_TIMER "etimer"//循环1秒钟事件，用来不断的查持仓，防止CTP流控
#define EVENT_OPENER "eOpener"
#define EVENT_CALL "eCall"
#define EVENT_START "eStart"
#define EVENT_CLEAR "eClear"
#define EVENT_MONEY "eMoney"
#define EVENT_POST "ePost"
#define EVENT_TICK "etick"
#define EVENT_TRADE "etrade"
#define EVENT_ORDER "eorder"
#define EVENT_STOP_ORDER "stoporder"
#define EVENT_POSITION "ePosition"
#define EVENT_ACCOUNT "eAccount"
#define EVENT_CONTRACT "eContract"
#define EVENT_ERROR "eError"
#define EVENT_LOG "elog"
#define EVENT_LOADSTRATEGY "eloadstrategy"
#define EVENT_UPDATESTRATEGY "eupdatestrategy"
#define EVENT_UPDATEPORTFOLIO "eupdateportfolio"
//回测事件
#define EVENT_BACKTEST_TICK "ebacktesttick"
#define EVENT_BACKTEST_BAR "ebacktestbar"
#define EVENT_CTABACKTESTERFINISHED "backtessterfinished"


//中间件事件

#define EVENT_MULTI_ACCOUNT_ORDER "eMultiAccountOrder"
#define EVENT_MULTI_ACCOUNT_ORDER_REPORT "eMultiAccountOrderReport"
#define EVENT_MULTI_ACCOUNT_TRADE_REPORT "eMultiAccountTradeReport"

///////////////////////////////////////


/**********************************事件类型*****************************************/
class  Event //定义事件基类
{
public:
	Event(std::string type) :m_eventtype(type)
	{}
	std::string GetEventType()
	{
		return m_eventtype;
	}
public:
	std::string m_eventtype;

	//MSGPACK_DEFINE(m_eventtype);
};
class  Event_TesterFinished :public Event
{
public:
	Event_TesterFinished() :Event(EVENT_CTABACKTESTERFINISHED)
	{}
};

class  Event_Exit :public Event
{
public:
	Event_Exit() :Event(EVENT_QUIT)
	{}
};

class   Event_Timer :public Event
{
public:
	Event_Timer() :Event(EVENT_TIMER)
	{}
};

class   Event_Opener :public Event
{
public:
	Event_Opener() :Event(EVENT_OPENER)
	{}
};

class   Event_Call :public Event
{
public:
	Event_Call() :Event(EVENT_CALL)
	{}
};

class   Event_File :public Event
{
public:
	Event_File() : Event(EVENT_FILE)
	{}

	//代码
	std::string strategyName; //策略名
	std::string symbol; //合约名
	std::string direction; //方向
	int size; //头寸
	double price; //价格
	std::string time; //时间

};
class   Event_DFile :public Event
{
public:
	Event_DFile() : Event(EVENT_DFILE)
	{}

	//代码
	std::string filepath; //文件路径
	std::string symbol;

};

class   Event_Start :public Event
{
public:
	Event_Start() :Event(EVENT_START)
	{}
};

class   Event_Clear :public Event
{
public:
	Event_Clear() :Event(EVENT_CLEAR)
	{}
};
class   Event_Money :public Event
{
public:
	Event_Money() :Event(EVENT_MONEY)
	{}
};

class   Event_Post :public Event
{
public:
	Event_Post() :Event(EVENT_POST)
	{}
	std::string gatewayname;
};
class   Event_Tick :public Event
{
public:
	Event_Tick(): Event(EVENT_TICK)
	{}
	//MSGPACK_DEFINE(symbol, exchange, gatewayname, lastprice, volume, openInterest, date, time);

	//代码
	std::string symbol;
	std::string exchange;
	std::string gatewayname;
	//成交数据
	double lastprice;//最新成交价
	double volume;//总成交量
	double openInterest;//持仓量
	std::string date;//日期
	std::string time;//时间
	//常规行情
	double openPrice;//今日开
	double highPrice;//今日高
	double lowPrice;//今日低
	double preClosePrice;//昨收

	double upperLimit;//涨停
	double lowerLimit;//跌停
	//五档行情
	double bidprice1;
	double bidprice2;
	double bidprice3;
	double bidprice4;
	double bidprice5;

	double askprice1;
	double askprice2;
	double askprice3;
	double askprice4;
	double askprice5;

	double bidvolume1;
	double bidvolume2;
	double bidvolume3;
	double bidvolume4;
	double bidvolume5;

	double askvolume1;
	double askvolume2;
	double askvolume3;
	double askvolume4;
	double askvolume5;

};

class   Event_Trade :public Event
{
public:
	Event_Trade() :Event(EVENT_TRADE)
	{}
	//MSGPACK_DEFINE(symbol, exchange, tradeID, orderID, gatewayname, direction, offset, price, volume, tradeTime, datetime);
	//代码编号
	std::string symbol;
	std::string exchange;
	std::string tradeID;   //交易编号
	std::string orderID;  //本地订单编号
	std::string orderSysID; //系统订单编号
	std::string gatewayname;
	//成交相关
	std::string direction;//方向
	std::string offset; //成交开平仓
	double price;//成交价格
	double volume;//成交量
	std::string tradeTime;//成交日期
	std::string datetime;//成交具体时间
};

class   Event_Order :public Event
{
public:
	Event_Order() :Event(EVENT_ORDER)
	{}
	//MSGPACK_DEFINE(symbol, exchange,  orderID, gatewayname, direction, offset, price, totalVolume, tradedVolume, orderTime, status, cancelTime, frontID, sessionID);
	//编号相关
	std::string symbol;
	std::string exchange;
	std::string orderID;//本地订单编号
	std::string orderSysID; //系统订单编号
	std::string gatewayname;
	//报单相关
	std::string direction;//方向
	std::string offset;//开平方向
	double price; //报单价格
	double totalVolume;//报单总量
	double tradedVolume;//成交数量
	std::string status;//报单状态

	std::string orderTime;//发单时间
	std::string cancelTime;//撤单时间

	int frontID;//前置机编号
	int sessionID;//连接编号
};
class   Event_StopOrder :public Event
{
public:
	Event_StopOrder() :Event(EVENT_STOP_ORDER)
	{}
	//MSGPACK_DEFINE(symbol, exchange, orderID, gatewayname, direction, offset, price,
		//totalVolume, tradedVolume, orderTime, status, cancelTime, frontID, sessionID, strategyName);

	//编号相关
	std::string symbol;
	std::string exchange;
	std::string orderID;//订单编号
	std::string gatewayname;
	//报单相关
	std::string direction;//方向
	std::string offset;//开平方向
	double price; //报单价格
	double totalVolume;//报单总量
	double tradedVolume;//成交数量
	std::string status;//报单状态

	std::string orderTime;//发单时间
	std::string cancelTime;//撤单时间

	int frontID;//前置机编号
	int sessionID;//连接编号
	std::string strategyName;
	std::string className;

};

class   Event_Contract :public Event
{
public:
	Event_Contract() :Event(EVENT_CONTRACT)
	{}
	//MSGPACK_DEFINE(symbol, exchange, name, gatewayname, productClass, size, priceTick,
		//strikePrice, underlyingSymbol, optionType);

	std::string symbol;
	std::string exchange;
	std::string name;                   //合约中文名
	std::string gatewayname;
	std::string productClass;           //合约类型
	int size;                           //合约大小
	int days;                           //合约到期天数
	double priceTick;					//合约最小价格Tick

	double strikePrice;					//行权价
	std::string underlyingSymbol;       //标的物合约代码
	std::string optionType;				//期权类型
};

class   Event_Position :public Event
{
public:
	Event_Position() :Event(EVENT_POSITION)
	{
		position = 0;
		todayPosition = 0;
		ydPosition = 0;
		todayPositionCost = 0;
		ydPositionCost = 0;
		price = 0;
		frozen = 0;
	}
	//MSGPACK_DEFINE(symbol, direction, position, gatewayname, todayPosition, ydPosition, todayPositionCost,
		//ydPositionCost, price, frozen);

	std::string symbol;
	std::string direction;
	std::string gatewayname;
	double position;
	double todayPosition;
	double ydPosition;
	double todayPositionCost;
	double ydPositionCost;
	double price;
	double frozen;
	double CloseProfitByTrade;
};

class   Event_Account :public Event
{
public:
	Event_Account() :Event(EVENT_ACCOUNT)
	{}
	//MSGPACK_DEFINE(gatewayname, accountid, preBalance, balance,
		//available, commission, margin, closeProfit, positionProfit);

	std::string gatewayname;
	std::string accountid;
	double preBalance;//昨日账户结算净值
	double balance;//账户净值
	double available;//可用资金
	double commission;//今日手续费
	double margin;//保证金占用
	double closeProfit;//平仓盈亏
	double positionProfit;//持仓盈亏
};

class   Event_Error :public Event
{
public:
	Event_Error() :Event(EVENT_ERROR)
	{}
	//MSGPACK_DEFINE(errorID, errorMsg, additionalInfo, gatewayname,
		//errorTime);

	std::string errorID;//错误代码
	std::string errorMsg;//错误信息
	std::string additionalInfo;//附加信息
	std::string gatewayname;
	std::string errorTime = Utils::getCurrentSystemTime();
};

class   Event_Log :public Event
{
public:
	Event_Log() :Event(EVENT_LOG)
	{}
	//MSGPACK_DEFINE(msg, gatewayname,
		//logTime);

	std::string msg;//log信息
	std::string gatewayname;
	std::string logTime = Utils::getCurrentSystemTime();
	std::string account;
};

class   Event_UpdateStrategy :public Event
{
public:
	Event_UpdateStrategy() :Event(EVENT_UPDATESTRATEGY)
	{}
	//MSGPACK_DEFINE(strategyname, parammap,
		//varmap);

	std::string strategyname;
	bool inited;
	bool trading;
	std::map<std::string, std::string>parammap;
	std::map<std::string, std::string>varmap;
};

class   Event_UpdatePortfolio :public Event
{
public:
	Event_UpdatePortfolio() :Event(EVENT_UPDATEPORTFOLIO)
	{}
	//MSGPACK_DEFINE(strategyname, symbol,
		//datetime, Portfoliodata, strategyrows);

	std::string strategyname;
	std::string symbol;
	time_t datetime;
	Portfolio_Result_Data Portfoliodata;
	std::vector<int>strategyrows;
};

class   Event_LoadStrategy :public Event
{
public:
	Event_LoadStrategy() :Event(EVENT_LOADSTRATEGY)
	{}
	//MSGPACK_DEFINE(strategyname, parammap,
		//varmap);

	std::string strategyname;
	std::map<std::string, std::string>parammap;
	std::map<std::string, std::string>varmap;
};

class   Event_Backtest_Tick :public Event
{
public:
	Event_Backtest_Tick() :Event(EVENT_BACKTEST_TICK)
	{}
	TickData tick;
};

class   Event_Backtest_Bar :public Event
{
public:
	Event_Backtest_Bar() :Event(EVENT_BACKTEST_BAR)
	{}
	BarData bar;
};

//中间件事件定义
class Event_MultiAccountOrder : public Event
{
public:
	Event_MultiAccountOrder() : Event(EVENT_MULTI_ACCOUNT_ORDER) {}

	OrderReq orderReq;
	std::string strategyName;
	std::string className;

};

class Event_MultiAccountOrderReport : public Event
{
public:
	Event_MultiAccountOrderReport() : Event(EVENT_MULTI_ACCOUNT_ORDER_REPORT) {}

	std::shared_ptr<Event_Order> order;
	std::string virtualOrderID;
	std::string strategyName;
};

class Event_MultiAccountTradeReport : public Event
{
public:
	Event_MultiAccountTradeReport() : Event(EVENT_MULTI_ACCOUNT_TRADE_REPORT) {}

	std::shared_ptr<Event_Trade> trade;
	std::string virtualOrderID;
	std::string strategyName;
	std::string account;
};
#endif