# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
#
# SPDX-License-Identifier: Apache-2.0

import pytest
import os

from pytest_embedded import Dut
from audio_test_package.audio_recorder.recorder_stream import Recorder

@pytest.mark.esp32
@pytest.mark.esp32s3
def test_pipeline_play_sdcard_music_str_detect(dut: Dut)-> None:
    dut.expect(r'Destroy all the resources', timeout=30)
