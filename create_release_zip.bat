@echo off
SET BUILD_DIR=RaytracerReleasePkg
SET RUNTIMEDATA=RuntimeData
SET SHADERBUILD=ShaderBuild
SET ZIP_FILE=%BUILD_DIR%.zip

REM Remove old files
rmdir /S /Q %BUILD_DIR%
del /Q %ZIP_FILE%

REM Copy source files
mkdir %BUILD_DIR%
mkdir %BUILD_DIR%\%RUNTIMEDATA%
mkdir %BUILD_DIR%\%SHADERBUILD%
xcopy /Y BuildOutput\*.* %BUILD_DIR%
xcopy /E /Y %RUNTIMEDATA%\*.* %BUILD_DIR%\%RUNTIMEDATA%
xcopy /E /Y %SHADERBUILD%\*.* %BUILD_DIR%\%SHADERBUILD%

REM Delete files we don't want to release
del /Q %BUILD_DIR%\*.pdb
del /Q %BUILD_DIR%\*.ipdb
del /Q %BUILD_DIR%\*.iobj
del /Q %BUILD_DIR%\*.ilk
del /Q %BUILD_DIR%\*_Debug.exe

REM Zip everything up
powershell.exe -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::CreateFromDirectory('%BUILD_DIR%', '%ZIP_FILE%'); }"

REM Delete directory
rmdir /S /Q %BUILD_DIR%
