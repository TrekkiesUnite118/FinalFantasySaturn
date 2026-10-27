#!/bin/sh
# Builds msxconv.jar. Needs a JDK 9 or newer (javac and jar on PATH, or JAVA_HOME set).
# msx68k.bin is the assembled msx68k.s (vasmm68k_mot -m68000 -Fbin -o msx68k.bin msx68k.s).
set -e
cd "$(dirname "$0")"
BIN=${JAVA_HOME:+$JAVA_HOME/bin/}
rm -rf build
mkdir build
"${BIN}javac" --release 8 -d build *.java
cp msx68k.bin build/
"${BIN}jar" cfe msxconv.jar MsxConv -C build .
rm -rf build
echo Built msxconv.jar
