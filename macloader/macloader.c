/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#ifdef COB_TYPE
#include <time.h>
#endif

#define LOG_TAG "macloader"
#include <utils/Log.h>
//@@#include <log/log_main.h>
#include <cutils/misc.h>
#include <cutils/properties.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <fnmatch.h>
#include <cutils/android_filesystem_config.h>
#include <cutils/fs.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include "macloader.h"

#define PACKAGE    "macloader"
#define VERSION    "2.1.0"

#define TRUE    1
#define FALSE   0

#define DIR_WIFI_MACADDR        "/efs/wifi"
#define PATH_WIFI_MACADDR_INFO  "/efs/wifi/.mac.info"
#ifdef COB_TYPE
#define PATH_WIFI_MACADDR_BACKUP    "/efs/wifi/.mac.cob"
#endif

#ifdef BCM_OTP_WRITE
#define PATH_WIFI_OTP_INFO	"/data/.otp.info"
#endif

#define MAC_STR_LEN 17
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MAC2STR_SEC(a) (a)[0], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define MACSTR_SEC "%02x.%02x.%02x"
#define MACREGEX "[a-f0-9][a-f0-9]:[a-f0-9][a-f0-9]:[a-f0-9][a-f0-9]:[a-f0-9][a-f0-9]:[a-f0-9][a-f0-9]:[a-f0-9][a-f0-9]"

#ifndef IFNAME
#define IFNAME "wlan0"
#endif

#define PATH_WIFI_CID_INFO "/data/misc/conn/.cid.info"
#define PATH_WIFI_WIFIVER_INFO "/data/misc/conn/.wifiver.info"

static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";
//static const char MODULE_FILE[]         = "/proc/modules";   // nomore used

#ifdef BROADCOM_DUT
static const char DRIVER_MODULE_NAME[]  = "dhd";
static const char DRIVER_MODULE_TAG[]   = "dhd ";

static const char DRIVER_MODULE_ARG[]  = "firmware_path=/system/vendor/etc/wifi/bcmdhd_sta.bin_b1 nvram_path=/system/vendor/etc/wifi/nvram_net.txt";
#ifndef SEC_WLAN_BUILTIN_DRIVER
static const char DRIVER_MODULE_PATH[]  = WIFI_DRIVER_MODULE_PATH;
#else
static const char DRIVER_MODULE_PATH[] = "";
static const char WLAN_INTERFACE_PATH[] = "/sys/class/net/wlan0";
#endif /* SEC_WLAN_BUILTIN_DRIVER */
static const char MFG_MODULE_ARG[]   = "firmware_path=system/vendor/etc/wifi/bcmdhd_mfg.bin_b1 nvram_path=system/vendor/etc/wifi/nvram_mfg.txt";
#elif ATHEROS_DUT
static const char DRIVER_MODULE_NAME[]  = "";
static const char DRIVER_MODULE_TAG[]   = "";
static const char DRIVER_MODULE_ARG[]   = "";
static const char DRIVER_MODULE_PATH[]  = "";
#elif TI_DUT
static const char DRIVER_MODULE_NAME[]  = "";
static const char DRIVER_MODULE_TAG[]   = "";
static const char DRIVER_MODULE_ARG[]   = "";
static const char DRIVER_MODULE_PATH[]  = "";
#elif QUALCOMM_DUT
#ifdef SEC_WLAN_BUILTIN_DRIVER
#ifndef WIFI_DRIVER_FW_PATH_STA
#define WIFI_DRIVER_FW_PATH_STA		"sta"
#endif
#ifndef WIFI_DRIVER_FW_PATH_MFG
#define WIFI_DRIVER_FW_PATH_MFG		"ftm"
#endif
#ifndef WIFI_DRIVER_FW_PATH_PARAM
#define WIFI_DRIVER_FW_PATH_PARAM		"/sys/module/wlan/parameters/fwpath"
#endif
#else /* SEC_WLAN_BUILTIN_DRIVER */
static const char DRIVER_MODULE_NAME[]  = "wlan";
static const char DRIVER_MODULE_TAG[]   = "wlan ";
#ifdef QUALCOMM_SCPC
static const char DRIVER_MODULE_ARG[]   = "con_mode=5";
static const char DRIVER_MODULE_ARG2[]   = "";
#else
static const char DRIVER_MODULE_ARG[]   = "";
#endif
static const char DRIVER_MODULE_PATH[]  = "/system/lib/modules/wlan.ko";
#endif /* SEC_WLAN_BUILTIN_DRIVER */
#elif MARVELL_DUT
static const char DRIVER_MODULE_NAME[]  = "";
static const char DRIVER_MODULE_TAG[]   = "";
static const char DRIVER_MODULE_ARG[]   = "";
static const char DRIVER_MODULE_PATH[]  = "";
#elif STERICSSON_DUT
static const char DRIVER_MODULE_NAME[]  = "cw1200_wlan";
static const char DRIVER_MODULE_TAG[]   = "cw1200_wlan";
static const char DRIVER_MODULE_ARG[20];
static const char DRIVER_MODULE_PATH[]  = WIFI_DRIVER_MODULE_PATH;
#elif SPRD_DUT
static const char DRIVER_MODULE_NAME[]  = "sprdwl";
static const char DRIVER_MODULE_TAG[]   = "sprdwl";
static const char DRIVER_MODULE_ARG[]   = "";
static const char DRIVER_MODULE_PATH[]  = "/system/lib/modules/sprdwl.ko";
#elif SAMSUNGSLSI_DUT
static const char DRIVER_MODULE_NAME[]  = "";
static const char DRIVER_MODULE_TAG[]   = "";
static const char DRIVER_MODULE_ARG[]   = "";
static const char DRIVER_MODULE_PATH[]  = "";
#elif MTK_DUT
static const char DRIVER_MODULE_NAME[]  = "";
static const char DRIVER_MODULE_TAG[]   = "";
static const char DRIVER_MODULE_ARG[]   = "";
static const char DRIVER_MODULE_PATH[]  = "";
#else
static const char DRIVER_MODULE_NAME[]  = "no_setup_driver_module_name";
static const char DRIVER_MODULE_TAG[]   = "no_setup_driver_module_tag";
static const char DRIVER_MODULE_ARG[]   = "";
static const char DRIVER_MODULE_PATH[]  = "";
#endif

#ifdef COB_TYPE
#define NUM_REPOSITORY 4
#define FILE_PATH_LEN 50

static const char MAC_REPOSITORY[NUM_REPOSITORY][FILE_PATH_LEN] = {
    "/efs/imei/.nvmac.info\0",
    "/data/.nvmac.info\0",
    "/data/.mac.info\0",
    "/data/misc/wifi/.mac.info\0"
};
#endif

#ifdef BCM_OTP_WRITE
static int mac_driver_loaded = 0;
#endif

static int ioctl_sock = 0;

#if defined(BROADCOM_DUT) && defined(BCM4334B2_POWER_WAR)

#define BCM4334POKE_MAX_WAIT (100) /* 1 equals to 50 ms */
#define POKE_PID_NAME "wlan.poke_helper.pid"
#define POKE_RET_NAME "wlan.poke_helper.ret"
#define POKE_ERR_NAME "wlan.patchram.error"

int tickcount()
{
    int ms;
    struct timeval time;

    gettimeofday(&time, NULL);
    ms = time.tv_sec*1000 + time.tv_usec/1000;
    return ms;
}

static int write_property(char *prop, char *value)
{
    int ret = -1;
    int count = 2*20; /* wait at most 2 seconds for completion */
    char prop_readback[PROPERTY_VALUE_MAX] = {'\0'};

    property_set(prop, value);

    /* wait prop to be set */
    while (count-- > 0) {
        if (property_get(prop, prop_readback, NULL)) {
            if (strcmp(value, prop_readback) == 0) {
                ALOGE("%s is written to (%s)\n", value, prop);
                ret = 0;
                break;
            }
        }
        property_set(prop, value);
        usleep(50000); /* 50ms */
    }

    return ret;
}

/* return patchram pid, 0 indicating errors */
static int setup_pid()
{
    pid_t poke_pid;
    char val[PROPERTY_VALUE_MAX] = {'\0'};

    memset(val, 0, sizeof(val));
    if (!property_get(POKE_PID_NAME, val, NULL)) {
        ALOGE("Failed to read %s\n", POKE_PID_NAME);
        return 0;
    }

    poke_pid = atoi(val);

    return poke_pid;
}

static int is_poke_running()
{
    int ret = 0;
    int tick;
    int count;
    char poke_status[PROPERTY_VALUE_MAX] = {'\0'};

    tick = tickcount();
    /* wait until bcm4334poke running (or timed out) */
    ret = -1;
    count = BCM4334POKE_MAX_WAIT;
    while (count-- > 0) {
        if (property_get(POKE_PID_NAME, poke_status, "-1")) {
            if (strcmp(poke_status, "-1")) {
                ALOGE("%s set to (%s) by service ******** \n", POKE_PID_NAME, poke_status);
                ret = 0;
                break;
            }
        }
        usleep(100000); /* 100ms */
    }

    if (ret != 0) {
        ALOGE("timed out in waiting for return value \n");
    }

    tick = tickcount() - tick;
    ALOGE("is_trigger_poke (set) takes %d ms\n", tick);
    return ret;
}

static int wifi_trigger_poke(int set)
{
    int ret = 0;
    int tick;
    pid_t poke_pid;
    struct sigaction sa;
    sigset_t sig_set;
    siginfo_t sig_info;
    struct timespec timeout;
    char poke_status[PROPERTY_VALUE_MAX] = {'\0'};
    int count = BCM4334POKE_MAX_WAIT; /* wait at most 5 seconds for completion */

    tick = tickcount();

    ret = write_property(POKE_RET_NAME, "invalid");
    if (ret != 0) {
        ALOGE("Failed to reset poke return property%s\n", POKE_RET_NAME);
        return -1;
    }

    count = BCM4334POKE_MAX_WAIT;
    while (count-- > 0) {
        poke_pid = setup_pid();
        if (poke_pid == 0) {
            ALOGE("Failed to read BT poke PID\n");
            return -1;
        }
        if (set)
            ret = kill(poke_pid, SIGUSR1);
        else
            ret = kill(poke_pid, SIGUSR2);

        if (ret == 0) break;

        usleep(50000); /* 50ms */
    }

    /* wait until bcm4334poke returns (or timed out) */
    ret = -1;
    count = BCM4334POKE_MAX_WAIT;
    while (count-- > 0) {
        if (property_get(POKE_RET_NAME, poke_status, NULL)) {
            if (strcmp(poke_status, "invalid") != 0) {
                ALOGE("%s set to (%s) by service ******** \n", POKE_RET_NAME, poke_status);
                ret = 0;
                break;
            }
        }
        usleep(50000); /* 50ms */
    }

    if (ret != 0) {
        ALOGE("timed out in waiting for return value \n");
        ret = write_property(POKE_ERR_NAME, "timeout");
    }

    tick = tickcount() - tick;
    ALOGE("wifi_trigger_poke (set) takes %d ms\n", tick);
    return ret;
}

#endif /* BROADCOM_DUT && BCM4334B2_POWER_WAR */

#ifdef SEC_WLAN_BUILTIN_DRIVER

#ifndef WIFI_DRIVER_FW_PATH_STA
#define WIFI_DRIVER_FW_PATH_STA		"/system/vendor/etc/wifi/bcmdhd_sta.bin_b2"
#endif
#ifndef WIFI_DRIVER_FW_PATH_MFG
#define WIFI_DRIVER_FW_PATH_MFG		"/system/vendor/etc/wifi/bcmdhd_mfg.bin"
#endif
#ifndef WIFI_DRIVER_NVRAM_PATH_MFG
#define WIFI_DRIVER_NVRAM_PATH_MFG		"system/vendor/etc/wifi/nvram_mfg.txt"
#endif
#ifndef WIFI_DRIVER_FW_PATH_PARAM
#define WIFI_DRIVER_FW_PATH_PARAM		"/sys/module/dhd/parameters/firmware_path"
#endif
#ifndef WIFI_DRIVER_NVRAM_PATH_PARAM
#define WIFI_DRIVER_NVRAM_PATH_PARAM		"/sys/module/dhd/parameters/nvram_path"
#endif

static int wifi_change_fw_path(const char *fwpath)
{
    int len;
    int fd;
    int ret = 0;

    if (!fwpath)
        return ret;

    fd = TEMP_FAILURE_RETRY(open(WIFI_DRIVER_FW_PATH_PARAM, O_WRONLY));

    if (fd < 0) {
        ALOGE("Failed to open wlan fw path param (%s)", strerror(errno));
        return -1;
    }

    len = strlen(fwpath) + 1;

    if (TEMP_FAILURE_RETRY(write(fd, fwpath, len)) != len) {
        ALOGE("Failed to write wlan fw path param (%s)", strerror(errno));
        ret = -1;
    }

    close(fd);

    return ret;
}

static int wifi_change_nvram_path(const char *calpath)
{
    int len;
    int fd;
    int ret = 0;

    if (!calpath) {
        ALOGE("calpath is null");
        return ret;
    }

    fd = TEMP_FAILURE_RETRY(open(WIFI_DRIVER_NVRAM_PATH_PARAM, O_WRONLY));

    if (fd < 0) {
        ALOGE("Failed to open wlan nvram path param (%s)", strerror(errno));
        return -1;
    }

    len = strlen(calpath) + 1;

    if (TEMP_FAILURE_RETRY(write(fd, calpath, len)) != len) {
        ALOGE("Failed to write wlan nvram path param (%s)", strerror(errno));
        ret = -1;
    }

    close(fd);

    return ret;
}
#endif /* SEC_WLAN_BUILTIN_DRIVER */

static int insmod(const char *filename, const char *args)
{
    /* O_NOFOLLOW is removed as wlan.ko is symlink pointing to
      the vendor specfic file which is in readonly location */
    int fd = open(filename, O_RDONLY | O_CLOEXEC);

    if (fd == -1) {
#ifndef SEC_PRODUCT_SHIP
        ALOGD("insmod: open(\"%s\") failed: %s", filename, strerror(errno));
#else
        ALOGD("insmod: open failed: %s", strerror(errno));
#endif
        return -1;
    }

#if defined(BROADCOM_DUT) && defined(BCM4334B2_POWER_WAR)
    if(is_poke_running()) {
        ALOGE("Poke isn't running \": %s\n", strerror(errno));
    }
    wifi_trigger_poke(1);
    int rc = syscall(__NR_finit_module, fd, args, 0);
    wifi_trigger_poke(0);
#else
    int rc = syscall(__NR_finit_module, fd, args, 0);
#endif /* defined(BCM4334B2_POWER_WAR) */
    ALOGE("init_module with filename: %s\n", filename);
	if (rc == -1) {
#ifndef SEC_PRODUCT_SHIP
        ALOGD("finit_module for \"%s\" failed: %s", filename, strerror(errno));
#else
        ALOGD("finit_module for failed: %s", strerror(errno));
#endif
	}
	
	close(fd);
	return rc;
}


static int rmmod(const char *modname)
{
    int ret = -1;
    int maxtry = 10;

    while (maxtry-- > 0) {
        ret = delete_module(modname, O_NONBLOCK | O_EXCL);
        if (ret < 0 && errno == EAGAIN)
            usleep(500000); // 500ms
        else
            break;
    }

    if (ret != 0)
        ALOGD("Unable to unload driver module \"%s\": %s\n",
             modname, strerror(errno));

    return ret;
}

static int load_mac_driver()
{
    struct ifreq ifr;
    int retry_cnt=2;

#ifdef BROADCOM_DUT

#ifdef SEC_WLAN_BUILTIN_DRIVER
    // Case 1: built-in driver
        /* load normal fw path */
        if (wifi_change_fw_path(WIFI_DRIVER_FW_PATH_MFG) < 0) {
            ALOGE("wifi_change_fw_path('%s') failed!!\n", WIFI_DRIVER_FW_PATH_MFG);
            return -1;
        }

        /* load normal nvram path */
        if (wifi_change_nvram_path(WIFI_DRIVER_NVRAM_PATH_MFG) < 0) {
            ALOGE("wifi_change_nvram_path('%s') failed!!\n", WIFI_DRIVER_NVRAM_PATH_MFG);
            return -1;
        }

#else  /* SEC_WLAN_BUILTIN_DRIVER */

    // Case 2: module driver
    ALOGE(" Loading the wifi driver MFG_MODULE_ARG");
    if (insmod(DRIVER_MODULE_PATH, MFG_MODULE_ARG) < 0) {
        if(errno != EEXIST) {
            while(retry_cnt-- > 0) {
                usleep(500000); //500ms
                if(insmod(DRIVER_MODULE_PATH, MFG_MODULE_ARG) < 0)
                    ALOGE("insmod() failed, retry insmod()!\n");
                else
                    break;
                if(retry_cnt == 0) {
                    ALOGE("insmod() failed!!\n");
                    return -1;
                }
            }
        }
    }
#endif /* SEC_WLAN_BUILTIN_DRIVER */
#elif STERICSSON_DUT
    ALOGE(" Loading the wifi driver with : %s",DRIVER_MODULE_ARG);
    if (insmod(DRIVER_MODULE_PATH, DRIVER_MODULE_ARG) < 0) {
        if(errno != EEXIST) {
            ALOGE("insmod() failed!!\n");
            return -1;
        }
    }
#elif SAMSUNGSLSI_DUT
    /* Nothing to DO */
    /* cysh@mc_wifi 2016-05062017 Nothing to DO */
    return 0;
#elif MTK_DUT
    /* Nothing to DO */
#elif QUALCOMM_DUT
#ifdef SEC_WLAN_BUILTIN_DRIVER
#ifdef QUALCOMM_SCPC
    if (wifi_change_fw_path(WIFI_DRIVER_FW_PATH_MFG) < 0) {
        ALOGE("wifi_change_fw_path('%s') failed!!\n", WIFI_DRIVER_FW_PATH_MFG);
        return -1;
    }
#else /* QUALCOMM_SCPC */
    if (wifi_change_fw_path(WIFI_DRIVER_FW_PATH_STA) < 0) {
        ALOGE("wifi_change_fw_path('%s') failed!!\n", WIFI_DRIVER_FW_PATH_STA);
        return -1;
    }
#endif /* QUALCOMM_SCPC */
#else  /* SEC_WLAN_BUILTIN_DRIVER */
    ALOGE(" Loading the wifi driver DRIVER_MODULE_ARG");
    if (insmod(DRIVER_MODULE_PATH, DRIVER_MODULE_ARG) < 0) {
        if(errno != EEXIST) {
            while(retry_cnt-- > 0) {
                usleep(500000); //500ms
                if(insmod(DRIVER_MODULE_PATH, DRIVER_MODULE_ARG) < 0)
                    ALOGE("insmod() failed, retry insmod()!\n");
                else
                    break;
                if(retry_cnt == 0) {
                    ALOGE("insmod() failed!!\n");
                    return -1;
                }
            }
        }
    }
#endif /* SEC_WLAN_BUILTIN_DRIVER */
#else
    ALOGE(" Loading the wifi driver DRIVER_MODULE_ARG");
    if (insmod(DRIVER_MODULE_PATH, DRIVER_MODULE_ARG) < 0) {
        if(errno != EEXIST) {
            while(retry_cnt-- > 0) {
                usleep(500000); //500ms
                if(insmod(DRIVER_MODULE_PATH, DRIVER_MODULE_ARG) < 0)
                    ALOGE("insmod() failed, retry insmod()!\n");
                else
                    break;
                if(retry_cnt == 0) {
                    ALOGE("insmod() failed!!\n");
                    return -1;
                }
            }
        }
    }
#endif

    ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (ioctl_sock < 0) {
        ALOGE("%s: socket(PF_INET,SOCK_DGRAM)", __func__);
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, IFNAME, IFNAMSIZ);

    if (ioctl(ioctl_sock, SIOCGIFFLAGS, &ifr)) {
        ALOGE("%s: ioctl(ioctl_sock, SIOCGIFFLAGS, &ifr)", __func__);
        return -1;
    } else {
        ALOGE("flag : 0x%04x", ifr.ifr_flags);
        ifr.ifr_flags |= IFF_UP;
        if (ioctl(ioctl_sock, SIOCSIFFLAGS, &ifr)) {
            ALOGE("%s: ioctl(ioctl_sock, SIOCSIFFLAGS, &ifr)", __func__);
            return -1;
        }
    }
    ALOGI(IFNAME" interface is successfully up");

    return 0;
}

#ifdef QUALCOMM_SCPC
static int load_mac_driver2()
{
    struct ifreq ifr;
    int retry_cnt=2;

    ALOGE(" Loading the wifi driver DRIVER_MODULE_ARG2");
#ifdef SEC_WLAN_BUILTIN_DRIVER
    if (wifi_change_fw_path(WIFI_DRIVER_FW_PATH_STA) < 0) {
        ALOGE("wifi_change_fw_path('%s') failed!!\n", WIFI_DRIVER_FW_PATH_STA);
        return -1;
    }
#else /* SEC_WLAN_BUILTIN_DRIVER */
    if (insmod(DRIVER_MODULE_PATH, DRIVER_MODULE_ARG2) < 0) {
        if(errno != EEXIST) {
            while(retry_cnt-- > 0) {
                usleep(500000); //500ms
                if(insmod(DRIVER_MODULE_PATH, DRIVER_MODULE_ARG2) < 0)
                    ALOGE("insmod() failed, retry insmod()!\n");
                else
                    break;
                if(retry_cnt == 0) {
                    ALOGE("insmod() failed!!\n");
                    return -1;
                }
            }
        }
    }
#endif /* SEC_WLAN_BUILTIN_DRIVER */

    ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (ioctl_sock < 0) {
        ALOGE("%s: socket(PF_INET,SOCK_DGRAM)", __func__);
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, IFNAME, IFNAMSIZ);

    if (ioctl(ioctl_sock, SIOCGIFFLAGS, &ifr)) {
        ALOGE("%s: ioctl(ioctl_sock, SIOCGIFFLAGS, &ifr)", __func__);
        return -1;
    } else {
        ALOGE("flag : 0x%04x", ifr.ifr_flags);
        ifr.ifr_flags |= IFF_UP;
        if (ioctl(ioctl_sock, SIOCSIFFLAGS, &ifr)) {
            ALOGE("%s: ioctl(ioctl_sock, SIOCSIFFLAGS, &ifr)", __func__);
            return -1;
        }
    }
    ALOGI(IFNAME" interface is successfully up");

    return 0;
}
#endif

static int unload_mac_driver()
{
    ALOGE("Unload mac driver\n");
    struct ifreq ifr;

#ifdef SAMSUNGSLSI_DUT
    return -1;  /* cysh@mc_wifi 2016-05062019 */
#endif

    ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (ioctl_sock < 0) {
        ALOGE("%s: socket(PF_INET,SOCK_DGRAM)", __func__);
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, IFNAME, IFNAMSIZ);

    if (ioctl(ioctl_sock, SIOCGIFFLAGS, &ifr)) {
        ALOGE("%s: ioctl(ioctl_sock, SIOCGIFFLAGS, &ifr)", __func__);
        return -1;
    } else {
        ALOGE("flag : 0x%04x", ifr.ifr_flags);
        ifr.ifr_flags &= (~IFF_UP);
        if (ioctl(ioctl_sock, SIOCSIFFLAGS, &ifr)) {
            ALOGE("%s: ioctl(ioctl_sock, SIOCSIFFLAGS, &ifr)", __func__);
            return -1;
        }
    }

    if (errno == EEXIST) return 0;

#ifndef SEC_WLAN_BUILTIN_DRIVER
    if (rmmod(DRIVER_MODULE_NAME) == 0) return 0;
#endif

    return -1; /* Success */
}


#ifdef COB_TYPE
static void generate_random_mac(char* mac)
{
    int value;
    unsigned int seed;
    unsigned char rand_mac[3];
    struct timeval val;

    gettimeofday(&val, NULL);

    seed = val.tv_sec * 1000000 + val.tv_usec;

    ALOGE("seed : %u", seed);
    srand((unsigned int) seed);

    value = rand();
    rand_mac[0] = value & 0xFF;
    rand_mac[1] = (value & 0xFF00) >> 8;
    rand_mac[2] = (value & 0xFF0000) >> 16;

    sprintf(mac, "00:12:36:%02X:%02X:%02X", rand_mac[0], rand_mac[1],rand_mac[2]);
    ALOGI("random Bytes are "MACSTR_SEC, rand_mac[0], rand_mac[1],rand_mac[2]);
}


static void get_mac_from_prev_repository(char* mac)
{
    int i, fd;

    for (i = 0; i < NUM_REPOSITORY; i++) {
        if (0 == access(MAC_REPOSITORY[i], F_OK)) {
            break;
        }
    }

    if (i == NUM_REPOSITORY) {
        ALOGI("There are no previous repository");
        goto no_repository;
    }

    if ((fd = open(MAC_REPOSITORY[i], O_RDONLY)) >= 0) {
        read(fd, mac, MAC_STR_LEN);
        close(fd);
        ALOGI("MAC from previous repository[%d]", i);
        return;
    }
    ALOGE("Couldn't get data from repository");

no_repository:
    memset(mac, 0, sizeof(char)*MAC_STR_LEN);
}
#endif// COB_TYPE


static int check_mac_data_validation(char* mac)
{
#if defined(BROADCOM_DUT) && defined(COB_TYPE)
    if (!strncmp(mac, "00:90:4C:XX:XX:XX", MAC_STR_LEN/2)) {
        ALOGE("It's DEFAULT NVMAC for BROADCOM (EPIGRAM, INC)!!");
        return -1;
    }
#endif

    if (!strncmp(mac, "00:00:00:00:00:00", MAC_STR_LEN)) {
        ALOGE("It's NULL MAC!!");
        return -1;
    }

#ifdef COB_TYPE
    if (!strncmp(mac, "00:12:34:00:0D:EF", MAC_STR_LEN)) {
        ALOGE("It's not COB MAC!!");
        return -1;
    }

    if (!memcmp(mac, "00:12:34", 8) || !memcmp(mac, "00:12:35", 8)) {
        ALOGE("It's old mac prefix!!\n");
        return -1;
    }
#endif

    if (!fnmatch(MACREGEX, mac, FNM_CASEFOLD)) {
        ALOGI("WIFI MAC is valid");
        return 0;
    } else {
        ALOGE("WIFI MAC is invalid");
        return -1;
    }
}


static int check_efsmac_data_validation()
{
    int fd;

    if ((fd = open(PATH_WIFI_MACADDR_INFO, O_RDONLY)) >= 0) {
        char mac[MAC_STR_LEN+1] = {0,};

        read(fd, mac, MAC_STR_LEN);
        close(fd);
        return check_mac_data_validation(mac);
    }
    ALOGE("Can not access mac file");
    return -1;
}

#ifdef BCM_OTP_WRITE
static int check_otp_data_validation()
{
    int fd;
    if ((fd = open(PATH_WIFI_OTP_INFO, O_RDONLY)) >= 0)
    {
        close(fd);
        return 0;
    }
    else {
        ALOGE("Can not access otp checking file");
        return -1;
    }
}
#endif

static int check_cid_data_validation()
{
    int fd;
    int strnum;

    if ((fd = open(PATH_WIFI_CID_INFO, O_RDONLY)) >= 0) {
        char cid[8] = { 0, };
	int str_cnt;

        str_cnt = read(fd, cid, 7);
	if (str_cnt < 0) {
		close(fd);
		return -1;
	}

	cid[str_cnt] = '\0';
        close(fd);

        if (strstr(cid, "samsung") || strstr(cid, "murata")
            || strstr(cid, "semco") || strstr(cid, "semcove")
            || strstr(cid, "semcosh") || strstr(cid, "semco3rd")
			|| strstr(cid, "wisol") || strstr(cid, "wisolfem1")) {
            ALOGI("WIFI cid is valid as %s", cid);
            return 0;
        } else {
            ALOGE("WIFI cid[%s] is invalid", cid);
            return -1;
        }
    }
    ALOGE("Can not access cid file");
    return -1;
}


static void create_efs_mac()
{
    int fd;

    ALOGE("Trying to create factory mac file");

    if ((fd = open(PATH_WIFI_MACADDR_INFO, O_CREAT|O_RDWR, 0660)) >= 0) {
        char mac[MAC_STR_LEN+1] = {0,};

        if (fchown(fd, AID_SYSTEM, AID_RADIO)) {
            ALOGE("%s: couldn't change file owner", __func__);
            close(fd);
            goto mac_creation_failure;
        }

#ifdef COB_TYPE
        {
            int fd2;
            if ((fd2 = open(PATH_WIFI_MACADDR_BACKUP, O_RDONLY)) >= 0) {
                read(fd2, mac, MAC_STR_LEN);
                close(fd2);
                ALOGI("get MAC from backup file");
            } else {
                get_mac_from_prev_repository(mac);
            }
        }

        if (check_mac_data_validation(mac) < 0) {
            generate_random_mac(mac);
        }
#else
        sprintf(mac, "00:12:34:00:0D:EF");
#endif
        write(fd, mac, strlen(mac));
        close(fd);

#ifndef COB_TYPE
        if (load_mac_driver()) goto mac_creation_failure;
        usleep(500000); // 500ms : time for updating OTP MAC in driver.
#ifdef BCM_OTP_WRITE
    mac_driver_loaded = 1;
#endif
#endif // COB_TYPE

    }

mac_creation_failure:

#ifndef COB_TYPE
    unload_mac_driver();
#endif
    return;
}

static int check_wifiver_file()
{
    FILE *fp;
    int nFileSize;

    fp = fopen(PATH_WIFI_WIFIVER_INFO, "rb");

    if (fp == NULL) {
        ALOGE("Can not access wifiver.info file");
    }
    else {
        fseek(fp,0,SEEK_END);
        nFileSize = ftell(fp);
        fclose(fp);

        if (nFileSize < 2) {
            ALOGI("There is no data on wifiver.info file, reload mfg firmware.");
            return 1;
        } else {
            ALOGE("Do not need to load firmware.");
            return 0;
        }
    }
    return 0;
}

#define WIFI_POWER_PATH                 "/dev/wmtWifi"

int mtk_wifi_set_power(int enable) {
    int sz;
    int fd = -1;
    const char buffer = (enable ? '1' : '0');

    fd = open(WIFI_POWER_PATH, O_WRONLY);
    if (fd < 0) {
        ALOGE("Open \"%s\" failed", WIFI_POWER_PATH);
        goto out;
    }
    sz = write(fd, &buffer, 1);
    if (sz < 0) {
        ALOGE("Set \"%s\" [%c] failed", WIFI_POWER_PATH, buffer);
        goto out;
    }

out:
    if (fd >= 0) close(fd);
    return 0;
}

int main(int argc, char **argv) {
#if defined(BROADCOM_DUT) && defined(SEC_WLAN_BUILTIN_DRIVER)
    int retry_cnt = 50;

    system("echo \"macloader: macloader triggered\" > /dev/kmsg");

    while(retry_cnt-- > 0) {
        if(access(WLAN_INTERFACE_PATH, F_OK) >= 0) {
            system("echo \"macloader: wlan interface loaded\" > /dev/kmsg");
            sleep(2);
            break;
        }
        else {
            system("echo \"macloader: Not yet wlan interface loaded\" > /dev/kmsg");
            sleep(1);
        }

    }
#endif
    if(access(DIR_WIFI_MACADDR, R_OK|W_OK|X_OK)) {
        ALOGE("factory file system is not ready yet");
        usleep(500000); // 500ms
    }

    if (access(PATH_WIFI_MACADDR_INFO, F_OK) || check_efsmac_data_validation() ) {
        create_efs_mac();
    }

#if defined(BROADCOM_DUT) && defined(NEED_CID)
    retry_cnt = 10;
    ALOGE("CID info file will be created!!!");
 
    while(retry_cnt-- > 0) {
      if (access(PATH_WIFI_CID_INFO, F_OK) || check_cid_data_validation()) {
          load_mac_driver();
          usleep(500000); // 500ms : time for updating CID in driver.
#ifdef BCM_OTP_WRITE
          mac_driver_loaded = 1;
#endif
          unload_mac_driver();
      }
      if(!check_cid_data_validation()) {
    	  ALOGE("CID info file was created!!!");
      	break;
      }
      ALOGE("CID info file was not created, thus retry creation CID info (%d)!!!", retry_cnt);
      sleep(1);
    }
#endif

#ifdef BCM_OTP_WRITE
    if(mac_driver_loaded == 0) {
    if(access(PATH_WIFI_OTP_INFO,  F_OK) || check_otp_data_validation()) {
            load_mac_driver();
	    usleep(500000); // 500ms : time for updating OTP MAC in driver.

        unload_mac_driver();
    }
    }
    chmod(PATH_WIFI_OTP_INFO,0666);
#endif

    ALOGE("Check wifiver info file..");
#ifndef QUALCOMM_SCPC
    if (check_wifiver_file()) {
#endif
        load_mac_driver();
        usleep(500000); // 500ms : time for updating CID in driver.
        unload_mac_driver();

#ifdef MTK_DUT		
		mtk_wifi_set_power(1);
		usleep(500000); // 500ms : time for updating CID in driver.
		mtk_wifi_set_power(0);
#endif


#ifdef QUALCOMM_SCPC		
        load_mac_driver2();
        usleep(500000); // 500ms : time for updating CID in driver.
        unload_mac_driver();
#else
    }
#endif
    return 0;
}
