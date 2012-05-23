echo off 
set DOXYGENDIR="C:\Program Files\doxygen\bin"
set TARGETDIR="..\doc\api\htm"

PATH=%PATH%;%DOXYGENDIR%
echo "."
echo "Generating Marg API documentation from sources"

echo on 

doxygen.exe api_doc.dox
mkdir %TARGETDIR%\icon
copy icon\titre_margouilla.png %TARGETDIR%\icon
echo off 

pause

