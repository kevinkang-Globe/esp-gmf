# Changelog

## *Unreleased*

### Features

- Added `esp_gmf_io_codec_dev_set_dev` API to set the audio codec device handle
- Added `esp_gmf_io_http_set_event_callback` API to set an HTTP event callback
- Added basic reset functionality to each IO to properly clean up resources and reset state

### Bug Fixes
- Fixed an issue where HTTP IO did not reset the thread and databus properly
- Fixed file open check for treat return error as valid fd
- Corrected return value validation for *acq_write/read and *acq_release_write/read callback function implementations
- Fixed HTTP connection timeout handling during _http_close operations

## v0.6.3

### Bug Fixes

- Changed the tag for embedded flash IO from `embed_flash` to `embed`
- Refactor the script to make the generated files follow our coding style

## v0.6.2

### Features

- Limit the version of `esp_codec_dev`

## v0.6.1

- Fixed memory leaks in I/O flash destroy API
- Corrected default values for I/O file and I/O I2S elements to ensure stable initialization
- Enforced esp_gmf_err_t return type for all element initialization functions (I/O flash, HTTP)

## v0.6.0

### Features
- Added support for NULL object configuration
- Renamed component to `gmf_io`
- Updated the License

### Bug Fixes
- Standardized return values of port and data bus acquire/release APIs to esp_gmf_err_io_t only

## v0.5.1

### Bug Fixes

- Fixed the component requirements


## v0.5.0

### Features

- Initial version of `esp-gmf-io`
