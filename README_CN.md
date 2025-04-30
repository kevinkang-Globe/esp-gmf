# ESP-GMF
[English](./README.md)

ESP-GMF 全称 Espressif General Multimedia Framework，是乐鑫开发的应用于 IoT 多媒体领域的轻量级通用软件框架。它灵活性高，可扩展性强，专为 IoT 芯片量身打造，RAM 资源占用只有 7 KB。ESP-GMF 可应用于音频、图像、视频等产品，还可应用于任何流式处理数据的产品中。

ESP-GMF 包含 GMF-Core、 GMF-Elements 和 GMF-Examples 三个模块。

- GMF-Core 是 ESP-GMF 软件框架的核心，有 GMF-Element、GMF-Pipeline、GMF-Task 等主要部件。
- GMF-Elements 是基于 GMF-Core 实现的各种具体功能 element，比如编解码算法、音效处理算法等。
- GMF-Examples 提供如何使用 GMF-Pipeline 实现简单功能的示例，比如播放 flash 或 SD 卡中的音乐，同时还有基于 GMF-Pipeline 实现的 High Level 功能的组件示例，比如 **ESP Audio Simple Player** 是一个简单音频解码播放器。

# ESP-GMF 组件介绍

ESP-GMF 各个功能模块以组件的形式存在，目前包含 [GMF-Core](./gmf_core/README_CN.md)、[GMF-Audio](./elements/gmf_audio/README_CN.md)、ESP-GMF-Image、[GMF-Misc](./elements/gmf_misc/README_CN.md)、[GMF-IO](./elements/gmf_io/README_CN.md)、 [GMF-AI-Audio](./elements/gmf_ai_audio/README_CN.md) 和 [GMF-Video](./elements/gmf_video/README_CN.md)。

|  组件名称 |  功能 | 依赖的组件  |
| :------------: | :------------:|:------------ |
|  [gmf_core](./gmf_core) | GMF 基础框架  |  无 |
|  [gmf_audio](./elements/gmf_audio) | GMF 音频编解码和<br>音效处理 element  | - `gmf_core`<br>- `esp_audio_effects`<br> - `esp_audio_codec` |
|  [gmf_misc](./elements/gmf_misc) | 工具类 element   | 无  |
|  [gmf_io](./elements/gmf_io) | 文件、flash、HTTP 输入输出  | - `gmf_core`<br>- `esp_codec_dev`  |
|  [gmf_ai_audio](./elements/gmf_ai_audio) | 智能语音算法和<br>语音识别 element | - `esp-sr`<br>- `gmf_core` |
|  [gmf_video](./elements/gmf_video) | GMF 视频编解码和<br>视频效果处理 element  | - `gmf_core`<br>- `esp_video_codec` |

在开发项目时，推荐使用官方 GMF-Elements 仓库的 elements 和 IOs 组件进行开发，也可以自行创建 element 和 IO 组件来扩展其应用场景。

# ESP-GMF 使用说明

GMF-Core API 的简单示例代码请参考 [test_apps](./gmf_core/test_apps/main/cases/gmf_pool_test.c)，GMF-Elements 实际应用示例请参考 GMF_Elements 下的 [ examples ](./gmf_examples/basic_examples/)。

# 常见问题

- **ESP-GMF 和 ESP-ADF 有什么区别？**

  ESP-ADF 是一个包含很多模块的功能性仓库，比如 `audio_pipeline`、`services`、`peripherals` 和 `audio boards` 等，它常应用与比较复杂的项目中。ESP-GMF 是将 `audio_pipeline` 独立出来并进行功能扩展，使其支持音频、视频、图像等流式数据的应用场景。ESP-GMF 按功能分为不同的组件，灵活性优于 ESP-ADF 的 `audio_pipeline`，例如用于简单流式数据的处理，从 SD 卡/flash 播放一个音频，以及多个组件结合使用提供较复杂的功能模块（如音频播放器 `esp_audio_simple_player `）。ESP-ADF 的后期版本会使用 ESP-GMF 替代 `audio_pipeline` 模块。
