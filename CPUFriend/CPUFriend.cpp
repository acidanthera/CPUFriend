//
//  CPUFriend.cpp
//  CPUFriend
//
//  Copyright © 2017-2022 PMheart. All rights reserved.
//

#include <Headers/kern_api.hpp>

#include "CPUFriend.hpp"

static const char *kextACPISMC[] { "/System/Library/Extensions/IOPlatformPluginFamily.kext/Contents/PlugIns/ACPI_SMC_PlatformPlugin.kext/Contents/MacOS/ACPI_SMC_PlatformPlugin" };
static const char *kextX86PP[]   { "/System/Library/Extensions/IOPlatformPluginFamily.kext/Contents/PlugIns/X86PlatformPlugin.kext/Contents/MacOS/X86PlatformPlugin" };

enum : size_t {
	KextACPISMC,
	KextX86PP,
};

static KernelPatcher::KextInfo kextList[] {
	{ "com.apple.driver.ACPI_SMC_PlatformPlugin", kextACPISMC, arrsize(kextACPISMC), {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.apple.driver.X86PlatformPlugin",       kextX86PP,   arrsize(kextX86PP),   {}, {}, KernelPatcher::KextInfo::Unloaded },
};

static constexpr size_t kextListSize = arrsize(kextList);

static CPUFriendPlugin *callbackCpuf = nullptr;

OSDefineMetaClassAndStructors(CPUFriendData, IOService)

IOService *CPUFriendData::probe(IOService *provider, SInt32 *score)
{
	if (!provider) {
		return nullptr;
	}
	
	if (!callbackCpuf) {
		SYSLOG("cpuf", "missing storage instance");
		return nullptr;
	}
	
	if (callbackCpuf->frequencyData) {
		DBGLOG("cpuf", "frequency data already obtained, skipping");
		return nullptr;
	}
	
	DBGLOG("cpuf", "looking for cf-frequency-data in %s", safeString(provider->getName()));
	auto data = OSDynamicCast(OSData, provider->getProperty("cf-frequency-data"));
	if (!data) {
		auto cpu = provider->getParentEntry(gIOServicePlane);
		if (!cpu) {
			SYSLOG("cpuf", "unable to access cpu parent");
			return nullptr;
		}
		
		DBGLOG("cpuf", "looking for cf-frequency-data again in %s", safeString(cpu->getName()));
		data = OSDynamicCast(OSData, cpu->getProperty("cf-frequency-data"));
	}
	
	if (data) {
		callbackCpuf->frequencyDataSize = data->getLength();
		callbackCpuf->frequencyData     = data->getBytesNoCopy();
	} else {
		// This is expected for first start, second will do.
		DBGLOG("cpuf", "failed to obtain cf-frequency-data");
	}
	
	return nullptr;
}

void CPUFriendPlugin::init()
{
	callbackCpuf = this;
	
	lilu.onKextLoadForce(
		kextList,
		kextListSize,
		[](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
			static_cast<CPUFriendPlugin *>(user)->processKext(patcher, index, address, size);
		},
		this
	);
}

void CPUFriendPlugin::updateResource(kern_return_t &result, const void * &resourceData, uint32_t &resourceDataLength)
{
  if (!callbackCpuf) {
    SYSLOG("cpuf", "config callback arrived at nowhere");
    return;
  }

  auto data = callbackCpuf->frequencyData;
  auto size = callbackCpuf->frequencyDataSize;
  if (data && size > 0) {
    DBGLOG("cpuf", "feeding frequency data, size %u", size);
    resourceData       = data;
    resourceDataLength = size;
    result = kOSReturnSuccess;
  } else {
    // this is fine when not providing customized data,
    // in the worst case it's just the original
    // frequencyData and frequencyDataSize get handled,
    // which looks safe enough.
    DBGLOG("cpuf", "failed to feed cpu data (size %u, %d), feeding data from org callback", size, data != nullptr);
  }
}

void CPUFriendPlugin::myACPISMCConfigResourceCallback(uint32_t requestTag, kern_return_t result, const void *resourceData, uint32_t resourceDataLength, void *context)
{
	DBGLOG("cpuf", "myACPISMCConfigResourceCallback before (tag %u, result %d, data %d, size %u, context %d)", requestTag, result, resourceData != nullptr, resourceDataLength, context != nullptr);
	callbackCpuf->updateResource(result, resourceData, resourceDataLength);
	DBGLOG("cpuf", "myACPISMCConfigResourceCallback after (tag %u, result %d, data %d, size %u, context %d)", requestTag, result, resourceData != nullptr, resourceDataLength, context != nullptr);
	FunctionCast(myACPISMCConfigResourceCallback, callbackCpuf->orgACPISMCConfigLoadCallback)(requestTag, result, resourceData, resourceDataLength, context);
}

void CPUFriendPlugin::myX86PPConfigResourceCallback(uint32_t requestTag, kern_return_t result, const void *resourceData, uint32_t resourceDataLength, void *context)
{
	DBGLOG("cpuf", "myX86PPConfigResourceCallback before (tag %u, result %d, data %d, size %u, context %d)", requestTag, result, resourceData != nullptr, resourceDataLength, context != nullptr);
	callbackCpuf->updateResource(result, resourceData, resourceDataLength);
	DBGLOG("cpuf", "myX86PPConfigResourceCallback after (tag %u, result %d, data %d, size %u, context %d)", requestTag, result, resourceData != nullptr, resourceDataLength, context != nullptr);
	FunctionCast(myX86PPConfigResourceCallback, callbackCpuf->orgX86PPConfigLoadCallback)(requestTag, result, resourceData, resourceDataLength, context);
}

void CPUFriendPlugin::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size)
{
	if (kextList[KextACPISMC].loadIndex == index) {
		DBGLOG("cpuf", "patching KextACPISMC");
		KernelPatcher::RouteRequest request(
			"__ZL22configResourceCallbackjiPKvjPv",
			myACPISMCConfigResourceCallback,
			orgACPISMCConfigLoadCallback
		);
		patcher.routeMultiple(index, &request, 1, address, size);
	} else if (kextList[KextX86PP].loadIndex == index) {
		DBGLOG("cpuf", "patching KextX86PP");
		KernelPatcher::RouteRequest request(
			"__ZN17X86PlatformPlugin22configResourceCallbackEjiPKvjPv",
			myX86PPConfigResourceCallback,
			orgX86PPConfigLoadCallback
		);
		patcher.routeMultiple(index, &request, 1, address, size);
	}
}
