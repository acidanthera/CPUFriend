//
//  kern_start.cpp
//  CPUFriend
//
//  Copyright © 2017 - 2019 PMheart. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "CPUFriend.hpp"

static CPUFriendPlugin cpuf;

static const char *bootargOff[] {
	"-cpufoff"
};

static const char *bootargDebug[] {
	"-cpufdbg"
};

static const char *bootargBeta[] {
	"-cpufbeta"
};

PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
	parseModuleVersion(xStringify(MODULE_VERSION)),
	LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
	bootargOff,
	arrsize(bootargOff),
	bootargDebug,
	arrsize(bootargDebug),
	bootargBeta,
	arrsize(bootargBeta),
	KernelVersion::MountainLion,
	KernelVersion::BigSur,
	[]() {
		cpuf.init();
	}
};
