@echo off
chcp 65001 > nul
echo Git Update
git pull origin lab2_meson

echo Make
if exist build rmdir /S /Q build
meson setup build
meson compile -C build

echo Execution
build\test_runner.exe
pause 
