idf_component_register(SRCS "test_gmf_core_main.c" "elements/gmf_audio_play_com.c"
                            "elements/gmf_audio_effects_test.c"
                            "elements/gmf_audio_play_el_test.c"
                            "elements/gmf_audio_rec_el_test.c"
                            "elements/gmf_ai_audio_test.c"
                       INCLUDE_DIRS "." "elements"
                       REQUIRES unity esp_codec_dev system_common test_utils
                       EMBED_FILES "hi_lexin.pcm"
                       WHOLE_ARCHIVE)
