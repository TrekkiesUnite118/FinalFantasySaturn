@echo off
rem Builds msxconv.jar. Needs a JDK 9 or newer (javac and jar on PATH, or JAVA_HOME set).
rem msx68k.bin is the assembled msx68k.s (vasmm68k_mot -m68000 -Fbin -o msx68k.bin msx68k.s).
setlocal
cd /d "%~dp0"
set BIN=
if defined JAVA_HOME set "BIN=%JAVA_HOME%\bin\"
if exist build rmdir /s /q build
mkdir build
"%BIN%javac" --release 8 -d build *.java || exit /b 1
copy /y msx68k.bin build >nul || exit /b 1
"%BIN%jar" cfe msxconv.jar MsxConv -C build . || exit /b 1
rmdir /s /q build
echo Built msxconv.jar
