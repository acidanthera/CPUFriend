//
// Copyright (c) 2017 Vanilla. All rights reserved.
//

DefinitionBlock ("", "SSDT", 1, "APPLE ", "freqdata", 0x00000001)
{
    //
    // The CPU device name. (CPU0 here)
    //
    External (_PR_.CPU0, DeviceObj)

    Scope (\_PR.CPU0)
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
                               
                }
            })
        }
    }
}

