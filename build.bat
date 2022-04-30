@echo off
if not defined cl (
	call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
)
if not exist build mkdir build
pushd build
cl -FC -Zi -DDebug=1 ..\source\win32_racer.cpp /link user32.lib gdi32.lib Winmm.lib
popd
