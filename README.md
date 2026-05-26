# gddll

`gddll` 是给 `futuretrade` 主程序加载的跟单策略 DLL。策略入口为 `createStrategy`，实际策略类为 `QsStrategy`，继承 `StrategyTemplate` 并通过 `ICTAEngine` 调用主程序的行情、账户、持仓、下单、撤单和事件接口。

## 功能

- 按主账户净持仓变化计算跟随账户目标持仓。
- 根据跟随账户资金比例和配置系数计算跟单手数。
- 自动处理开多、平多、开空、平空调整。
- 支持 `Strategy/follow_config.json` 动态配置热加载。
- 提供订单跟踪、撤单后重发、连续错误熔断和基础风险限制。
- 已同步 `futuretrade` 新版策略模板的单策略实例回调锁 `m_callbackMutex`，兼容主程序并发回调模型。

## 目录

- `gddll.sln`：Visual Studio 解决方案。
- `gddll.vcxproj`：DLL 工程文件。
- `QsStrategy.h/.cpp`：跟单策略主体。
- `StrategyTemplate.h/.cpp`：策略基类，需与主程序侧 ABI 保持一致。
- `ICTAEngine.h`、`IEventEngine.h`、`qcstructs.h`：主程序和策略 DLL 之间的接口与数据结构。
- `tools.*`：K 线、数组管理和 TA-Lib 辅助工具。
- `include/`、`lib/`：第三方头文件和 TA-Lib 链接库。

## 构建

推荐环境：

- Visual Studio 2022 或更新版本，C++ 桌面开发组件。
- MSVC v143 toolset。
- Windows SDK。工程文件当前写的是 `10.0.22621.0`；如果本机只安装了其他 SDK，可在 MSBuild 命令里覆盖 `WindowsTargetPlatformVersion`。

示例构建命令：

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe' `
  'E:\gddll\gddll.sln' `
  /m `
  /p:Configuration=Release `
  /p:Platform=x64 `
  /p:WindowsTargetPlatformVersion=10.0.26100.0
```

构建成功后输出：

- `x64/Release/gddll.dll`
- `x64/Debug/gddll.dll`

## 配置

策略默认读取：

```text
Strategy/follow_config.json
```

文件不存在时会自动创建默认配置。示例：

```json
{
  "master_account": "071749",
  "followers": {
    "229574": {
      "coefficient": 1.0,
      "enabled": true
    }
  },
  "settings": {
    "price_adjustment_ticks": 5,
    "min_position_change": 1,
    "max_follow_ratio": 10.0,
    "min_follow_ratio": 0.1,
    "max_consecutive_errors": 5,
    "circuit_break_duration": 5
  }
}
```

字段说明：

- `master_account`：主账户 ID。
- `followers`：跟随账户配置，`coefficient` 为跟单系数，`enabled` 控制是否启用。
- `price_adjustment_ticks`：下单价格相对最新价调整的 tick 数。
- `min_position_change`：触发跟单的最小净持仓变化。
- `max_follow_ratio` / `min_follow_ratio`：跟单比例风险边界。
- `max_consecutive_errors`：连续错误达到该值后触发熔断。
- `circuit_break_duration`：熔断持续时间，单位为分钟。

## 并发与兼容性

`D:\futuretrade` 主程序已将策略回调从全局锁改为单策略实例锁。此 DLL 的 `StrategyTemplate` 必须包含同名成员：

```cpp
std::mutex m_callbackMutex;
```

主程序会在 tick、订单、成交、停止单等回调前持有该锁；DLL 内部自行注册的 `EVENT_OPENER`、`EVENT_CALL`、`EVENT_START`、`EVENT_FILE` 也在回调入口持有同一把锁，避免同一个策略实例被并发重入。

如果主程序侧 `StrategyTemplate` 的成员布局继续变化，策略 DLL 也必须同步更新并重新编译，否则可能出现加载失败或运行期内存访问错误。

## 部署

1. 构建 `Release|x64`。
2. 将 `x64/Release/gddll.dll` 放到 `futuretrade` 主程序配置的策略 DLL 加载目录。
3. 确认主程序和 DLL 使用同一套 `StrategyTemplate`、`ICTAEngine`、`qcstructs` ABI。
4. 按需准备 `Strategy/follow_config.json`。

## 最近维护

- 项目名从 `futuredll` 统一改为 `gddll`。
- 输出文件改为 `gddll.dll`。
- 同步 `futuretrade` 的 `m_callbackMutex` 并发回调模型。
