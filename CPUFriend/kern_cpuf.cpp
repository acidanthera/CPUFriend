//
//  kern_cpuf.cpp
//  CPUFriend
//
//  Copyright © 2017 Vanilla. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include "kern_cpuf.hpp"

static const char *binList[] {
  "/System/Library/Extensions/IOPlatformPluginFamily.kext/Contents/PlugIns/X86PlatformPlugin.kext/Contents/MacOS/X86PlatformPlugin"
};

static const char *idList[] {
  "com.apple.driver.X86PlatformPlugin"
};

static const char *symbolList[] {
  "__ZN17X86PlatformPlugin22configResourceCallbackEjiPKvjPv"
};

static KernelPatcher::KextInfo kextList[] {
  { idList[0], &binList[0], arrsize(binList), false, false, {}, KernelPatcher::KextInfo::Unloaded }
};

static constexpr size_t kextListSize {arrsize(kextList)};

static CPUFriendPlugin *callbackCpuf {nullptr};

OSDefineMetaClassAndStructors(CPUFriendPlatform, IOService)

IOService *CPUFriendPlatform::probe(IOService *provider, SInt32 *score) {
  if (provider) {
    if (callbackCpuf) {
      if (!callbackCpuf->frequencyData) {
        auto name = provider->getName();
        if (!name)
          name = "(null)";
        DBGLOG("cpuf @ looking for cf-frequency-data in %s", name);
        
        auto data = OSDynamicCast(OSData, provider->getProperty("cf-frequency-data"));
        if (!data) {
          auto cpu = provider->getParentEntry(gIOServicePlane);
          if (cpu) {
            name = cpu->getName();
            if (!name)
              name = "(null)";
            DBGLOG("cpuf @ looking for cf-frequency-data in %s", name);
            
            data = OSDynamicCast(OSData, cpu->getProperty("cf-frequency-data"));
          } else {
            SYSLOG("cpuf @ unable to access cpu parent");
          }
        }
        
        if (data) {
          callbackCpuf->frequencyDataSize = data->getLength();
          callbackCpuf->frequencyData = data->getBytesNoCopy();
        } else {
          SYSLOG("cpuf @ failed to obtain cf-frequency-data");
        }
      }
    } else {
      SYSLOG("cpuf @ missing storage instance");
    }
  }
  
  return nullptr;
}

bool CPUFriendPlugin::init() {
  callbackCpuf = this;
  
  LiluAPI::Error error = lilu.onKextLoad(kextList, kextListSize,
  [](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    static_cast<CPUFriendPlugin *>(user)->processKext(patcher, index, address, size);
  }, this);
  
  if (error != LiluAPI::Error::NoError) {
    SYSLOG("cpuf @ failed to register onKextLoad method %d", error);
    return false;
  }

  return true;
}

void CPUFriendPlugin::myConfigResourceCallback(uint32_t requestTag, kern_return_t result, const void *resourceData, uint32_t resourceDataLength, void *context) {
  if (callbackCpuf && callbackCpuf->orgConfigLoadCallback) {
    auto data = callbackCpuf->frequencyData;
    auto sz = callbackCpuf->frequencyDataSize;
    if (data && sz > 0) {
      DBGLOG("cpuf @ feeding frequency data %u", sz);
      resourceData = data;
      resourceDataLength = sz;
      result = kOSReturnSuccess;
    } else {
      SYSLOG("cpuf @ failed to feed cpu data (%u, %d)", sz, data != nullptr);
    }
    
    callbackCpuf->orgConfigLoadCallback(requestTag, result, resourceData, resourceDataLength, context);
  } else {
    SYSLOG("cpuf @ config callback arrived at nowhere");
  }
}

void CPUFriendPlugin::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
  if (progressState != cpufessingState::EverythingDone) {
    for (size_t i = 0; i < kextListSize; i++) {
      if (kextList[i].loadIndex == index) {
        DBGLOG("cpuf @ current kext is %s progressState %d", kextList[i].id, progressState);
        
        if (!strcmp(kextList[i].id, idList[0])) {
          auto callback = patcher.solveSymbol(index, symbolList[0]);
          if (callback) {
            orgConfigLoadCallback = reinterpret_cast<t_callback>(patcher.routeFunction(callback, reinterpret_cast<mach_vm_address_t>(myConfigResourceCallback), true));
            if (patcher.getError() == KernelPatcher::Error::NoError) {
              DBGLOG("cpuf @ routed %s", symbolList[0]);
            } else {
              SYSLOG("cpuf @ failed to route %s", symbolList[0]);
            }
          } else {
            SYSLOG("cpuf @ failed to find %s", symbolList[0]);
          }
          
          progressState |= cpufessingState::CallbackRouted;
        }
        
        // Ignore all the errors for other processors
        patcher.clearError();
        break;
      }
    }
  }
}
