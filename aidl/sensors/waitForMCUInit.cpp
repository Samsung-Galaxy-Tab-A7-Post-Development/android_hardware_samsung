/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <fstream>
#include <chrono>
#include <thread>
#include <dlfcn.h>

extern "C" void* sensorsHalGetSubHal(uint32_t* version) {
    static auto sensorsHalGetSubHalOrig = reinterpret_cast<typeof(sensorsHalGetSubHal)*>(
            dlsym(dlopen("sensors.sensorhub.so", RTLD_NOW), "sensorsHalGetSubHal"));
    static void* ret = nullptr;
    bool mcuInitialized = false;

    while (!mcuInitialized) {
        std::ifstream mcu("/sys/devices/virtual/sensors/ssp_sensor/mcu_test");
        std::string value;
        if (!mcu) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (mcu >> value) {
            size_t length = value.length();
            size_t suffixLength = 2;  // Length of "OK"
            if (length >= suffixLength) {
                mcuInitialized = value.substr(length - suffixLength) == "OK";
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ret = sensorsHalGetSubHalOrig(version);
    return ret;
}
