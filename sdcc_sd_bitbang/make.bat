@echo off
set "ROM=sdcard_bitbang"
set "QUARTUS_PROJECT_NAME=z80soc"
set "QUARTUS_PROJECT_FOLDER=..\..\..\DE1"
set "ROM_FOLDER=.\ROM"
set "QUARTUS_INSTALL_DIR=c:\Apps\altera\13.0sp1\quartus"

echo Compiling %ROM%
sdcc -o build/ -mz80 --opt-code-size -I../include -c %ROM%.c

echo Compiling FatFS...
sdcc -o build/ -mz80 -I../include -c ..\include\z80soc.c
sdcc -o build/ -mz80 --disable-warning 196 --disable-warning 126 --disable-warning 110 -I ../FatFS/source -c diskio.c
sdcc -o build/ -mz80 --disable-warning 196 --disable-warning 126 --disable-warning 110 -I ../FatFS/source -c ..\FatFS\source\ff.c

echo Comppiling libraries
sdasz80 -o build/crt0.rel ..\include\crt0.s

echo Linking code...
sdcc -o build/ -mz80 --code-loc 0x0100 --data-loc 0x0000 --no-std-crt0 -o build/%ROM%.ihx build/%ROM%.rel build/z80soc.rel build/diskio.rel build/ff.rel build/crt0.rel

echo Converting the ROM file
rem both .hex and .mif can be used. I created a ihx2mif since .mif seems more standard to use with MegaAizard Plugin memory cores.
rem To use .hex, the ROM memory core should be updated to load rom.hex instead of rom.mif
if not exist "build/%ROM%.ihx" (
    echo Error: The file "build/%ROM%.ihx" was not created.
    exit /b 1
)

packihx build/%ROM%.ihx > %ROM_FOLDER%\rom.hex
..\bin\ihx2mif2.exe build/%ROM%.ihx %ROM_FOLDER%\rom.mif
if %ERRORLEVEL% NEQ 0 (
    echo Convertion of ihx to mif failed!
    exit /b 1
) else (
    echo Convertion of ihx to mif finished.
)