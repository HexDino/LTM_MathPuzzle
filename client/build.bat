@echo off
REM Build script for Math Puzzle Client on Windows

echo ================================
echo Math Puzzle Game - Qt Client Build
echo ================================
echo.

REM Check if qmake is in PATH
where qmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: qmake not found in PATH
    echo Please install Qt and add qmake to your PATH
    echo Example: set PATH=C:\Qt\5.15.2\mingw81_64\bin;%%PATH%%
    pause
    exit /b 1
)

echo Found qmake: 
qmake --version
echo.

REM Clean previous build
if exist Makefile (
    echo Cleaning previous build...
    nmake clean 2>nul
    mingw32-make clean 2>nul
    del Makefile* 2>nul
    echo.
)

REM Run qmake
echo Running qmake...
qmake MathPuzzleClient.pro
if %ERRORLEVEL% neq 0 (
    echo ERROR: qmake failed
    pause
    exit /b 1
)
echo.

REM Determine which make to use
where nmake >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo Building with nmake (MSVC)...
    nmake
    set MAKE_RESULT=%ERRORLEVEL%
) else (
    where mingw32-make >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        echo Building with mingw32-make (MinGW)...
        mingw32-make
        set MAKE_RESULT=%ERRORLEVEL%
    ) else (
        echo ERROR: No make tool found (nmake or mingw32-make)
        pause
        exit /b 1
    )
)

if %MAKE_RESULT% neq 0 (
    echo.
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ================================
echo Build successful!
echo ================================
echo.
echo To run the client:
if exist release\MathPuzzleClient.exe (
    echo   release\MathPuzzleClient.exe
) else if exist debug\MathPuzzleClient.exe (
    echo   debug\MathPuzzleClient.exe
) else if exist MathPuzzleClient.exe (
    echo   MathPuzzleClient.exe
)
echo.
pause


