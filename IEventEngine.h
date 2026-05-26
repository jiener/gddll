#pragma once
// IEventEngine.h
#ifndef IEVENTENGINE_H
#define IEVENTENGINE_H

#include <string>
#include <functional>
#include <memory>

// 前向声明 Event 类
class Event;

// 定义任务类型，即处理事件的函数
typedef std::function<void(std::shared_ptr<Event>)> TASK;

// 事件引擎接口
class IEventEngine {
public:
    virtual ~IEventEngine() = default;

    // 启动事件引擎
    virtual void StartEngine() = 0;

    // 停止事件引擎
    virtual void StopEngine() = 0;

    // 注册事件处理函数
    virtual void RegEvent(std::string eventType, TASK task) = 0;

    // 注销事件处理函数
    virtual void UnregEvent(std::string eventType) = 0;

    // 投递事件
    virtual void Put(std::shared_ptr<Event> e) = 0;
};

#endif // IEVENTENGINE_H
