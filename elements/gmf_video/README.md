# ESP-GMF-Video

- [中文版](./README_CN.md)

ESP GMF Video is a suite of video processing elements designed for a variety of tasks such as video encoding, and applying video effects. These elements can be combined to form a complete video processing pipeline.

## Elements

### Video Encoder
The Video Encoder element uses the `esp_video_codec` to convert raw video frames into compressed video streams. It currently supports two codecs:
- **H264**
- **MJPEG**

### Video Decoder
The video decoder element use the `esp_video_codec` to convert compressed video streams into raw video frames. It currently supports two codecs：
- **H264**
- **MJPEG**

### Video PPA (Pixel Processing Accelerator)
The Video PPA element is a compact element, support multiple functions. It is currently available on the ESP32P4 board and includes following functionalities:
- **Color Conversion:**  
  The ESP32P4 supports color conversion through two hardware modules:
  - **2D-DMA:** Automatically selected for better efficiency if supported.
  - **PPA:** Used as a fallback when 2D-DMA not supported
- **Resizing:**  
  Resizing functionality is provided by the PPA module.
- **Cropping:**  
  Cropping functionality is provided by the PPA module.
- **Rotation:**  
  Supports rotations at 0°, 90°, 180°, and 270°.

### Video FPS Converter
This module adjusts the frame rate of the video. It decreases the input frame rate to a specified output rate, using the Presentation Time Stamp (PTS) embedded in the input data to accurately schedule frames.

### Video Overlay Mixer
The Video Overlay Mixer module allows users to overlay additional graphics onto a video frame. By receiving overlay data via a user-defined port, it can blend elements such as timestamps, watermarks, or other images into a designated region of the original video frame.

## ESP-GMF-Video Release and SoC Compatibility

The following table summarizes the support for ESP-GMF-Video elements across Espressif SoCs in the current release.  
A check mark (&#10004;) indicates that the element is supported, while a cross mark (&#10006;) indicates it is not supported.

| Element         | ESP32       | ESP32-S2    | ESP32-S3    | ESP32-P4    |
|-----------------|:-----------:|:-----------:|:-----------:|:-----------:|
| Video PPA       | &#10006;    | &#10006;    | &#10006;    | &#10004;    |
| FPS Converter   | &#10004;    | &#10004;    | &#10004;    | &#10004;    |
| Overlay Mixer   | &#10004;    | &#10004;    | &#10004;    | &#10004;    |
| Video Decoder   | MJPEG only  | MJPEG only  | &#10004;    | &#10004;    |
| Video Encoder   | MJPEG only  | MJPEG only  | &#10004;    | &#10004;    |

## Notes

- **Video PPA** is currently supported **only** on the ESP32-P4.
- **FPS Converter** and **Overlay Mixer** are supported on **all SoCs**.
- **Video Decoder** supports MJPEG decoding on ESP32 and ESP32-S2
- **Video Encoder** supports MJPEG encoding on ESP32 and ESP32-S2

## Usage
ESP GMF Video modules are often used together to build a complete video processing pipeline. For example, you might first convert its colors or size, adjust the frame rate, and overlay and finally output through video encoder. For a practical implementation, please refer to the example in [test_app](../test_apps/main/elements/gmf_video_el_test.c).
