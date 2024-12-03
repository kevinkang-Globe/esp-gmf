# ESP-GMF
[中文版](./README_CN.md)

The Espressif General Multimedia Framework (ESP-GMF) is a lightweight and versatile software framework developed by Espressif specifically for IoT multimedia applications. It is highly flexible and scalable and tailored for IoT chips, with RAM usage as low as 7 KB. ESP-GMF supports applications in audio, image, and video processing, as well as any data-streaming product.

ESP-GMF consists of three modules: **GMF-Core**, **GMF-Elements**, and **GMF-Examples**.

- **GMF-Core** is the core of the ESP-GMF software framework and includes the main components such as GMF-Element, GMF-Pipeline, and GMF-Task.
- **GMF-Elements** implements various functional elements based on GMF-Core, such as codec algorithms, audio processing algorithms, and more.
- **GMF-Examples** provides examples of how to use GMF-Pipeline to implement simple functionalities, such as playing music from flash or an SD card. It also includes examples of high-level components implemented using GMF-Pipeline, such as the **Audio Simple Player**, a basic audio decoding player.

# ESP-GMF Component Overview

Each ESP-GMF functional module exists as a component, currently including [GMF-Core](./gmf_core/README.md), [GMF-Audio](./gmf_elements/gmf_audio/README.md), gmf-image, [GMF-Misc](./gmf_elements/gmf_misc/README.md), and [GMF-IO](./gmf_elements/gmf_io/README.md).

| Component Name | Function | Dependent Components |
| :------------: | :------------: | :------------ |
| [GMF-Core](./gmf_core) | Core framework for GMF | None |
| [GMF-Element-Audio](./gmf_elements/gmf_audio) | Elements for audio encoding, decoding,<br>and sound effects processing | - `gmf-core`<br>- `esp_audio_effects`<br>- `esp_audio_codec` |
| [GMF-Element-MISC](./gmf_elements/gmf_misc) | Utility elements | None |
| GMF-Element-Image | Elements for image encoding, decoding,<br>and image effects processing | - `gmf-core`<br>- `esp_new_jpeg` |
| [GMF-IO](./gmf_elements/gmf_io) | Input/output for files, flash, and HTTP | - `gmf-core`<br>- `esp_codec_dev` |

When developing a project, it is recommended to use the elements and IOs components from the official GMF-Elements repository. You can also create your own elements and IO components to extend its application scenarios.

# ESP-GMF Usage Guide

For a basic example of GMF-Core API usage, see [test_apps](./gmf_core/test_apps/main/cases/gmf_pool_test.c). For practical application examples of GMF-Elements, see the [ examples ](./examples/basic_examples/) under GMF_Elements.

# Frequently Asked Questions

- **What is the difference between ESP-GMF and ESP-ADF?**

   ESP-ADF is a functional repository that includes many modules, such as `audio_pipeline`, `services`, `peripherals`, and `audio boards`. It is commonly used for more complex projects. ESP-GMF is an extension of the `audio_pipeline` module, designed to support applications for streaming audio, video, and image data. ESP-GMF offers a more modular structure than ESP-ADF's `audio_pipeline` by organizing functionality into different components, making it more flexible. For example, it is suitable for simple data streaming, such as playing audio from SD card/flash, or combining multiple components to provide more complex modules (e.g., audio player `esp_audio_simple_player`). Future versions of ESP-ADF will replace the `audio_pipeline` module with ESP-GMF.
