# GMF 应用工具包

- [English Version](./README.md)

GMF 应用工具包（gmf_app_utils）是一个为方便 ESP GMF 开发应用程序而提供的常用便捷 API，它包含常见外设设置的配置，比如 Wi-Fi、SD card、AudioCodec 初始化，还包括一些系统管理功能，如性能监控接口和串口终端命令行接口（CLI）。

## 功能特性

### 外设管理（`esp_gmf_app_setup_peripheral.h`）
- 音频编解码器管理
  - 可配置的 I2S 模式（标准、TDM、PDM）
  - 独立的播放和录音配置
- I2C 接口管理
- 存储管理
  - SD 卡初始化和挂载
- 连接管理
  - Wi-Fi 初始化连接

### 系统工具（`esp_gmf_app_sys.h`）
提供系统资源监控功能的启动/停止，便于用户运行时性能跟踪以及资源使用情况监控，使用时需要在 menuconfig 中开启 `CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID` 和 `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS`。

### 命令行接口（`esp_gmf_app_cli.h`）
提供交互式 CLI 支持，用户可自定义命令提示符、注册自定义命令。同时也默认注册了一些常用系统命令便于用户直接使用，如下：

- 默认加载的系统命令
  - `restart`：软件重启
  - `free`：查看当前空闲内存大小
    - 显示内部内存和 PSRAM 的空闲大小
    - 显示历史最小空闲内存记录
  - `tasks`：显示任务运行信息和 CPU 使用情况
  - `log`：设置模块日志级别
    - 用法：log <标签> <级别 0-5>
    - 级别说明：关闭（0）、错误（1）、警告（2）、信息（3）、调试（4）、详细（5）
  - `io`：GPIO 引脚控制
    - 用法：io <引脚编号> <电平 0-1>
    - 设置指定 GPIO 引脚的输出电平
