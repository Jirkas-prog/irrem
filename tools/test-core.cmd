@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
if not exist test-output mkdir test-output
cl /nologo /EHsc /std:c++14 /W4 /utf-8 tests\core.cpp /Fe:test-output\core.exe /Fo:test-output\core.obj
if errorlevel 1 exit /b 1
test-output\core.exe
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++14 /W4 /utf-8 /I".pio\libdeps\tdisplay\ArduinoJson\src" tests\config.cpp /Fe:test-output\config.exe /Fo:test-output\config.obj
if errorlevel 1 exit /b 1
test-output\config.exe
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++14 /W4 /utf-8 /I".pio\libdeps\tdisplay\ArduinoJson\src" tests\gestures.cpp /Fe:test-output\gestures.exe /Fo:test-output\gestures.obj
if errorlevel 1 exit /b 1
test-output\gestures.exe
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++14 /W4 /utf-8 /Itests\display-stubs tests\display.cpp /Fe:test-output\display.exe /Fo:test-output\display.obj
if errorlevel 1 exit /b 1
test-output\display.exe
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++14 /W4 /utf-8 tests\wifi.cpp /Fe:test-output\wifi.exe /Fo:test-output\wifi.obj
if errorlevel 1 exit /b 1
test-output\wifi.exe
