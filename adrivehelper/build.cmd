@echo off
7z x deps.7z -o. -y
set VC_LTL=%~dp0\VC-LTL
set YY_THUNKS=%~dp0\YY-Thunks
thunk --os xp --arch x86 -- --release
pause
