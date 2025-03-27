@echo off
echo Building SSSISANIGGER.dll and SSSISANIGGER.exe

REM Check if Visual Studio environment is initialized
where cl >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo Visual Studio environment not found.
    echo Please run this from a Visual Studio Developer Command Prompt or run vcvarsall.bat first.
    exit /b 1
)

echo Building SSSISANIGGER.dll...
cl -D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -DUNICODE -nologo -EHsc -std:c++latest -DNDEBUG -W1 -O2 -MT -GL -Iinclude -Ivendor -Ivendor/imgui -Ivendor/imgui/legacy SSSISANIGGER/*.cpp SSSISANIGGER/legacy/*.cpp SSSISANIGGER/legacy/features/*.cpp vendor/imgui/*.cpp vendor/imgui/legacy/*.cpp -link -DLL -OUT:SSSISANIGGER.dll -LTCG -MACHINE:x86

if %ERRORLEVEL% neq 0 (
    echo Failed to build SSSISANIGGER.dll
    exit /b 1
)

echo Building SSSISANIGGER.exe...
cl -D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -DUNICODE -nologo -EHsc -std:c++latest -DNDEBUG -W1 -O2 -MT -GL -Iinclude -Ivendor -Ivendor/imgui -Idriver injector.cpp driver/driver.cpp -link -OUT:SSSISANIGGER.exe -LTCG advapi32.lib

if %ERRORLEVEL% neq 0 (
    echo Failed to build SSSISANIGGER.exe
    exit /b 1
)

echo Build completed successfully. 