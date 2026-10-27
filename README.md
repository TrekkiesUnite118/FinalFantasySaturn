# Final Fantasy for Sega Saturn
This is an attempt at porting the NES version of Final Fantasy 1 to the Sega Saturn using SBL (Sega Basic Library). This originally began as a branch off of ndiddy's sample code, but has morphed into it's own thing. This started as me converting the logic over from the disassembly to C. I eventually got sidetracked and the project became stale.

Recently I figured this might be a good test for using AI to implement the remaining logic I hadn't gotten to yet. I originally had the Prelude, Title screen, Party Creation Screen, Overworld map screen, and a significant portion of the Battle system ported over. For my test I had Claude AI implement the rest. Overall I think it did a pretty good job thanks to the framework I had laid out as well as the incredibly well documented disassembly. My hope is that this can be a good example of using AI intelligently for this kind of project, rather than the slop examples we've been seeing lately. The framework could also allow easier porting of other NES games such as FF2 and FF3.

This project works by using the original NES ROM for all the data. It is NOT an emulator. You will need to provide your own US NES ROM. Any good dump should work. Sound is provided using a modified version of celeriyacon's NSF Player. 

The NES Disassembly can be found here:

https://github.com/Entroper/FF1Disassembly

ndiddy's original example project can be found here: 

https://github.com/shicky256/saturndevsbl

This originally used Ponut64's Sound Driver. This has been replaced with an NES APU driver. Ponesound is still in the baseline incase people want to use it later
for things like PCM audio and sound effects. The driver can be found here:

https://github.com/ponut64/SCSP_poneSound

The Sound is handled by using a modified version of celeriyacon's CroNSF Driver. The NSF Player functionality has been stripped out leaving only the SCSP and APU layer.
A reimplementation of the original game's sound driver instead drives this APU and SCSP driver to play the original sound data from the ROM. The original NSF Player Repo is here:

https://github.com/celeriyacon/cronsf

To build this you will need SBL which can be found here:

https://segaxtreme.net/resources/sbl-libraries.7/

You will also need a C environment that can compile for the SH-2 CPUs. If you have GCC installed you should be ok, however ndiddy has a zip archive that should have most of the environment ready to go in one easy package:

https://github.com/shicky256/SaturnDev#readme


## What's currently implemented:

The entire game should be implemented now, however I have not done a full playthrough yet, only spot checked with setting different states in the code to debug.

Controls are as follows:

* D-Pad - Same as the NES Version.
* A/C - Same as NES A
* B - Same as NES Version.
* Start - Brings up the Start Menu.
* L - Brings up the World Map.
* R - Brings up the Party Order Menu.

Optional bug fixes for the original game's logic are available in the Start Menu by pressing R.

Saving has also been revamped. Inns, Cottages, Tents, etc. still heal the party but no longer save. Instead saving is now handled through the Start Menu. You can save in any town or anywhere on the world map. There are 3 Save Slots available per Save Device. Devices supported are as follows:

 * Internal Save RAM
 * Backup RAM Cartridge
 * Floppy Disk Drive.

## MSX Graphics and Sound Support

Support for graphics and audio from the MSX2 port are now supported. You will need to provide the data for these on your own. In the tools directory is a Java application called msxconv.jar that will generate the necessary files for you. To use it you will first need to download this tool:

https://github.com/romh-acking/final-fantasy-msx2-say-dos-tool

Use this tool on an image of your MSX2 Final Fantasy Disk to extract the files and sector data. Then use the msxconf.jar file by pointing it to the extracted data as so:

java -jar mxsconf.jar <PATH_TO_EXTRACTED_MSX_FILES> <PATH_TO_MSX_SECTOR_DATA> MSXGFX.BIN MSXSND.BIN

Copy the generated files to the cd directory and build the image.

To enable MSX Graphics and Sound, press the L shoulder button in the start menu to bring up the new Options menu. You can then toggle between MSX and NES audio and graphics. Your choices will be saved to your save file.

## Future Goals:

Possible ideas for future features are as follows:

* Options for MSX Graphics and Sound.
* Option for PS1 or Pixel Remaster Graphics and Sound.
* Modern interface and quality of life improvements that have been added to later ports of the original game.
* Options for CD Audio Music.
* Optional graphical enhancements (Gouraud Shading for buff and debuffs in battle, transparent text boxes, etc.)
* Support for the Japanese ROM.

Keep in mind these are just ideas, there's no gaurantee they'll actually be implemented.

