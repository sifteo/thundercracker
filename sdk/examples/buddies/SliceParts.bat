REM TODO: Port to shell script instead, so we can use this on any platform

@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

if "%1"=="" goto noinput

set INPUT_FOLDER=%1
set OUTPUT_FOLDER=%2

for /f %%a IN ('dir /b %INPUT_FOLDER%\parts*.png') do (
    set OUTPUT=!OUTPUT_FOLDER!\%%a
    set INPUT=!INPUT_FOLDER!\%%a
    
    set OUTPUT_PART_1=!OUTPUT:.png=_1.png!
    set OUTPUT_PART_2=!OUTPUT:.png=_2.png!
    set OUTPUT_PART_3=!OUTPUT:.png=_3.png!
    set OUTPUT_PART_4=!OUTPUT:.png=_4.png!
    
    if exist !OUTPUT! del !OUTPUT!
    if exist !OUTPUT_PART_1! del !OUTPUT_PART_1!
    if exist !OUTPUT_PART_2! del !OUTPUT_PART_2!
    if exist !OUTPUT_PART_3! del !OUTPUT_PART_3!
    if exist !OUTPUT_PART_4! del !OUTPUT_PART_4!
    
    REM Grab each buddy part and stick in its own PNG
    convert -size 48x48 xc:transparent !OUTPUT_PART_1!
    convert -size 48x48 xc:transparent !OUTPUT_PART_2!
    convert -size 48x48 xc:transparent !OUTPUT_PART_3!
    convert -size 48x48 xc:transparent !OUTPUT_PART_4!
    composite -geometry +0+0 !INPUT![48x40+40+0] !OUTPUT_PART_1! !OUTPUT_PART_1!
    composite -geometry +4+0 !INPUT![40x48+0+40] !OUTPUT_PART_2! !OUTPUT_PART_2!
    composite -geometry +0+8 !INPUT![48x40+40+88] !OUTPUT_PART_3! !OUTPUT_PART_3!
    composite -geometry +4+0 !INPUT![40x48+88+40] !OUTPUT_PART_4! !OUTPUT_PART_4!
    
    REM Create a transparent PNG to hold the final image
    convert -size 48x768 xc:transparent !OUTPUT!
    
    REM Rotation 1
    composite -geometry +0+0 !OUTPUT_PART_1! !OUTPUT! !OUTPUT!
    composite -geometry +0+48 !OUTPUT_PART_2! !OUTPUT! !OUTPUT!
    composite -geometry +0+96 !OUTPUT_PART_3! !OUTPUT! !OUTPUT!
    composite -geometry +0+144 !OUTPUT_PART_4! !OUTPUT! !OUTPUT!

    REM Rotation 2
    convert !OUTPUT_PART_1! -rotate -90 !OUTPUT_PART_1!
    convert !OUTPUT_PART_2! -rotate -90 !OUTPUT_PART_2!
    convert !OUTPUT_PART_3! -rotate -90 !OUTPUT_PART_3!
    convert !OUTPUT_PART_4! -rotate -90 !OUTPUT_PART_4!
    composite -geometry +0+192 !OUTPUT_PART_1! !OUTPUT! !OUTPUT!
    composite -geometry +0+240 !OUTPUT_PART_2! !OUTPUT! !OUTPUT!
    composite -geometry +0+288 !OUTPUT_PART_3! !OUTPUT! !OUTPUT!
    composite -geometry +0+336 !OUTPUT_PART_4! !OUTPUT! !OUTPUT!
    
    REM Rotation 3
    convert !OUTPUT_PART_1! -rotate -90 !OUTPUT_PART_1!
    convert !OUTPUT_PART_2! -rotate -90 !OUTPUT_PART_2!
    convert !OUTPUT_PART_3! -rotate -90 !OUTPUT_PART_3!
    convert !OUTPUT_PART_4! -rotate -90 !OUTPUT_PART_4!
    composite -geometry +0+384 !OUTPUT_PART_1! !OUTPUT! !OUTPUT!
    composite -geometry +0+432 !OUTPUT_PART_2! !OUTPUT! !OUTPUT!
    composite -geometry +0+480 !OUTPUT_PART_3! !OUTPUT! !OUTPUT!
    composite -geometry +0+528 !OUTPUT_PART_4! !OUTPUT! !OUTPUT!
    
    REM Rotation 4
    convert !OUTPUT_PART_1! -rotate -90 !OUTPUT_PART_1!
    convert !OUTPUT_PART_2! -rotate -90 !OUTPUT_PART_2!
    convert !OUTPUT_PART_3! -rotate -90 !OUTPUT_PART_3!
    convert !OUTPUT_PART_4! -rotate -90 !OUTPUT_PART_4!
    composite -geometry +0+576 !OUTPUT_PART_1! !OUTPUT! !OUTPUT!
    composite -geometry +0+624 !OUTPUT_PART_2! !OUTPUT! !OUTPUT!
    composite -geometry +0+672 !OUTPUT_PART_3! !OUTPUT! !OUTPUT!
    composite -geometry +0+720 !OUTPUT_PART_4! !OUTPUT! !OUTPUT!
    
    REM Replace the old transparent green with proper transparency
    convert !OUTPUT! -transparent "rgb(72,255,170)" !OUTPUT!
    
    REM Adjust the blue buddy, since its eyes are too close to the color
    REM STIR uses for transparency.
    if "%%a"=="parts2.png" convert !OUTPUT! -fuzz 25% -fill "rgb(36,218,255)" -opaque "rgb(72,255,255)" !OUTPUT!
    
    del !OUTPUT_PART_1!
    del !OUTPUT_PART_2!
    del !OUTPUT_PART_3!
    del !OUTPUT_PART_4!
)

goto done

:noinput
echo Slice Parts: Converts a folder of folder of Gen1 buddy face
echo              parts PNGs for use with the Thundercracker version.
echo - Usage: SliceParts INPUT_FOLDER OUTPUT_FOLDER
goto done

:done
