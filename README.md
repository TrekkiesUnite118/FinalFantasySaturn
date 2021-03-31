# Final Fantasy for Sega Saturn
This is an attempt at porting the NES version of Final Fantasy 1 to the Sega Saturn using SBL (Sega Basic Library). This originally began as a branch off of ndiddy's sample code, but has morphed into it's own thing. 

This project works by using the original NES ROM for all but the graphics data, which has been converted over to a Saturn friendly format. You will need to provide your own NES ROM. For sound CD Audio recordings of the PSP/PSX sound track are used.

ndiddy's original example project can be found here: 

https://github.com/shicky256/saturndevsbl

This also uses Ponut64's Sound Driver, you simply need to drop in a compiled binary of it into the cd directory. The driver can be found here:

https://github.com/ponut64/SCSP_poneSound

To build this you will need SBL which can be found here:

https://segaxtreme.net/resources/sbl-libraries.7/

You will also need a C environment that can compile for the SH-2 CPUs. If you have GCC installed you should be ok, however ndiddy has a zip archive that should have most of the environment ready to go in one easy package:

https://github.com/shicky256/SaturnDev#readme


## What's currently implemented:

Currently the following is implmented:

* Decompression of the Overworld Map Tile Map from the original ROM.
* Drawing the Overworld map to RBG0 and allowing it to scroll.
* Overworld Map player sprites are drawn to the screen and animate.
* Character class states can be initialized from the original ROM.
* Enemy and Battle data is parsed from the Original ROM.
* Calculations for if you encounter a battle, what type of battle, and what enemies you encounter.
* The logic for what scenery for the battle you encounter is implemented.
* Simple state system to transition to the battle screen.
* Battle Screen is drawn to NBG1 and character battle sprites are drawn.
* Character names and HP are drawn to the battle screen.
* Names of the types of enemies in the battle formation you encountered are drawn on screen.
* You can exit the battle screen back to the overworld by pressing the A button.

