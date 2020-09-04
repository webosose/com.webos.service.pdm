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

#ifndef PLUGIN_API_H_
#define PLUGIN_API_H_

#include "IDevice.h"
#include "pdm-plugin/Api.h"
#include "pdm-plugin/PluginBase.h"

extern "C" Plugin *instantiatePlugin(Mananger *manager, LSHandle *lunaHandle);

enum PluginEventType { PdmEvent, PlugInEvent};

enum EventType { UNKNOWN = 0,
                  ADD,
                  REMOVE,
                  CHANGE,
                  MOUNT,
                  UMOUNT,
                  MOUNTALL,
                  UNMOUNTALL,
                  CONNECTING,
                  REMOVE_BEFORE_MOUNT,
                  MAX_COUNT_REACHED,
                  UNSUPPORTED_FS_FORMAT_NEEDED,
                  FSCK_STARTED,
                  FSCK_TIMED_OUT,
                  FSCK_FINISHED,
                  FORMAT_STARTED,
                  FORMAT_SUCCESS,
                  FORMAT_FAIL,
                  REMOVE_UNSUPPORTED_FS,
                  UMOUNTALL_SUCCESS,
                  UMOUNTALL_FAIL
};
//Plugin Event for power state
enum PowerState { POWER_STATE_NORMAL =2000,
                  POWER_STATE_SUSPEND,
                  POWER_STATE_REBOOT,
                  POWER_STATE_POWER_OFF,
                  POWER_STATE_RESUME_DONE,
                  POWER_STATE_NONE,
};
//Plugin Event for power processing state
enum PowerProcessing { POWER_PROCESS_NORMAL = 1000,
                  POWER_PROCESS_PREPARE_SUSPEND,
                  POWER_PROCESS_PREPARE_RESUME,
                  POWER_PROCESS_REQEUST_SUSPEND,
                  POWER_PROCESS_REQEUST_UMOUNTALL,
                  POWER_PROCESS_NONE,
};

//Plugin Event for camera state
enum CameraState {
                  PDM_CAM_NONE = 3000,                /**< 0x0 None */
                  PDM_CAM_OFF = PDM_CAM_NONE,            /**< 0x0 Camera Off */
                  PDM_CAM_READY,                    /**< 0x1 Camera ready */
                  PDM_CAM_BUILTIN,                 /**< 0x2 Camera builtin*/
                  PDM_CAM_BOTH = PDM_CAM_NONE,            /**< 0x0 Camera both*/
                  PDM_CAM_NOTSUPPORT = PDM_CAM_NONE        /**< 0x0 Camera not supported*/
};

//Plugin Event for modem dongle support eco mode
enum ModemDongleSupport {
    MODEM_DONGLE_SUPPORTED = 4000,
    MODEM_DONGLE_NOT_SUPPORTED
};

#endif /* PLUGIN_API_H_ */
