# Changelog

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
