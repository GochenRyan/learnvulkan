@echo off
setlocal enabledelayedexpansion

:: Find slangc.exe (prioritize VULKAN_SDK env var)
set "SLANGC="
if defined VULKAN_SDK (
    set "SLANGC=%VULKAN_SDK%\bin\slangc.exe"
    if exist "!SLANGC!" (
        goto :found
    )
)

:: Try Program Files (NVIDIA VulkanSDK) as fallback
for /f "tokens=1*" %%a in ('dir /b /ad "%ProgramFiles%\NVIDIA Corporation\VulkanSDK\*" 2^>nul') do (
    set "VULKAN_SDK=%ProgramFiles%\NVIDIA Corporation\VulkanSDK\%%a"
    set "SLANGC=!VULKAN_SDK!\bin\slangc.exe"
    if exist "!SLANGC!" (
        goto :found
    )
)

echo Error: Could not find slangc.exe. Please ensure Vulkan SDK is installed and VULKAN_SDK environment variable is set.
exit /b 1

:found

:: Check required parameters (source and destination)
if "%~1"=="" (
    echo Usage: %~nx0 [source_file] [output_file] [optional extra args...]
    echo Example: %~nx0 my_shader.slang my_shader.spv -D USE_ORM
    exit /b 1
)

if "%~2"=="" (
    echo Error: missing output file parameter.
    echo Usage: %~nx0 [source_file] [output_file] [optional extra args...]
    exit /b 1
)

set "SRC=%~1"
set "DST=%~2"

:: Collect remaining arguments (forward them to slangc)
set "EXTRA_ARGS="
shift
shift
:collect_loop
if "%~1"=="" goto :collected
    set "EXTRA_ARGS=!EXTRA_ARGS! %~1"
    shift
    goto :collect_loop
:collected

:: Trim leading space in EXTRA_ARGS (simple)
if defined EXTRA_ARGS (
    if "!EXTRA_ARGS:~0,1!"==" " set "EXTRA_ARGS=!EXTRA_ARGS:~1!"
)

echo Using slangc: !SLANGC!
echo Compiling: "!SRC!" -> "!DST!"
if defined EXTRA_ARGS (
    echo Extra args: !EXTRA_ARGS!
)

:: Build and run the slangc command (for SPIR-V)
:: Note: EXTRA_ARGS are forwarded as-is, so you can pass -DUSE_ORM or -D USE_ORM etc.
"!SLANGC!" "!SRC!" !EXTRA_ARGS! ^
    -target spirv ^
    -profile spirv_1_4 ^
    -emit-spirv-directly ^
    -fvk-use-entrypoint-name ^
    -entry vertMain ^
    -entry fragMain ^
    -o "!DST!"

set "RC=%ERRORLEVEL%"

if %RC% NEQ 0 (
    echo slangc returned error code %RC%.
) else (
    echo Compiled successfully.
)

endlocal
exit /b %RC%
