//
//  CPUFriend.cpp
//  CPUFriend
//
//  Copyright © 2017 Vanilla. All rights reserved.
//

#include <Headers/kern_api.hpp>

#include "CPUFriend.hpp"

static const char *kextACPISMC[] = { "/System/Library/Extensions/IOPlatformPluginFamily.kext/Contents/PlugIns/ACPI_SMC_PlatformPlugin.kext/Contents/MacOS/ACPI_SMC_PlatformPlugin" };
static const char *kextX86PP[] = { "/System/Library/Extensions/IOPlatformPluginFamily.kext/Contents/PlugIns/X86PlatformPlugin.kext/Contents/MacOS/X86PlatformPlugin" };

enum : size_t {
	KextACPISMC,
	KextX86PP
};

static KernelPatcher::KextInfo kextList[] {
	{ "com.apple.driver.ACPI_SMC_PlatformPlugin", kextACPISMC, arrsize(kextACPISMC), {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.apple.driver.X86PlatformPlugin", kextX86PP, arrsize(kextX86PP), {}, {}, KernelPatcher::KextInfo::Unloaded }
};

static constexpr size_t kextListSize = arrsize(kextList);

static CPUFriendPlugin *callbackCpuf = nullptr;

OSDefineMetaClassAndStructors(CPUFriendData, IOService)

IOService *CPUFriendData::probe(IOService *provider, SInt32 *score)
{
	if (provider) {
		if (callbackCpuf) {
			if (!callbackCpuf->frequencyData) {
				auto name = provider->getName();
				if (!name)
					name = "(null)";
				DBGLOG("probe", "looking for cf-frequency-data in %s", name);
				
				auto data = OSDynamicCast(OSData, provider->getProperty("cf-frequency-data"));
				if (!data) {
					auto cpu = provider->getParentEntry(gIOServicePlane);
					if (cpu) {
						name = cpu->getName();
						if (!name)
							name = "(null)";
						DBGLOG("probe", "looking for cf-frequency-data in %s", name);
						data = OSDynamicCast(OSData, cpu->getProperty("cf-frequency-data"));
					} else {
						SYSLOG("probe", "unable to access cpu parent");
					}
				}
				
				if (data) {
					callbackCpuf->frequencyDataSize = data->getLength();
					callbackCpuf->frequencyData = data->getBytesNoCopy();
				} else {
					SYSLOG("probe", "failed to obtain cf-frequency-data");
				}
			}
		} else
			SYSLOG("probe", "missing storage instance");
	}
	
	return nullptr;
}

bool CPUFriendPlugin::init()
{
	callbackCpuf = this;
	
	LiluAPI::Error error = lilu.onKextLoad(kextList, kextListSize, [](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		static_cast<CPUFriendPlugin *>(user)->processKext(patcher, index, address, size);
	}, this);
	
	if (error != LiluAPI::Error::NoError) {
		SYSLOG("init", "failed to register onKextLoad method %d", error);
		return false;
	}

	return true;
}

void CPUFriendPlugin::updateResource(kern_return_t &result, const void * &resourceData, uint32_t &resourceDataLength)
{
		if (callbackCpuf) {
			auto data = callbackCpuf->frequencyData;
			auto sz   = callbackCpuf->frequencyDataSize;
			if (data && sz > 0) {
				DBGLOG("updateResource", "feeding frequency data %u", sz);
				resourceData = data;
				resourceDataLength = sz;
				result = kOSReturnSuccess;
			} else {
				SYSLOG("updateResource", "failed to feed cpu data (%u, %d)", sz, data != nullptr);
			}
		} else {
			SYSLOG("updateResource", "config callback arrived at nowhere");
		}
}

void CPUFriendPlugin::myACPISMCConfigResourceCallback(uint32_t requestTag, kern_return_t result, const void *resourceData, uint32_t resourceDataLength, void *context)
{
	DBGLOG("cpuf", "myACPISMCConfigResourceCallback %u %d %d %u %d", requestTag, result, resourceData != nullptr, resourceDataLength, context != nullptr);
	callbackCpuf->updateResource(result, resourceData, resourceDataLength);
	DBGLOG("cpuf", "myACPISMCConfigResourceCallback done %u %d %d %u %d", requestTag, result, resourceData != nullptr, resourceDataLength, context != nullptr);
	FunctionCast(myACPISMCConfigResourceCallback, callbackCpuf->orgACPISMCConfigLoadCallback)(requestTag, result, resourceData, resourceDataLength, context);
}

void CPUFriendPlugin::myX86PPConfigResourceCallback(uint32_t requestTag, kern_return_t result, const void *resourceData, uint32_t resourceDataLength, void *context)
{
	DBGLOG("cpuf", "myX86PPConfigResourceCallback %u %d %d %u %d", requestTag, result, resourceData != nullptr, resourceDataLength, context != nullptr);
	callbackCpuf->updateResource(result, resourceData, resourceDataLength);
	DBGLOG("cpuf", "myX86PPConfigResourceCallback done %u %d %d %u %d", requestTag, result, resourceData != nullptr, resourceDataLength, context != nullptr);
	FunctionCast(myX86PPConfigResourceCallback, callbackCpuf->orgX86PPConfigLoadCallback)(requestTag, result, resourceData, resourceDataLength, context);
}

void CPUFriendPlugin::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size)
{
	if (progressState != ProcessingState::EverythingDone) {
		for (size_t i = 0; i < kextListSize; i++) {
			if (kextList[i].loadIndex == index) {
				DBGLOG("processKext", "current kext is %s progressState %d", kextList[i].id, progressState);
				// clear error from the very beginning just in case
				patcher.clearError();
				
				if (i == KextACPISMC || i == KextX86PP) {
					callbackCpuf = this;
					
					KernelPatcher::RouteRequest requests[] {
						KernelPatcher::RouteRequest("__ZL22configResourceCallbackjiPKvjPv", myACPISMCConfigResourceCallback, orgACPISMCConfigLoadCallback),
						KernelPatcher::RouteRequest("__ZN17X86PlatformPlugin22configResourceCallbackEjiPKvjPv", myX86PPConfigResourceCallback, orgX86PPConfigLoadCallback)
					};
					
					patcher.routeMultiple(index, requests, address, size);
					
					progressState |= ProcessingState::CallbackRouted;
				}
			}
		}
	}
	
	// Ignore all the errors for other processors
	patcher.clearError();
}
