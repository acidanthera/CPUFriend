CPUFriend Installation & Usage
===================================

#### System Requirements
CPUFriend needs macOS ***v10.8*** or greater.

#### Installation
It's highly recommended to let the bootloader inject CPUFriend, otherwise you'll need [LiluFriend](https://github.com/PMheart/LiluFriend) to ensure CPUFriend will work properly.

#### Available kernel flags
Add `-cpufdbg` to enable debug logging (ONLY available in DEBUG binaries).

Add `-cpufoff` to disable CPUFriend entirely.

Add `-cpufbeta` to enable CPUFriend on unsupported OS versions.

#### Configuration
Open `CPUFriend/Extras` and you will see `CPUFriendProvider.kext` and `ssdt_data.dsl`. They should be the correct places where you start. 

Choose any way you like. Either via a provider kext (CPUFriendProvider.kext) or via ACPI (ssdt_data.dsl). 

The next step could be a little bit hard to explain. I'll write something if I have time/if I can... 

Here are also some examples to refer to. Uncompress `Examples.zip` for more details.

#### Usage of HexHelper.sh
````
--hex file
  Convert file to hexdecimal form called the same name of file, with .hex suffix.
  You can copy/paste the generated data to the Info.plist of CPUFriendProvider.kext (IOKitPersonalites -> CPUFriendDataProvider -> cf-frequency-data)
	
--acpi-hex file
  Convert file to hexdecimal form that will be accepted by ACPI specifications called the same name of file, with .hex suffix.
  You can copy/paste the generated data to the cf-frequency-data section of ssdt_sata.dsl and then compile it to aml.
````
