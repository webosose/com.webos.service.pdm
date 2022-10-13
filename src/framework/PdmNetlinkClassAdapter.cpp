// Copyright (c) 2022 LG Electronics, Inc.
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

#include "Common.h"
#include "PdmNetlinkClassAdapter.h"
#include "DeviceClass.h"
#include "DeviceClassCommand.h"
#include "DeviceClassFactory.h"
#include "PdmNetlinkEvent.h"

PdmNetlinkClassAdapter::PdmNetlinkClassAdapter() : mCmdManager(nullptr) {}

PdmNetlinkClassAdapter::~PdmNetlinkClassAdapter() {}

PdmNetlinkClassAdapter& PdmNetlinkClassAdapter::getInstance() {
    static PdmNetlinkClassAdapter obj;
	return obj;
}

void PdmNetlinkClassAdapter::setCommandManager(CommandManager* cmdMgr)
{
	mCmdManager = cmdMgr;
}

void PdmNetlinkClassAdapter::handleEvent(struct udev_device* device)
{
	DeviceClass* devClasPtr = DeviceClassFactory::getInstance().create(device);
	if (mCmdManager && devClasPtr) {
		DeviceClassCommand *devClassCmd = new (std::nothrow) DeviceClassCommand(devClasPtr);
		mCmdManager->sendCommand(devClassCmd);
	}
}

