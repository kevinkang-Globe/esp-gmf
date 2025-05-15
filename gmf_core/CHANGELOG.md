# Changelog

## *Unreleased*

### Features
- Added `esp_gmf_pool_iterate_element` to support iteration over elements in the GMF pool
- Added FourCC codes to represent video element caps
- Optimized GMF argument and method name handling to avoid unnecessary copying
- Added `esp_gmf_oal_get_spiram_cache_align` for retrieving SPIRAM cache alignment
- Added helper macros for defining and retrieving GMF method and argument identifiers
- Added helper function for GMF method execution
- Added `esp_gmf_io_reset` API to reset the IO thread and reload jobs
- Added `meta_flag` field to `esp_gmf_payload_t` to support audio decoder recovery status tracking
- Added raw_pcm in `esp_fourcc.h`

### Bug Fixes

- Fixed pause timeout caused by missing sync event when pause producer entered an error state
- Fixed a thread safety issue with the gmf_task `running` flag
- Fixed parameter type mismatch in memory transfer operations to ensure data integrity

## v0.6.1

### Bug Fixes

- Fixed an issue where gmf_task failed to wait for event bits
- Fixed compilation failure when building for P4


## v0.6.0

### Features
- Added GMF element capabilities
- Added GMF port attribution functionality
- Added gmf_cache APIs for GMF payload caching
- Added truncated loop path handling for gmf_task execution
- Added memory alignment support for GMF fifo bus
- Renamed component to `gmf_core`

### Bug Fixes

- Fixed support for NULL configuration in GMF object initialization
- Standardized return values of port and data bus acquire/release APIs to esp_gmf_err_io_t only
- Improved task list output formatting

## v0.5.1

### Bug Fixes

- Fixed an issue where the block data bus returns a valid size when the done flag is set
- Fixed an issue where the block unit test contained incorrect code


## v0.5.0

### Features

- Initial version of `esp-gmf-core`