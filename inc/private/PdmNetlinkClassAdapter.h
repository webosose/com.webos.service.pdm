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

#ifndef _PDMNETLINKCLASSADAPTER_H
#define _PDMNETLINKCLASSADAPTER_H
#include <libudev.h>
#include "CommandManager.h"


class PdmNetlinkClassAdapter {
private:
	explicit PdmNetlinkClassAdapter();
	PdmNetlinkClassAdapter(const PdmNetlinkClassAdapter&) = delete;
	PdmNetlinkClassAdapter& operator=(const PdmNetlinkClassAdapter&) = delete;
	PdmNetlinkClassAdapter(PdmNetlinkClassAdapter&&) = delete;
	PdmNetlinkClassAdapter& operator=(PdmNetlinkClassAdapter&&) = delete;
	CommandManager *mCmdManager;
public:
	static PdmNetlinkClassAdapter& getInstance();
	void setCommandManager(CommandManager*);
	void handleEvent(struct udev_device*, bool isPowerOnConnect);
   	~PdmNetlinkClassAdapter();

};
#endif //_PDMNETLINKCLASSADAPTER_H
