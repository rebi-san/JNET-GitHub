@echo off

dir /a:-r-d /b *.dds > ToTouchList.txt

for /f "eol=; tokens=1*" %%i in (ToTouchList.txt) do ..\..\..\Tools\Touch %%i

del ToTouchList.txt
