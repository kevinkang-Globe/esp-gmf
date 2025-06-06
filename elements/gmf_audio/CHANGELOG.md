# Changelog

## *Unreleased*

### Features

- Updated `esp_audio_codec` to version v2.3.0
- Updated `esp_audio_effects` to version v1.1.0
- Added `esp_gmf_audio_enc_get_frame_size`, `esp_gmf_audio_enc_set_bitrate`, `esp_gmf_audio_enc_get_bitrate`, `esp_gmf_audio_enc_reconfig` and `esp_gmf_audio_enc_reconfig_by_sound_info` functions to `gmf_audio_enc`
- Added `SBC` and `LC3` encoders
- Added `esp_gmf_audio_dec_reconfig` and `esp_gmf_audio_dec_reconfig_by_sound_info` functions to `gmf_audio_dec`
- Added `SBC` and `LC3` decoders
- Added `meta_flag` in `gmf_audio_dec` to support audio decoder recovery status tracking
- Added common audio parameter setting interface
- Redefined audio methods name
- Removed the audio encoder and decoder reconfig interface in `esp_gmf_audio_helper.c`
- Used the `esp_gmf_element_handle_t` type handle in the `gmf_audio` module

### Bug Fixes

- Fixed `gmf_audio_enc` process blocked due to forget release of in_load when truncate is returned
- Fixed parameter mismatch in `audio_dec_reconfig_dec_by_sound_info`

## v0.6.3

### Bug Fixes

- Fixed decoder not releasing input port on failure
- Fixed missing input size check in gmf_audio_enc when input is insufficient to encode a frame and stream has ended
- Fixed missing zero size check for acquire_in in gmf_audio_dec
- Fixed unmatched sub_cfg when reconfigured by esp_gmf_audio_helper_reconfig_dec_by_uri

## v0.6.2

### Features

- Limit the version of `esp_audio_codec` and `esp_audio_effects`


## v0.6.1

- Added missing #include "esp_gmf_audio_element.h" across all audio elements
- Fixed out-of-range parameter handling for mixer and EQ elements
- Resolved rate/bit/channel converter bypass errors caused by asynchronous modification of obj->cfg between set/event callbacks and process functions
- Deleted one unreasonable log from esp_gmf_audio_helper.c


## v0.6.0

### Features
- Updated the logic for GMF capabilities and method handling
- Added support for NULL object configuration
- Added GMF port attribution for elements
- Added gmf_cache and optimized the loop path for the audio encoder and decoder
- Renamed component to `gmf_audio`
- Updated the License


## v0.5.2

### Bug Fixes

- Fixed an issue where the decoder's output done flag was repeatedly set after the input was marked as done
- Fixed an issue where the input port was not released when the input data size was 0


## v0.5.1

### Bug Fixes

- Fixed the component requirements


## v0.5.0

### Features

- Initial version of `esp-gmf-audio`
