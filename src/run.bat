if exist ..\cd\ goto created 
mkdir ..\cd
copy c:\saturn\cdtools\abs.txt ..\cd\abs.txt
copy c:\saturn\cdtools\bib.txt ..\cd\bib.txt
copy c:\saturn\cdtools\cpy.txt ..\cd\cpy.txt
:created
copy .\graphic\*.map ..\cd\
copy .\graphic\*.spr ..\cd\
copy .\graphic\*.tle ..\cd\
copy out.bin ..\cd\0.bin
c:\saturn\cdtools\mkisofs -sysid "SEGA SATURN" -volid "FinalFantasy Demo" -volset "FinalFantasy Demo" -publisher "SEGA ENTERPRISES, LTD." -preparer "SEGA ENTERPRISES, LTD." -appid "FinalFantasy Demo" -abstract "c:\saturn\cdtools\ABS.TXT" -copyright "c:\saturn\cdtools\CPY.TXT" -biblio "c:\saturn\cdtools\BIB.TXT" -generic-boot "c:\saturn\cdtools\ip_gnu.bin" -full-iso9660-filenames -o ..\out\out.iso ..\cd
cd ..\out
c:\saturn\mednafen\mednafen out.cue 
REM c:\saturn\yabause\!yabause.exe