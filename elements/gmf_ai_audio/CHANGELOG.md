# Changelog

## *Unreleased*

### Features

- Added `esp_gmf_afe_set_event_cb` API to register AFE element event callbacks
- Replaced the interface for decoder reconfiguration
- Introduced `esp_gmf_wn` element to support wake word detection

### Bug Fixes

- Fixed an issue where the output attribute of the `esp_gmf_afe` element was not set correctly
- Fixed a build error occurring when SPIRAM is disabled

## v0.6.2

### Features

- Limit the version of `esp_audio_codec` and `esp_codec_dev` in `aec_rec` example

## v0.6.1

### Bug Fixes

- Fixed a build issue caused by a variable type mismatch

## v0.6.0

### Features

- Initial version of GMF AI Audio Elements
