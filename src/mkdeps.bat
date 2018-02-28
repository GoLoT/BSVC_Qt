
set gccpath=%1
set file=%2
set flags=%3
set flags=%flags:"=%
set dir=%file:"=%
set dir=%dir:\=/%
FOR /F "tokens=* USEBACKQ" %%F IN (`dirname %dir%`) DO (
SET dir=%%F/
)
ECHO %dir%

%gccpath% -MM -MG %flags% %file% | sed -e "s@\(.*\).o:@%dir%\\1.o %dir%\\1.d:@" > %4
