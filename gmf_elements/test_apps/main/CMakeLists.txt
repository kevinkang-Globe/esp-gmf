idf_component_register(SRC_DIRS "." "elements"
                       INCLUDE_DIRS "." "elements"
                       REQUIRES unity esp_codec_dev system_common test_utils
                       EMBED_FILES "hi_lexin.pcm"
                       WHOLE_ARCHIVE)
