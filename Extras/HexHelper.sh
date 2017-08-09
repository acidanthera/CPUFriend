#!/bin/bash

#set -x

function main()
{
	case "${1}" in
		--hex )
			shift
			if [[ -e "${1}" ]]; then
				cd "$(echo ${1} | grep -v "`basename ${1}`")"
				xxd -pr -u "${1}" | tr -d '\n' > "${1}.hex"
			else
				echo "${1} does not exist, failed to convert hex"
				exit 1
			fi
		;;
		
		--acpi-hex )
			shift
			if [[ -e "${1}" ]]; then
				cd "$(echo ${1} | grep -v "`basename ${1}`")"
				xxd -pr -u "${1}" | tr -d '\n' | sed 's/.\{2\}/\0x&, /g' > "${1}.hex"
			else
				echo "${1} does not exist, failed to convert hex"
				exit 2
			fi
		;;
		
		* )
			echo "Invalid option"
		;;
	esac
	
	open -a /Applications/TextEdit.app "${1}.hex"
}

main "$@"
