# SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: CC0-1.0

import pytest
import os

from pytest_embedded import Dut

@pytest.mark.esp32
@pytest.mark.esp32s3
def test_str_detect(dut: Dut)-> None:
    dut.expect(r'ESP_GMF_TASK: One times job is complete, del', timeout=20)
