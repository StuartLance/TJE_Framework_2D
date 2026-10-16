@echo off
REM Builds the game in Release and creates the delivery zip: package.bat <NIA>
setlocal
if "%~1"=="" (
	echo Usage: package.bat NIA
	exit /b 1
)
set NAME=Jailbreak_2D_%~1
set OUT=dist\%NAME%

cmake -S . -B build || exit /b 1
cmake --build build --config Release --target TJE_Framework_2D || exit /b 1

if exist dist rmdir /s /q dist
mkdir "%OUT%"
copy build\Release\TJE_Framework_2D.exe "%OUT%\Jailbreak.exe" || exit /b 1
if exist build\Release\SDL2.dll (copy build\Release\SDL2.dll "%OUT%\") else (copy build\SDL2.dll "%OUT%\") || exit /b 1
xcopy /e /i /q data "%OUT%\data" || exit /b 1
xcopy /e /i /q src "%OUT%\src" || exit /b 1
copy README.TXT "%OUT%\" || exit /b 1
copy screenshot_menu.png "%OUT%\" || exit /b 1
copy screenshot_game.png "%OUT%\" || exit /b 1

powershell -NoProfile -Command "Compress-Archive -Path '%OUT%\*' -DestinationPath 'dist\%NAME%.zip' -Force" || exit /b 1
echo.
echo Created dist\%NAME%.zip - test it: unzip it and double-click Jailbreak.exe
