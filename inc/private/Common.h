// Copyright (c) 2019-2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef COMMON_H_
#define COMMON_H_

#include <string>

namespace PdmDevAttributes {

enum DeviceEventType { STORAGE_DEVICE = 0,
                       NON_STORAGE_DEVICE,
                       ALL_DEVICE,
                       HID_DEVICE,
                       VIDEO_DEVICE,
                       GAMEPAD_DEVICE,
                       MTP_DEVICE,
                       PTP_DEVICE,
                       SOUND_DEVICE,
                       BLUETOOTH_DEVICE,
                       CDC_DEVICE,
                       AUTO_ANDROID_DEVICE,
                       NFC_DEVICE,
                       UNKNOWN_DEVICE
                     };

//device status
    const std::string DEVICE_ADD = "add";
    const std::string DEVICE_REMOVE = "remove";
    const std::string DEVICE_CHANGE = "change";
    const std::string YES = "1";
    const std::string NO = "0";

    const std::string INPUT_CLASS = "INPUT_CLASS";
    const std::string DEVNUM = "DEVNUM";
    const std::string BUSNUM = "BUSNUM";
    const std::string ID_FS_TYPE_NEW  =  "ID_FS_TYPE_NEW";
    const std::string UUID  =  "UUID";
    const std::string ACTION  =  "ACTION";
    const std::string DEVLINKS  = "DEVLINKS";
    const std::string DEVNAME  =  "DEVNAME";
    const std::string DEVPATH  =  "DEVPATH";
    const std::string DEVTYPE  =  "DEVTYPE";
    const std::string CARD_READER = "CARD_READER";
    const std::string DISK_MEDIA_CHANGE = "DISK_MEDIA_CHANGE";
    const std::string ID_BUS  =  "ID_BUS";
    const std::string ID_FS_TYPE  = "ID_FS_TYPE"  ;
    const std::string ID_FS_USAGE  = "ID_FS_USAGE" ;
    const std::string ID_FS_UUID  =  "ID_FS_UUID";
    const std::string ID_FS_UUID_ENC  =  "ID_FS_UUID_ENC";
    const std::string ID_FS_VERSION  =  "ID_FS_VERSION";
    const std::string ID_INSTANCE  =  "ID_INSTANCE";
    const std::string ID_MODEL  =  "ID_MODEL";
    const std::string ID_MODEL_ENC  =  "ID_MODEL_ENC";
    const std::string ID_MODEL_ID  =  "ID_MODEL_ID";
    const std::string ID_PART_ENTRY_DISK  =  "ID_PART_ENTRY_DISK";
    const std::string ID_PART_ENTRY_NUMBER  = "ID_PART_ENTRY_NUMBER" ;
    const std::string ID_PART_ENTRY_OFFSET  =  "ID_PART_ENTRY_OFFSET";
    const std::string ID_PART_ENTRY_SCHEME  =  "ID_PART_ENTRY_SCHEME";
    const std::string ID_PART_ENTRY_SIZE  =  "ID_PART_ENTRY_SIZE";
    const std::string ID_PART_ENTRY_TYPE  =  "ID_PART_ENTRY_TYPE";
    const std::string ID_PART_TABLE_TYPE  =  "ID_PART_TABLE_TYPE";
    const std::string ID_PATH  =  "ID_PATH";
    const std::string ID_PATH_TAG  =  "ID_PATH_TAG";
    const std::string ID_REVISION  =  "ID_REVISION";
    const std::string ID_SERIAL  =  "ID_SERIAL";
    const std::string ID_SERIAL_SHORT  =  "ID_SERIAL_SHORT";
    const std::string ID_TYPE  =  "ID_TYPE";
    const std::string ID_USB_DRIVER  =  "ID_USB_DRIVER";
    const std::string ID_USB_INTERFACES  =  "ID_USB_INTERFACES";
    const std::string ID_USB_INTERFACE_NUM  =  "ID_USB_INTERFACE_NUM";
    const std::string ID_VENDOR_ENC  =  "ID_VENDOR_ENC";
    const std::string ID_VENDOR  =  "ID_VENDOR";
    const std::string ID_VENDOR_ID  =  "ID_VENDOR_ID";
    const std::string ID_FS_LABEL_ENC = "ID_FS_LABEL_ENC";
    const std::string MAJOR  =  "MAJOR";
    const std::string MINOR  =  "MINOR";
    const std::string SEQNUM  =  "SEQNUM";
    const std::string SUBSYSTEM  =  "SUBSYSTEM";
    const std::string TAGS  =  "TAGS";
    const std::string USEC_INITIALIZED  =  "USEC_INITIALIZED";
    const std::string dir_name  =  "dir_name";
    const std::string mount_options  =  "mount_options";
    const std::string ID_INPUT  =  "ID_INPUT";
    const std::string ID_INPUT_MOUSE  =  "ID_INPUT_MOUSE";
    const std::string PROCESSED = "PROCESSED";
    const std::string PRODUCT = "PRODUCT";
    const std::string SPEED = "SPEED";
    const std::string ID_BLACKLIST = "ID_BLACKLIST";
    const std::string ID_VENDOR_FROM_DATABASE = "ID_VENDOR_FROM_DATABASE";
    const std::string ID_MEDIA_PLAYER = "ID_MEDIA_PLAYER";
    const std::string HARD_DISK = "HARD_DISK";
    const std::string READ_ONLY = "READ_ONLY";
    const std::string ID_ATA = "ID_ATA";
    const std::string ID_GAMEPAD = "ID_GAMEPAD";
    const std::string ID_BLUETOOTH = "ID_BLUETOOTH";
    const std::string ID_USB_SERIAL = "ID_USB_SERIAL";
    const std::string ID_USB_MODEM_DONGLE = "ID_USB_MODEM_DONGLE";
//To check the device on power on
    const std::string IS_POWER_ON_CONNECT = "isPowerOnConnect";
// Black listed super speed devices
    const std::string ID_BLACK_LISTED_SUPER_SPEED_DEVICE = "ID_BLACK_LISTED_SUPER_SPEED_DEVICE";

//device categorisation
    const std::string USB_DEVICE = "usb_device";
    const std::string USB_HID    = "hid";
    const std::string USB_PARTITION = "partition";
    const std::string DISK = "disk";

// Unknown device
    const std::string DEV_TYPE_UNKNOWN = "UNKNOWN";

//fsck mode
    const std::string PDM_FSCK_AUTO = "auto";
    const std::string PDM_FSCK_FORCE = "force";

//drive types
    const std::string PDM_DRV_TYPE_NOFS     = "nofs";
    const std::string PDM_DRV_TYPE_NTFS     = "ntfs";
    const std::string PDM_DRV_TYPE_TNTFS    = "tntfs";
    const std::string PDM_DRV_TYPE_FAT      = "vfat";
    const std::string PDM_DRV_TYPE_TFAT     = "tfat";
    const std::string PDM_DRV_TYPE_JFS      = "jfs";
    const std::string PDM_DRV_TYPE_EXT2     = "ext2";
    const std::string PDM_DRV_TYPE_EXT3     = "ext3";
    const std::string PDM_DRV_TYPE_EXT4     = "ext4";
    const std::string PDM_DRV_TYPE_FUSE_PTP = "ptp_fuse";
    const std::string PDM_DRV_TYPE_FUSE_MTP = "mtp_fuse";
    const std::string PDM_DRV_TYPE_EXFAT    = "exfat";
    const std::string PDM_DRV_TYPE_ERR      = "err";

//mount data
    const std::string MS_VFAT_UTF8          = "iocharset=utf8,shortname=mixed,uid=0,gid=5000,umask=0002";
    const std::string MS_TFAT_UTF8          = "iocharset=utf8,uid=0,gid=5000,umask=0002";
    const std::string MS_TFAT_UTF8_FAST     = "iocharset=utf8,fastmount=1,max_prealloc_size=32m,uid=0,gid=5000,umask=0002";
    const std::string MS_NTFS_UTF8          = "nls=utf8,uid=0,gid=5000,umask=0002";
    const std::string MS_TNTFS_UTF8          = "nls=utf8,max_prealloc_size=64m,uid=0,gid=5000,umask=0002";

//journel mode
    const std::string JOURNAL_WRITEBACK_MODE = "data=writeback";
    const std::string JOURNAL_ORDERED_MODE   = "data=ordered";
    const std::string JOURNAL_JOURNAL_MODE   = "data=journal";

//drive status
    const std::string MOUNT_OK                    = "MOUNT_OK ";
    const std::string MOUNT_NOT_OK                = "MOUNT_NOT_OK ";
    const std::string IS_MOUNTING                 = "IS_MOUNTING ";
    const std::string UMOUNT_NOT_OK               = "UMOUNT_NOT_OK ";
    const std::string UMOUNT_OK                   = "UMOUNT_OK ";
    const std::string IS_UNMOUNTING               = "IS_UNMOUNTING ";
    const std::string FORMAT_PROCESSING           = "FORMAT_PROCESSING ";
    const std::string FSCK_PROCESSING             = "FSCK_PROCESSING ";
    const std::string PERFORMANCE_TEST_PROCESSING = "PERFORMANCE_TEST_PROCESSING ";

    const std::string FILESYSTEM = "filesystem";

//Error reasons
    const std::string PDM_ERR_NOTHING = "NOTHING";
    const std::string PDM_ERR_NOMOUNTED = "NOMOUNTED";
    const std::string PDM_ERR_UNSUPPORT_FS = "UNSUPPORT_FILESYSTEM";
    const std::string PDM_ERR_EJECTED = "EJECTED";
    const std::string PDM_ERR_NEED_FSCK = "NEED_FSCK";

//Storage types
    const std::string PDM_STORAGE_EMMC = "EMMC";
    const std::string PDM_STORAGE_STREAMING = "STREAMING";
    const std::string PDM_STORAGE_USB_FLASH = "FLASH";
    const std::string PDM_STORAGE_USB_HDD = "HDD";
    const std::string PDM_STORAGE_USB_UNDEFINED = "UNKNOWN";

    const std::string DEVICE_POWER_STATUS = "normal";

//PTP mount command
    const std::string PTP_MOUNT_COMMAND = "/usr/bin/gphotofs -o nonempty -o auto_unmount -o allow_other";

//fusermount unmount command for PTP and MTP
    const std::string FUSERMOUNT = "/usr/bin/fusermount -u ";

    const std::string ALERT_ID_USB_MAX_DEVICES = "usbMaxDevices";

//USB device Speed
    const std::string USB_SUPER_SPEED = "SUPER";
    const std::string USB_HIGH_SPEED = "HIGH";
    const std::string USB_FULL_SPEED = "FULL";
    const std::string USB_LOW_SPEED = "LOW";

//Fsck command timeout for storage device
    #define  FSCK_COMMAND_TIMEOUT  124

//Fsck timeout command value
    const std::string timeout = "timeout 20 ";
//Sound device
    const std::string CARD_ID = "CARD_ID";
    const std::string CARD_NUMBER = "CARD_NUMBER";
};

#endif /* COMMON_H_ */
