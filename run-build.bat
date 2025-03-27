@echo off
echo SSSISANIGGER Build System

echo Choose build method:
echo 1. Direct build (using exact compiler commands)
echo 2. Nob build system (with exact compiler flags)

choice /c 12 /n /m "Select an option (1 or 2): "

if %ERRORLEVEL% equ 1 (
    echo Running direct build...
    call build.bat
) else if %ERRORLEVEL% equ 2 (
    echo Building nob.exe first...
    cl -o nob.exe nob.c
    if %ERRORLEVEL% neq 0 (
        echo Failed to build nob.exe
        exit /b 1
    )
    
    echo Running nob with direct flag...
    .\nob.exe direct
)

echo Build process completed. 