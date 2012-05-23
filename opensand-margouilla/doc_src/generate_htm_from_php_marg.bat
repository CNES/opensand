echo off 
set PHPDIR="C:\Program Files\EasyPHP\php"
set TARGETDIR="..\doc"

PATH=%PATH%;%PHPDIR%
echo "."
echo "Generating Marg www site and documentation from php and content_*.htm"

echo on 

php.exe -q generic_page.php content_about.htm >%TARGETDIR%\about.htm
php.exe -q generic_page.php content_components.htm >%TARGETDIR%\components.htm
php.exe -q generic_page.php content_license.htm >%TARGETDIR%\license.htm
php.exe -q generic_page.php content_documentation.htm >%TARGETDIR%\documentation.htm
php.exe -q generic_page.php content_design_runtime.htm >%TARGETDIR%\design_runtime.htm
php.exe -q generic_page.php content_tutorial_new_bloc.htm >%TARGETDIR%\tutorial_new_bloc.htm
php.exe -q generic_page.php content_exemple_msg_bloc.htm >%TARGETDIR%\exemple_msg_bloc.htm
php.exe -q generic_page.php content_download.htm >%TARGETDIR%\download.htm
php.exe -q generic_page.php content_mailing_list.htm >%TARGETDIR%\mailing_list.htm
php.exe -q generic_page.php content_contact.htm >%TARGETDIR%\contact.htm

copy index.php %TARGETDIR%\index.php
copy %TARGETDIR%\about.htm %TARGETDIR%\index.htm
copy %TARGETDIR%\about.htm %TARGETDIR%\index.html

echo off 
echo .
echo Copying icons files
mkdir %TARGETDIR%\icon
copy icon\*.* %TARGETDIR%\icon\


pause

