@REM start ..\tmp\x64-Release\Tools\FBuild\FBuildCoordinator\FBuildCoordinator.exe
@REM start ..\tmp\x64-Release\Tools\FBuild\FBuildWorker\FBuildWorker.exe -console


start ..\tmp\x64-Debug\Tools\FBuild\FBuildCoordinator\FBuildCoordinator.exe
start ..\tmp\x64-Debug\Tools\FBuild\FBuildWorker\FBuildWorker.exe -console -coordinator=192.168.1.57 -cpus=2
