idf_component_register(SRCS "test_gmf_core_main.c" "elements/gmf_audio_play_com.c"
                            "elements/gmf_audio_effects_test.c"
                            "elements/gmf_audio_play_el_test.c"
                            "elements/gmf_audio_rec_el_test.c"
                       INCLUDE_DIRS "." "elements"
                       REQUIRES unity gmf_core esp_codec_dev system_common test_utils
                       WHOLE_ARCHIVE)
