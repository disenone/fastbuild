@echo off
FBuild.exe All-x64-Release
if "%errorlevel%" neq "0" goto :failedend
FBuild.exe All-x64-Debug
if "%errorlevel%" neq "0" goto :failedend
FBuild.exe solution
if "%errorlevel%" neq "0" goto :failedend

set cur_folder=%~dp0
set tmp_folder=%cur_folder%\..\tmp
set outputs=x64-Release x64-Debug

@REM 把 exe 复制到 bin
(for %%f in (%outputs%) do (
    mkdir %tmp_folder%\%%f\bin 1>NUL 2>NUL
    xcopy /y /q /s /k /d /h %tmp_folder%\%%f\Tools\FBuild\FBuild\FBuild.exe %tmp_folder%\%%f\bin > nul
    xcopy /y /q /s /k /d /h %tmp_folder%\%%f\Tools\FBuild\FBuildCoordinator\FBuildCoordinator.exe %tmp_folder%\%%f\bin > nul
    xcopy /y /q /s /k /d /h %tmp_folder%\%%f\Tools\FBuild\FBuildWorker\FBuildWorker.exe %tmp_folder%\%%f\bin > nul
))

@REM 把 exe 复制到外部目录
set bin_folder=%cur_folder%\..\..\111ue
mkdir %bin_folder% 1>NUL 2>NUL
xcopy /y /q /s /k /d /h %tmp_folder%\x64-Release\bin\FBuild.exe %bin_folder% > nul
xcopy /y /q /s /k /d /h %tmp_folder%\x64-Release\bin\FBuildCoordinator.exe %bin_folder% > nul
xcopy /y /q /s /k /d /h %tmp_folder%\x64-Release\bin\FBuildWorker.exe %bin_folder% > nul

echo;
echo Build Succeed !!!
goto :end

:failedend
echo;
echo Build Failed !!!

:end
