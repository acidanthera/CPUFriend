#!/bin/bash

kextFile="CPUFriendDataProvider.kext"
ssdtFile="ssdt_data.dsl"

function abort() {
  echo "ERROR: $1!"
  exit 1
}

function showHelp() {
  echo "Usage:"
  echo
  echo "-h, --help        Show this help message."
  echo "-a, --acpi <file> Create ${ssdtFile} with CPU power management data provided by file."
  echo "-k, --kext <file> Create ${kextFile} with CPU power management data provided by file."
}

function genSSDT() {
  local src="$1"
  local data2Hex=$(xxd -pr -u "${src}" | tr -d '\n' | sed 's/.\{2\}/\0x&, /g')

  local ifs=$IFS
  IFS=$'\n'
  local cpuNames=(`ioreg -p IODeviceTree -c IOACPIPlatformDevice -k cpu-type -k clock-frequency | egrep name | sed -e 's/ *[-|="<a-z>]//g'`)
  local cpuName="${cpuNames[0]}"
  # restore IFS
  IFS=$ifs

  cat << EOF > "${ssdtFile}"
DefinitionBlock ("", "SSDT", 2, "ACDT", "FreqData", 0x00000000)
{
  //
  // CPU device name, detected from ioreg.
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
        // Inject plugin-type = 1 to load X86*.kext
        //
        "plugin-type", 
        One, 
        
        //
        // Power management data file to be injected by CPUFriend.
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
  local dataBase64=$(cat "$src" | base64)

  mkdir -p "${kextFile}/Contents" && pushd "${kextFile}/Contents" &> /dev/null

  cat << EOF > Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleIdentifier</key>
  <string>org.acidanthera.driver.CPUFriendDataProvider</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundleName</key>
  <string>CPUFriendDataProvider</string>
  <key>CFBundlePackageType</key>
  <string>KEXT</string>
  <key>CFBundleShortVersionString</key>
  <string>1.0.1</string>
  <key>CFBundleVersion</key>
  <string>1.0.1</string>
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
      <data>${dataBase64}</data>
    </dict>
  </dict>
  <key>NSHumanReadableCopyright</key>
  <string>Copyright © 2017-2022 PMheart. All rights reserved.</string>
  <key>OSBundleRequired</key>
  <string>Root</string>
</dict>
</plist>
EOF
  
  popd &> /dev/null
}

function main() {
  case "$1" in
    -h|--help)
      showHelp
      exit 0
    ;;

    -a|--acpi )
      # now $1 is the resource plist
      shift
      if [ ! -f "$1" ]; then
        abort "$1 does not exist!"
      fi

      if [ -f "${ssdtFile}" ]; then
        read -p "${ssdtFile} already exists, override? (y/N) " ask4NewSSDT
        case "${ask4NewSSDT}" in
          y|Y )
          ;;
          
          * )
            exit 0
          ;;
        esac

        rm -f "${ssdtFile}"
      fi

      genSSDT "$1"
    ;;

    -k|--kext )
      # now $1 is the resource plist
      shift
      if [ ! -f "$1" ]; then
        abort "$1 does not exist!"
      fi

      if [ -d "${kextFile}" ]; then
        read -p "${kextFile} already exists, override? (y/N) " ask4NewKext
        case "${ask4NewKext}" in
          y|Y )
          ;;
          
          * )
            exit 0
          ;;
        esac

        rm -rf "${kextFile}"
      fi

      genKext "$1"
    ;;

    * )
      showHelp
      exit 1
    ;;
  esac
}

main "$@"
exit 0
