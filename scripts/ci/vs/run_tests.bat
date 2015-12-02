cd %APPVEYOR_BUILD_FOLDER%\tests
set TESTS_PLATFORM=%PLATFORM%
if "%PLATFORM%" equ "x86" set TESTS_PLATFORM=Win32
FOR /D %%G IN (*) DO ( 
    echo %APPVEYOR_BUILD_FOLDER%\tests\%%G
    cd %APPVEYOR_BUILD_FOLDER%\tests\%%G
    FOR /D %%E IN (*) DO ( 
        echo %APPVEYOR_BUILD_FOLDER%\tests\%%G\%%E
        cd %APPVEYOR_BUILD_FOLDER%\tests\%%G\%%E
        msbuild %%E.sln /p:Configuration=Debug /p:Platform=%TESTS_PLATFORM%
        cd bin
        vstest.console /logger:Appveyor %%E_debug.exe
        if "%errorlevel%" neq "0" EXIT /B 1
    )
)
cd ..
