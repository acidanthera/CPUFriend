#!/bin/bash

#set -x

function main()
{
	case "${1}" in
		--kext )
			shift
			if [[ -e "${1}" ]]; then
				gData2Base64="$(cat "${1}" | base64)"
				
				if [[ -d CPUFriendDataProvider.kext ]]; then
					read -p "CPUFriendDataProvider.kext already exists, still continue? (y/n) " askforNewKext
					case "${askforNewKext}" in
						y|Y )
						;;

						* )
							exit
						;;
					esac
					rm -r CPUFriendDataProvider.kext
				fi
				
				mkdir -p CPUFriendDataProvider.kext/Contents && cd CPUFriendDataProvider.kext/Contents
				
				gPlistContent=`cat <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleIdentifier</key>
	<string>org.vanilla.driver.CPUFriendDataProvider</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>CPUFriendDataProvider</string>
	<key>CFBundlePackageType</key>
	<string>KEXT</string>
	<key>CFBundleShortVersionString</key>
	<string>1.0.0</string>
	<key>CFBundleVersion</key>
	<string>1.0.0</string>
	<key>IOKitPersonalities</key>
	<dict>
		<key>CPUFriendDataProvider</key>
		<dict>
			<key>CFBundleIdentifier</key>
			<string>com.apple.driver.AppleACPIPlatform</string>
			<key>IOClass</key>
			<string>AppleACPICPU</string>
			<key>IONameMatch</key>
			<string>processor</string>
			<key>IOProbeScore</key>
			<integer>1100</integer>
			<key>IOProviderClass</key>
			<string>IOACPIPlatformDevice</string>
			<key>cf-frequency-data</key>
			<data>${gData2Base64}</data>
		</dict>
	</dict>
	<key>NSHumanReadableCopyright</key>
	<string>Copyright © 2017 Vanilla. All rights reserved.</string>
	<key>OSBundleRequired</key>
	<string>Root</string>
</dict>
</plist>`
				echo "${gPlistContent}" > Info.plist
			else
				echo "${1} does not exist, failed to convert hex"
				exit 1
			fi
		;;
		
		--acpi )
			shift
			if [[ -e "${1}" ]]; then
				gCPUName="$(ioreg -p IODeviceTree -c IOACPIPlatformDevice -k cpu-type -k clock-frequency | egrep name | sed -e 's/ *[-|="<a-z>]//g')"
				gCPURealName="$(echo ${gCPUName[0]} | awk '{print $1}')"
				gDataContent="$(xxd -pr -u "${1}" | tr -d '\n' | sed 's/.\{2\}/\0x&, /g')"
				gSSDTContent=`cat <<EOF
DefinitionBlock ("", "SSDT", 1, "APPLE ", "freqdata", 0x00000001)
{
		//
		// The CPU device name. (${gCPURealName} here)
		//
		External (_PR_.${gCPURealName}, DeviceObj)

		Scope (\_PR.${gCPURealName})
		{
				Method (_DSM, 4, NotSerialized)
				{
						If (LEqual (Arg2, Zero))
						{
								Return (Buffer (One)
								{
										 0x03                                           
								})
						}

						Return (Package (0x04)
						{
								//
								// Inject plugin-type = 0x01 to load X86*.kext
								//
								"plugin-type", 
								One, 
								
								//
								// Power management data file to replace.
								//
								"cf-frequency-data", 
								Buffer ()
								{
										${gDataContent}
								}
						})
				}
		}
}`

				if [[ -f ssdt_data.dsl ]]; then
					read -p "ssdt_data.dsl already exists, still continue? (y/n) " askforNewDsl
					case "${askforNewPlist}" in
						y|Y )
						;;

						* )
							exit
						;;
					esac
				fi
				echo "${gSSDTContent}" > ssdt_data.dsl  # will override the old one
			else
				echo "${1} does not exist, failed to convert hex"
				exit 2
			fi
		;;
		
		* )
			echo "Invalid option"
			exit 3
		;;
	esac
}

main "$@"
exit 0
