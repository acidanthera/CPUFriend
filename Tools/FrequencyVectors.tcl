#
#   HexFiend 2.14.1 Binary Template
#
#      File: X86PlatformPlugin.kext resources (IOPlatformPluginFamily.kext)
#   Authors: vit9696
#   Version: 0.1
#   Purpose: Intel XCPM Frequency and IOPMSystemSleepPolicyVariables Decoding
#
# Adapted to HexFiend format by usr-sse2.
# Copyright (c) 2018-2022 vit9696. All rights reserved.
#
# Thanks to Apple for XNU Debug Information <3.
#

little_endian

proc uint32_dict { name dict {default ""} } {
	set n [uint32]
	set v $default
	if { [dict exists $dict $n] } {
		set v [dict get $dict $n]
    }
    entry $name [format "%s (%d)" $v $n] 4 [expr [pos]-4]
	return $n
}

set xcpmio_fv_t [dict create \
  0 "XCPMIO_FV_NONE" \
  1 "XCPMIO_FV_ABSOLUTE" \
  2 "XCPMIO_FV_NON_TURBO_PCT" \
  3 "XCPMIO_FV_TURBO_PCT" \
  4 "XCPMIO_FV_LPM_PCT" \
  5 "XCPMIO_FMAX_VMIN"
]

proc xcpmio_idle_t {} {
	uint64 "idle_duration"
	uint8 "idle_next"
	uint8 "idle_scale_percent"
}

proc xcpmio_vector_t {} {
	global xcpmio_fv_t
	uint32 "fv_specifier"
	uint32_dict "fv_type" $xcpmio_fv_t ""
	uint32 "fv_mhz"
	uint64 "fv_ttd_us"
	section "idle" {
		for {set i 0} {$i < 16} {incr i} {
	        section "$i" {
	        	xcpmio_idle_t
	        }
	    }
    }	
}

proc xcpmio_qos_t {} {
	ascii 16 "qos_name"
	uint8 "qos_min_index"
	uint8 "qos_max_index"
	section "qos_scales" {
		for {set i 0} {$i < 32} {incr i} {
			uint16 "$i"
	    }	
	}
	uint8 "qos_epp"
}

set xcpmio_var_type_t [dict create \
  0 "UINT64" \
  1 "STRING"
]

proc xcpmio_var_t {} {
	global xcpmio_var_type_t
	ascii 16 "var_name"
	set var_type [uint32]
	move -4
	uint32_dict "var_type" $xcpmio_var_type_t ""
	if {$var_type == 0} {
		uint64 "v_uint64"
		move 8
	} elseif {$var_type == 1} {
		ascii 16 "v_string"
	} else {
		move 16
	}
}

proc xcpmio_vectors_t {} {
	uint32 "version"
	section "vector" {
		for {set i 0} {$i < 32} {incr i} {
	        section "$i" {
	        	xcpmio_vector_t
	        }
	    }	
    }
    section "qos" {
		for {set i 0} {$i < 16} {incr i} {
	        section "$i" {
	        	xcpmio_qos_t
	        }
	    }	
    }
    section "vars" {
		for {set i 0} {$i < 16} {incr i} {
	        section "$i" {
	        	xcpmio_var_t
	        }
	    }	
    }
}

set kIOPMSystemSleepPolicySignature     0x54504c53
set kIOPMSystemSleepPolicyVersion       2

proc IOPMSystemSleepPolicyEntryV1 {} {
	uint32 "factorMask"
	uint32 "factorBits"
	uint32 "sleepFlags"
	uint32 "wakeEvents"
}

proc IOPMSystemSleepPolicyTableV1 {} {
	uint32 "signature"
	uint16 "version"
	set entryCount [uint16]
	move -2
	uint16 "entryCount"
	section "entries" {
		for {set i 0} {$i < $entryCount} {incr i} {
	        section "$i" {
	        	IOPMSystemSleepPolicyEntryV1
	        }
	    }	
	}	
}

proc IOPMSystemSleepPolicyEntryV2 {} {
	section "unknown" {
		for {set i 0} {$i < 8} {incr i} {
			uint32 "$i"
		}
	}
}

proc IOPMSystemSleepPolicyTableV2 {} {
	uint32 "signature"
	uint16 "version"
	set entryCount [uint16]
	move -2
	uint16 "entryCount"
	section "entries" {
		for {set i 0} {$i < $entryCount} {incr i} {
	        section "$i" {
	        	IOPMSystemSleepPolicyEntryV2
	        }
	    }	
	}
}


set sleepPolicySignature [uint32]
if  {$sleepPolicySignature == $kIOPMSystemSleepPolicySignature} {
	set sleepPolicyTableVersion [uint16]
	move -6
	section "sleep_policy" {
		if {$sleepPolicyTableVersion == 1} {
			IOPMSystemSleepPolicyTableV1
		} else {
			IOPMSystemSleepPolicyTableV2
		}
	}
} else {
	move -4
	xcpmio_vectors_t;
}
