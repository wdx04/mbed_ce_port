# Copyright (c) 2020 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

target_include_directories(sdio-blockdevice
    INTERFACE
        .
)

target_sources(sdio-blockdevice
    INTERFACE
        sdio_device.cpp
)
