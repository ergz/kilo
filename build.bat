@echo off

rem when you have a working posix system in windows
rem gcc kilo.c -o kilo -Wall -Wextra -pedantic -std=c99

rem, build from wsl
rem location of the wsl bash
set BASH=C:\Windows\System32\bash.exe
set WSL_BASH=C:\Windows\System32\wsl.exe -d Ubuntu-20.04 -e

%BASH% -c "gcc -g -Wall -Wextra -pedantic -std=c99 kilo.c -o kilo.exe"
