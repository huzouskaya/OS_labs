@echo off
chcp 65001 > nul
echo Git Update
git pull

echo Make
del /Q hello.exe hello 2>nul
del /Q hello.o 2>nul
mingw32-make

echo Execution
hello.exe
pause 
