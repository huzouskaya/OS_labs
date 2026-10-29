@echo off
chcp 65001 > nul
echo Git Update
git pull щorigin lab2

echo Make
del /Q test_runner.exe *.o 2>nul
mingw32-make

echo Execution
test_runner.exe
pause 
