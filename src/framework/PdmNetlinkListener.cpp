// Copyright (c) 2019 LG Electronics, Inc.
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

#include <libudev.h>
#include <stdarg.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cstring>
#include <thread>

#include "PdmNetlinkListener.h"
#include "PdmNetlinkEvent.h"
#include "PdmLogUtils.h"

#define memzero(x,l) (std::memset((x), 0, (l)))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define SUBSYSTEM "usb"


struct udev* udev = nullptr;

PdmNetlinkListener::PdmNetlinkListener(){
}

PdmNetlinkListener::~PdmNetlinkListener(){
    udev_unref(udev);
    m_listenerThread.join();
}


bool PdmNetlinkListener::startListener(){
    init();
    runListner();
    return true;
}

bool PdmNetlinkListener::stopListener(){
    udev_unref(udev);
    m_listenerThread.join();
    return true;
}

/*  To get the new udev instances
*/
void PdmNetlinkListener::init(){

    udev = udev_new();
    if (!udev) {
        fprintf(stderr, "udev_new() failed\n");
        return ;
    }

    enumerate_devices(udev);

}

void PdmNetlinkListener::enumerate_devices(struct udev* udev)
{

    struct udev_enumerate* enumerate = udev_enumerate_new(udev);

    udev_enumerate_add_match_subsystem(enumerate, SUBSYSTEM);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_add_match_subsystem(enumerate, "sound");
    udev_enumerate_add_match_subsystem(enumerate, "video4linux");
    udev_enumerate_add_match_subsystem(enumerate, "net");
    udev_enumerate_add_match_subsystem(enumerate, "tty");

    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* entry;

    udev_list_entry_foreach(entry, devices) {
        const char* path = udev_list_entry_get_name(entry);
        struct udev_device* device = udev_device_new_from_syspath(udev, path);
        if (device != NULL) {
            PdmNetlinkEvent *pNE = new PdmNetlinkEvent;
            pNE->pdmParser(device, true);
            onEvent(pNE);
        }
        udev_device_unref(device);
    }

    udev_enumerate_unref(enumerate);
}

void PdmNetlinkListener::threadStart(){

    struct udev_monitor* monitor = NULL;
    int fd_ep;
    int fd_udev = -1;
    struct epoll_event ep_udev;

    fd_ep = epoll_create1(EPOLL_CLOEXEC);
    if (fd_ep < 0) {
        PDM_LOG_ERROR("PdmNetlinkListener: %s line: %d error creating epoll fd: %m", __FUNCTION__, __LINE__,fd_ep);
        goto out;
    }
    monitor = udev_monitor_new_from_netlink(udev, "udev");
    if (monitor == NULL) {
        PDM_LOG_ERROR("PdmNetlinkListener: %s line: %d no socket\n", __FUNCTION__, __LINE__);
        goto out;
    }

    if(udev_monitor_filter_add_match_subsystem_devtype(monitor, "block", NULL) < 0 ||
        udev_monitor_filter_add_match_subsystem_devtype(monitor, "input", NULL) < 0 ||
            udev_monitor_filter_add_match_subsystem_devtype(monitor, "usb", "usb_device") < 0 ||
                udev_monitor_filter_add_match_subsystem_devtype(monitor, "sound", NULL) < 0 ||
                    udev_monitor_filter_add_match_subsystem_devtype(monitor, "video4linux", NULL) < 0 ||
                        udev_monitor_filter_add_match_subsystem_devtype(monitor, "net", NULL) < 0 ||
                            udev_monitor_filter_add_match_subsystem_devtype(monitor, "tty", NULL) < 0){
        PDM_LOG_ERROR("PdmNetlinkListener: %s line: %d filter failed\n", __FUNCTION__, __LINE__);
        goto out;
    }

   if (udev_monitor_enable_receiving(monitor) < 0) {
        PDM_LOG_ERROR("PdmNetlinkListener: %s line: %d bind failed\n", __FUNCTION__, __LINE__);
        goto out;
   }
   fd_udev = udev_monitor_get_fd(monitor);
   memzero(&ep_udev, sizeof(struct epoll_event));
   ep_udev.events = EPOLLIN;
   ep_udev.data.fd = fd_udev;
   if (epoll_ctl(fd_ep, EPOLL_CTL_ADD, fd_udev, &ep_udev) < 0) {
        PDM_LOG_ERROR("PdmNetlinkListener: %s line: %d fail to add fd to epoll: %m", __FUNCTION__, __LINE__,fd_ep);
        goto out;
   }

  for (;;) {
    int fdcount;
    struct epoll_event ev[4];
    struct udev_device *device;

    fdcount = epoll_wait(fd_ep, ev, ARRAY_SIZE(ev), -1);

    for (int i = 0; i < fdcount; i++) {
        if (ev[i].data.fd == fd_udev && ev[i].events & EPOLLIN) {
            device = udev_monitor_receive_device(monitor);
            if (device == NULL) {
                PDM_LOG_WARNING("PdmNetlinkListener: %s line: %d no device from socket", __FUNCTION__, __LINE__);
                continue;
            }
            PdmNetlinkEvent *pNE = new (std::nothrow) PdmNetlinkEvent;
            if(pNE){
                pNE->pdmParser(device, false);
                onEvent(pNE);
            } else {
                PDM_LOG_ERROR("PdmNetlinkListener: %s line: %d memory failure for PdmNetlinkEvent",__FUNCTION__, __LINE__);
            }
            udev_device_unref(device);
            }
        }
    }

   out:
       if (fd_ep >= 0)
        close(fd_ep);
    udev_monitor_unref(monitor);
}

void PdmNetlinkListener::runListner(){
    m_listenerThread = std::thread(&PdmNetlinkListener::threadStart,this);
}
