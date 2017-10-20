//
//  CPUFriend.hpp
//  CPUFriend
//
//  Copyright © 2017 Vanilla. All rights reserved.
//

#ifndef kern_cpuf_hpp
#define kern_cpuf_hpp

#include <Headers/kern_patcher.hpp>
#include <Library/LegacyIOService.h>

class EXPORT CPUFriendPlatform : public IOService {
	OSDeclareDefaultStructors(CPUFriendPlatform)
public:
	IOService * probe(IOService * provider, SInt32 *score) override;
};

class CPUFriendPlugin {
	public:
		bool init();
		
		/**
		 *  ResourceLoad callback type
		 */
		using t_callback = void (*)(uint32_t, kern_return_t, const void *, uint32_t, void *);
		
		/**
		 *  Trampolines for original resource load callback
		 */
		t_callback orgConfigLoadCallback = nullptr;
		
		/**
		 *  Loaded user-specified frequency data
		 */
		const void * frequencyData = nullptr;
		
		/**
		 *  Loaded user-specified frequency data size
		 */
		uint32_t frequencyDataSize = 0;

	private:
		/**
		 *  Hooked ResourceLoad callback returning user-specified platform data
		 */
		static void myConfigResourceCallback(uint32_t requestTag, kern_return_t result, const void * resourceData, uint32_t resourceDataLength, void * context);
		
		/**
		 *  Patch kext if needed and prepare other patches
		 *
		 *  @param patcher KernelPatcher instance
		 *  @param index   kinfo handle
		 *  @param address kinfo load address
		 *  @param size    kinfo memory size
		 */
		void processKext(KernelPatcher & patcher, size_t index, mach_vm_address_t address, size_t size);
		
		/**
		 *  Current progress mask
		 */
		struct ProcessingState {
			enum : uint32_t {
				NothingReady   = 0,
				CallbackRouted = 1,
				EverythingDone = CallbackRouted
			};
		};
		uint32_t progressState = ProcessingState::NothingReady;
};

#endif /* kern_cpuf_hpp */
