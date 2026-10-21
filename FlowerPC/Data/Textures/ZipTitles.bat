::-----------------------------------------------------------------------------
:: Notes:  Zips all the textures used in the opening titles sequence.
::         They must be manually checked in and out of perforce.
::
:: Author: John Edwards
:: Date C: 1-20-2009
::-----------------------------------------------------------------------------

del *.gz
dir /b Title_Epilepsy* > Titles.txt
dir /b Title_Esrb* >> Titles.txt
dir /b Title_SaveMessage* >> Titles.txt
dir /b Title_Warning* >> Titles.txt
dir /b Title_SonyLogo* >> Titles.txt
dir /b Title_SmLogo* >> Titles.txt
dir /b Title_TgcLogo* >> Titles.txt
dir /b Title_BPLogo* >> Titles.txt
dir /b Title_ControlScheme* >> Titles.txt

..\..\..\Tools\lua ZipTitles.lua > ZipTitles2.bat

call ZipTitles2.bat

del ZipTitles2.bat
del titles.txt

if not "%1"=="NOPAUSE" pause
