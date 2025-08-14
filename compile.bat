@echo off

:: enabledelayedexpansion: Enabled Delayed variable extension allows the latest value of a variable to be dynamically obtained during script execution (rather than being fixed during script parsing). 
:: Use! variable! Replace the traditional %variable% with grammar
setlocal enabledelayedexpansion

:: Get the path of slangc.exe
set "SLANGC="
:: Prioritize searching from the environment variable VULKAN_SDK
if defined VULKAN_SDK (
    set "SLANGC=%VULKAN_SDK%\bin\slangc.exe"
    if exist "!SLANGC!" (
        goto :found
    )
)

:: If the VULKAN_SDK is not defined, try to find the VulkanSDK under Program Files
for /f "tokens=1*" %%a in ('dir /b /ad "%ProgramFiles%\NVIDIA Corporation\VulkanSDK\*"') do (
    set "VULKAN_SDK=%ProgramFiles%\NVIDIA Corporation\VulkanSDK\%%a"
    set "SLANGC=!VULKAN_SDK!\bin\slangc.exe"
    if exist "!SLANGC!" (
        goto :found
    )
)

echo Error: Could not find slangc.exe. Please ensure Vulkan SDK is installed and VULKAN_SDK environment variable is set.
exit /b 1

:found

:: Check the parameters
if "%~1"=="" (
    echo Usage: %0 [source_file] [output_file]
    echo Example: %0 my_shader.slang my_shader.spv
    exit /b 1
)

set "SRC=%~1"
set "DST=%~2"

:: Execute the compilation command
echo Using slangc: !SLANGC!
echo Compiling !SRC! -> !DST!

"!SLANGC!" "!SRC!" ^
    -target spirv ^
    -profile spirv_1_4 ^
    -emit-spirv-directly ^
    -fvk-use-entrypoint-name ^
    -entry vertMain ^
    -entry fragMain ^
    -o "!DST!"

endlocal