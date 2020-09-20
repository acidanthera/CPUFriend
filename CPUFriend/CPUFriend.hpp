//
//  CPUFriend.hpp
//  CPUFriend
//
//  Copyright © 2017 - 2019 PMheart. All rights reserved.
//

#ifndef kern_cpuf_hpp
#define kern_cpuf_hpp

#include <Headers/kern_patcher.hpp>
#include <IOKit/IOService.h>

class EXPORT CPUFriendData : public IOService {
	OSDeclareDefaultStructors(CPUFriendData)
public:
	IOService *probe(IOService *provider, SInt32 *score) override;
};

class CPUFriendPlugin {
public:
	void init();
	
	/**
	 *  Loaded user-specified frequency data
	 */
	const void *frequencyData = nullptr;
	
	/**
	 *  Loaded user-specified frequency data size
	 */
	uint32_t frequencyDataSize = 0;

private:
	/**
	 *  Trampolines for original resource load callback
	 */
	mach_vm_address_t orgACPISMCConfigLoadCallback {0};
	mach_vm_address_t orgX86PPConfigLoadCallback   {0};

	/**
	 *  Update resource request parameters with hooked data if necessary
	 *
	 *  @param result             kOSReturnSuccess on resource update
	 *  @param resourceData       resource data reference
	 *  @param resourceDataLength resource data length reference
	 */
	void updateResource(kern_return_t &result, const void * &resourceData, uint32_t &resourceDataLength);
	
	/**
	 *  Hooked functions
	 */
	static void myACPISMCConfigResourceCallback(uint32_t requestTag, kern_return_t result, const void *resourceData, uint32_t resourceDataLength, void *context);
	static void myX86PPConfigResourceCallback(uint32_t requestTag, kern_return_t result, const void *resourceData, uint32_t resourceDataLength, void *context);

	/**
	 *  Patch kext if needed and prepare other patches
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
};

#endif /* kern_cpuf_hpp */
