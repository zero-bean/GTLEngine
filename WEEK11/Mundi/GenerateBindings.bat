@echo off
REM ============================================================
REM Mundi Engine - Lua Binding Code Generator
REM ============================================================

echo.
echo ============================================================
echo    Mundi Engine - Lua Binding Code Generator
echo ============================================================
echo.

REM Change to script directory
cd /d "%~dp0"

REM Try to find Python in the following order:
REM 1. Embedded Python in Tools/Python/
REM 2. System Python in PATH

set PYTHON_CMD=

REM Check for embedded Python first
if exist "BuildTools\Python\python.exe" (
    set PYTHON_CMD=BuildTools\Python\python.exe
    echo [INFO] Using embedded Python: BuildTools\Python\python.exe
    echo.

    REM Auto-setup pip for embedded Python if not already done
    if not exist "BuildTools\Python\Lib\site-packages\pip" (
        echo [INFO] Setting up pip for embedded Python...

        REM Enable site-packages by uncommenting import site in ._pth file
        for %%f in (BuildTools\Python\python*._pth) do (
            findstr /C:"import site" "%%f" >nul
            if errorlevel 1 (
                echo import site>> "%%f"
                echo [OK] Enabled site-packages in %%f
            )
        )

        REM Download and install pip
        if not exist "BuildTools\Python\get-pip.py" (
            echo [INFO] Downloading get-pip.py...
            powershell -Command "Invoke-WebRequest -Uri https://bootstrap.pypa.io/get-pip.py -OutFile BuildTools\Python\get-pip.py"
        )

        if exist "BuildTools\Python\get-pip.py" (
            echo [INFO] Installing pip...
            %PYTHON_CMD% BuildTools\Python\get-pip.py --no-warn-script-location
            echo.
        )
    )
) else (
    REM Check if system Python is available
    python --version >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] Python not found!
        echo.
        echo Please either:
        echo   1. Install embedded Python to BuildTools\Python\ (see BuildTools\PYTHON_SETUP.md)
        echo   2. Install Python 3.7+ and add it to PATH
        echo.
        pause
        exit /b 1
    )
    set PYTHON_CMD=python
    echo [INFO] Using system Python
    echo.
)

REM Verify Python version
%PYTHON_CMD% --version
echo.

REM Auto-install required packages from requirements.txt
if exist "BuildTools\requirements.txt" (
    echo [INFO] Installing Python dependencies from requirements.txt...
    %PYTHON_CMD% -m pip install --quiet --upgrade -r BuildTools\requirements.txt
    if errorlevel 1 (
        echo [WARNING] Failed to install some packages. Trying without --quiet...
        %PYTHON_CMD% -m pip install --upgrade -r BuildTools\requirements.txt
        if errorlevel 1 (
            echo [ERROR] Failed to install required packages!
            echo Please check your Python and pip installation.
            pause
            exit /b 1
        )
    )
    echo [OK] All dependencies installed
    echo.
) else (
    echo [WARNING] BuildTools\requirements.txt not found!
    echo [INFO] Trying to install jinja2 directly...
    %PYTHON_CMD% -m pip install jinja2
    echo.
)

echo [1/3] Running code generator...
%PYTHON_CMD% BuildTools\CodeGenerator\generate.py --source-dir Source\Runtime --output-dir Generated

if errorlevel 1 (
    echo.
    echo [ERROR] Code generation failed!
    pause
    exit /b 1
)

echo.
echo [2/3] Checking generated files...
if not exist "Generated\*.generated.cpp" (
    echo [WARNING] No .generated.cpp files found!
) else (
    echo [OK] Generated files found in Generated\ folder
)

echo.
echo [3/3] Done!
echo.
echo ============================================================
echo You can now build the project in Visual Studio.
echo Generated files are in: Generated\
echo ============================================================
echo.

pause
