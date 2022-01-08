This is spaceFn implementation based on kbfiltr sample by MS (https://github.com/microsoft/Windows-driver-samples/tree/master/input/kbfiltr/sys).

-----------------------

The best dicussion I saw about spaceFn is this one: https://geekhack.org/index.php?topic=51069.0

One small disadvantage I saw with a spaceFn feature, no matter how we implement it, is that the trigger of normal space key down event is moved to the up "edge", which may be a small problem for cases where "space"->"another key" are pressed very quickly, the second key down can happen before the space key up and causing cancel of the space events.

-----------------------

There are user space implementations for windows, the kernel implementation is to help avoid competing with other possible keyboard hook applications.

-----------------------

To install the driver:
1) build with vs2019 + window driver kit, remember to setup the testing certificate.
  a) In current form I think this support only windows 10 and 11. I only tested on one windows 11 machine.
  b) The inf file may not be very accurate, it only install for 2 rather common HW IDs.
2) install the certificate on the target windows machine. Also need to "bcdedit /set testsigning on" to enable test signing of driver.
3) right click and choose install to install the inf in the built folder.

-----------------------

The default key map when space is hold:

       y u i o               esc  home up   end
       h j k l ; '    --->   pu   left down right del bs
       n m                   pd   `

Note the map is based on scan codes, which means 1) the map fits for all layouts. 2) the real key names here (US layout) may be inaccurate for other layouts on some keys, but the positions are.

-----------------------

Tech Notes:
1) The current inf installs the driver as upper filter of the keyboard function driver. Under the class filter (kbdclass).
2) It should be easily installed as a class filter right "BEFORE" (which means will run under) kbdclass in "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4D36E96B-E325-11CE-BFC1-08002BE10318}\UpperFilters".
3) Currently the key map is hardcoded in sfn2_init_static() function in sfn2core.c.

-----------------------

Room to improve:
1) a monitor/config program should be great.
2) installer.
3) perhaps better as a class filter, or even a class filter above, but the latter will need the different intercepting framwork.
