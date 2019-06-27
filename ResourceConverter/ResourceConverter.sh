#!/bin/bash

#set -x

kextName="CPUFriendDataProvider.kext"
ssdtName="ssdt_data.dsl"

function showHelp() {
	echo -e "Usage:\n"
	echo "-a, --acpi file Create ${kextName} with information provided by file."
	echo "-k, --kext file Create ${ssdtName} with information provided by file."
	echo
}

function genSSDT() {
	local src="$1"
	local data2Hex="$(xxd -pr -u $1 | tr -d '\n' | sed 's/.\{2\}/\0x&, /g')"

	local ifs=$IFS
	IFS=$'\n'
	local cpuNames=(`ioreg -p IODeviceTree -c IOACPIPlatformDevice -k cpu-type -k clock-frequency | egrep name | sed -e 's/ *[-|="<a-z>]//g'`)
	local cpuName="${cpuNames[0]}"
	# restore IFS
	IFS=$ifs

	cat << EOF > "${ssdtName}"
DefinitionBlock ("", "SSDT", 1, "APPLE ", "freqdata", 0x00000001)
{
		//
		// The CPU device name. (${cpuName} here)
		//
		External (_PR_.${cpuName}, DeviceObj)

		Scope (\_PR.${cpuName})
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
										${data2Hex}
								}
						})
				}
		}
}
EOF
}

function genKext() {
	local src="$1"
	local data2B64="$(cat $1 | base64)"

	mkdir -p "${kextName}/Contents" && pushd "${kextName}/Contents" &> /dev/null

	cat << EOF > Info.plist
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
			<data>${data2B64}</data>
		</dict>
	</dict>
	<key>NSHumanReadableCopyright</key>
	<string>Copyright © 2017 - 2019 PMheart. All rights reserved.</string>
	<key>OSBundleRequired</key>
	<string>Root</string>
</dict>
</plist>
EOF
	
	popd &> /dev/null
}

function main() {
	case "$1" in
		-a|--acpi )
			# now $1 is the plist
			shift
			# exit when plist does not exist
			[[ ! -f "$1" ]] && echo "$1 does not exist!" && exit 1

			if [[ -f "${ssdtName}" ]]; then
				read -p "${ssdtName} already exists, override? (y/N) " ask4NewSSDT
				case "${ask4NewSSDT}" in
					y|Y ) ;;
					
					* ) exit ;;
				esac

				rm -f "${ssdtName}"
			fi

			genSSDT "$1"
		;;

		-k|--kext )
			# now $1 is the plist
			shift
			# exit when plist does not exist
			[[ ! -f "$1" ]] && echo "$1 does not exist!" && exit 1

			if [[ -d "${kextName}" ]]; then
				read -p "${kextName} already exists, override? (y/N) " ask4NewKext
				case "${ask4NewKext}" in
					y|Y ) ;;
					
					* ) exit ;;
				esac

				rm -rf "${kextName}"
			fi

			genKext "$1"
		;;

		* )
			if [[ -z "$1" ]]; then
				echo -e "Invaild option: (null)\n"
			else
				echo -e "Invaild option: $1\n"
			fi
			showHelp
			exit 1
		;;
	esac
}

main "$@"
exit 0