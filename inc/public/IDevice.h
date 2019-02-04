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

#ifndef IDEVICE_H_
#define IDEVICE_H_

#include <string>

class IDevice {

public:
    IDevice() = default;
    virtual ~IDevice() = default;
    virtual int getUsbPortNumber() = 0;
    virtual int getDeviceNum() = 0;
    virtual std::string getDeviceStatus() = 0;
    virtual std::string getDeviceType() = 0;
    virtual std::string getErrorReason() = 0;
    virtual std::string getVendorName() = 0;
    virtual std::string getProductName() = 0;
    virtual std::string getSerialNumber() = 0;
    virtual std::string getDevSpeed() = 0;
    virtual std::string getDevicePath() = 0;
    virtual std::string getDeviceSubType() = 0;
    virtual std::string getDeviceName() = 0;
    virtual bool isConnectedToPower() = 0;
    virtual std::string getDeviceSpeed(int speed)const = 0;
    virtual bool canDisplayToast() = 0;
    virtual int getPortSpeed() = 0;
    virtual void setDeviceType(std::string devType) = 0;
    virtual int getBusNumber() = 0;
};



#endif /* IDEVICE_H_ */
