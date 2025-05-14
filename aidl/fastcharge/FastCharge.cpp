/*
 * SPDX-FileCopyrightText: 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "vendor.lineage.fastcharge-service.samsung"

#include "FastCharge.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <cutils/properties.h>

#include <samsung_fastcharge.h>

using ::android::base::ReadFileToString;
using ::android::base::Trim;
using ::android::base::WriteStringToFile;

namespace aidl {
namespace vendor {
namespace lineage {
namespace fastcharge {

static constexpr const char* kFastChargingProp = "persist.vendor.sec.fastchg_enabled";

FastCharge::FastCharge() {
    setEnabled(property_get_bool(kFastChargingProp, FASTCHARGE_DEFAULT_SETTING));
}

Return<bool> FastCharge::isEnabled() {
    return get(FASTCHARGE_PATH, 0) < 1;
}

Return<bool> FastCharge::setEnabled(bool enable) {
    set(FASTCHARGE_PATH, enable ? 0 : 1);

    bool enabled = isEnabled();
    property_set(kFastChargingProp, enabled ? "true" : "false");

    return enabled;
}

ndk::ScopedAStatus PowerShare::getSupportedFastChargeModes(int64_t* _aidl_return) {
}

ndk::ScopedAStatus PowerShare::getFastChargeMode(FastChargeMode _aidl_return) {
}

ndk::ScopedAStatus PowerShare::setFastChargeMode(FastChargeMode mode) {
}

}  // namespace fastcharge
}  // namespace lineage
}  // namespace vendor
}  // namespace aidl
