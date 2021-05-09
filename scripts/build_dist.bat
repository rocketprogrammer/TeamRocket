@echo off
cd ../build-TeamRocket-Desktop_x86_windows_msvc2019_pe_32bit-Release/release

:main
"C:\Program Files (x86)\CQtDeployer\1.4\cqtdeployer.bat" -bin TeamRocket.exe -qmake C:\Qt\5.15.2\msvc2019\bin\qmake.exe
pause
goto :main