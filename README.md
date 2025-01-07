CPUFriend
=========

[![Build Status](https://github.com/acidanthera/CPUFriend/actions/workflows/main.yml/badge.svg?branch=master)](https://github.com/acidanthera/CPUFriend/actions) [![Scan Status](https://scan.coverity.com/projects/16841/badge.svg?flat=1)](https://scan.coverity.com/projects/16841)

A [Lilu](https://github.com/acidanthera/Lilu) plug-in for dynamic power management data injection.

#### Notes
This repository must be compiled with latest [Lilu](https://github.com/acidanthera/Lilu) and [MacKernelSDK](https://github.com/acidanthera/MacKernelSDK), otherwise the compilation will fail!

Note: ***Debug version of Lilu.kext and MacKernelSDK project folder should be put in the same folder as CPUFriend! And debug versions of Lilu and CPUFriend should also be used together when debugging!***

#### Configuration
This kext is most likely not required when the CPU is natively supported by XNU (macOS kernel). In case of necessity, please see [Instructions](https://github.com/acidanthera/CPUFriend/blob/master/Instructions.md) for configuration.

#### Credits
- [Apple](https://www.apple.com) for macOS
- [vit9696](https://github.com/vit9696) for [Lilu.kext](https://github.com/acidanthera/Lilu) and various helps
- [PMheart](https://github.com/PMheart) for writing the software and maintaining it
