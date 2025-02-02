
/*
 * Copyright (C) 2022-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <compositionengine/UdfpsExtension.h>

uint32_t getUdfpsDimZOrder(uint32_t z) {
#ifdef FOD_DIM_LAYER_ZORDER
    z |= FOD_DIM_LAYER_ZORDER;
#endif
    return z;
}

uint32_t getUdfpsZOrder(uint32_t z, bool touched) {
    if (touched) {
        z |= FOD_PRESSED_LAYER_ZORDER;
    }
    return z;
}

uint64_t getUdfpsUsageBits(uint64_t usageBits, bool touched) {
    if (touched) {
        usageBits |= 0x400000000LL;
    }
    return usageBits;
}
