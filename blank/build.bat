@ECHO OFF

CLS

SET buildDir=TEMP
SET picoPath=C:/Program Files/Raspberry Pi/Pico SDK v1.5.1

IF ""%envInit%""=="""" (
	ECHO Setting up enviroment...
	SET envInit=true
	CALL "%picoPath%/pico-env.cmd" >NUL
)

IF NOT EXIST %buildDir% (
	MKDIR %buildDir%
)

PUSHD %buildDir%
cmake -S .. -DPICO_PATH="%picoPath%" -DCMAKE_BUILD_TYPE=Debug
cmake --build .
POPD
