if exist ..\cd\ goto created 
mkdir ..\cd
:created
copy .\graphic\*.map ..\cd\
copy .\graphic\*.spr ..\cd\
copy .\graphic\*.tle ..\cd\
copy out.bin ..\cd\0.bin
c:\saturn\cdtools\mkisofs -sysid "SEGA SATURN" -volid "FinalFantasy" -volset "FinalFantasy" -publisher "SEGA ENTERPRISES, LTD." -preparer "SEGA ENTERPRISES, LTD." -appid "FinalFantasy" -abstract "c:\saturn\cdtools\ABS.TXT" -copyright "c:\saturn\cdtools\CPY.TXT" -biblio "c:\saturn\cdtools\BIB.TXT" -generic-boot "c:\saturn\cdtools\ip_ff.bin" -full-iso9660-filenames -o ..\out\FinalFantasy.iso ..\cd
cd ..\out
c:\saturn\mednafen\mednafen FinalFantasy.cue 
REM c:\saturn\yabause\!yabause.exe