@echo off

echo %DATE% %TIME%

rem when you have a working posix system in windows
rem gcc kilo.c -o kilo -Wall -Wextra -pedantic -std=c99

rem, build from wsl
rem location of the wsl bash
rem set WSL_BASH=C:\Windows\System32\wsl.exe -d Ubuntu-20.04 -e
set BASH=C:\Windows\System32\bash.exe

echo ~~~~ Building from bat file ~~~~

%BASH% -c "gcc -g -Wall -Wextra -pedantic -std=c99 code/kilo.c -o build/kilo"
