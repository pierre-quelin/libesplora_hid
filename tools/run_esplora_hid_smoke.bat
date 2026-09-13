@echo off
setlocal EnableDelayedExpansion
REM Live Esplora smoke test — do NOT use variable name LIB (stolen by vcvars).
cd /d "%~dp0.." || (
  echo [ERROR] cannot cd to lib root
  exit /b 1
)
set "ESPLORA_HID_ROOT=%CD%"
echo [INFO] ESPLORA_HID_ROOT=%ESPLORA_HID_ROOT%

call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
  echo [ERROR] vcvars64.bat not found
  exit /b 1
)

set "OUTDIR=%ESPLORA_HID_ROOT%\build\smoke"
if not exist "%ESPLORA_HID_ROOT%\build" mkdir "%ESPLORA_HID_ROOT%\build"
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

set "INC=%ESPLORA_HID_ROOT%\src"
set "LIBUSB_ROOT=%ESPLORA_HID_ROOT%\deps\libusb-1.0.30"
set "LIBUSB_INC=%LIBUSB_ROOT%\include"
set "LIBUSB_LIBDIR=%LIBUSB_ROOT%\VS2019\MS64\dll"

echo [INFO] looking for "%LIBUSB_LIBDIR%\libusb-1.0.lib"
if not exist "%LIBUSB_LIBDIR%\libusb-1.0.lib" (
  echo [INFO] libusb missing — fetching...
  if not defined BUILD_TARGET set BUILD_TARGET=msvc16-x86_64
  call "%ESPLORA_HID_ROOT%\tools\Fetchlibusb_deps.bat"
  if errorlevel 1 (
    echo [ERROR] Fetchlibusb_deps.bat failed
    exit /b 1
  )
)

if not exist "%LIBUSB_INC%\libusb-1.0\libusb.h" (
  if exist "%LIBUSB_INC%\libusb.h" (
    if not exist "%LIBUSB_INC%\libusb-1.0" mkdir "%LIBUSB_INC%\libusb-1.0"
    copy /Y "%LIBUSB_INC%\libusb.h" "%LIBUSB_INC%\libusb-1.0\libusb.h" >nul
  )
)

if not exist "%LIBUSB_LIBDIR%\libusb-1.0.lib" (
  echo [ERROR] Still no libusb at "%LIBUSB_LIBDIR%"
  exit /b 1
)

cl /nologo /EHsc /std:c++17 /MD /DNOMINMAX /DNOGDI ^
  /I"%INC%" /I"%LIBUSB_INC%" ^
  "%~dp0esplora_hid_smoke.cpp" "%ESPLORA_HID_ROOT%\src\esplora\hid\esplora_hid.cpp" ^
  /Fe"%OUTDIR%\esplora_hid_smoke.exe" /Fo"%OUTDIR%\\" ^
  /link /LIBPATH:"%LIBUSB_LIBDIR%" libusb-1.0.lib
if errorlevel 1 exit /b 1

copy /Y "%LIBUSB_LIBDIR%\libusb-1.0.dll" "%OUTDIR%\" >nul
echo [INFO] Running smoke (press Esplora switch1 / DOWN when LED is green^)...
"%OUTDIR%\esplora_hid_smoke.exe"
echo RUN=%ERRORLEVEL%
exit /b %ERRORLEVEL%
