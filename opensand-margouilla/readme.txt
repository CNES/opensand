/**********************************************************************
**
** Margouilla project
**
** http://www.margouilla.com
**
**********************************************************************/


I/ What is Margouilla ? 
-----------------------
Margouilla is the codename for the Open Network Toolkit. Please call it 
Marg. By the way, a Margouila is a funny small lezard that lives in sunny
countries. 
Have a look at our logo. 
 


II/ What is the Open Network Toolkit ?  
--------------------------------------
The Open Network Toolkit is a set of tools dedicated to Communication 
Protocols. Thanks to the Open Network Toolkit it is thus possible to specify
a protocol, evaluate its performance, bandwidth consumption, with a lot 
of measurement points then target a final product running on many common 
platform. 
 


III/ Components 
---------------
The Open Network Toolkit is composed by:
- A C++ runtime that provides platform independant messaging, threads 
  management, timers, traces...
- A set of common protocols ready to use (IP/ATM/Ethernet layers...).
- A graphical editor with SDL design style.
 
 

IV/ Platforms 
-------------
The Runtime as been tested on :
- MS/Windows - 95, 98, NT, and 2000: Microsoft Visual C++ 5/6, Cygwin, Mingw 
- Unix - Linux, HPUX, Sun : gcc.

Majority of common protocols run on the same platform as the Runtime. Few are
platform dependant.

The Gaphical Editor works on :
- MS/Windows - 95, 98, NT, and 2000 (Compiled with Microsoft Visual C++ 5/6)
- Unix - Linux, HPUX, Sun : gcc.
 
 

V/ License  
----------
The toolkit is provided under LGPL license.
 


VI/ Distributions
-----------------
Margouilla is distributed through :
- full source tarball         : you have to compile the library, the exemples and
                                generate the documentation.
                                You need a c++ compiler, php, doxygen.
- ready to use source tarball : you have to compile the library and the exemples.
                                Documentation is already uptodate. 
                                You need a c++ compiler.
- ready to use binary tarball : Nothing to do. Ready to be used in your project.
        
		                        

VII/ How to compile 
-------------------

1/ Under Unixes (Linux, Cygwin, MinGw, HPux...)
-----------------------------------------------
It is as simple as typing:
#./configure
#make


2/ Under Windows
----------------
To build the documentation, set up php and doxygen.
Update DOXYGENDIR="C:\Program Files\doxygen\bin", in
./doc_src/generate_htm_api_from_src_marg.bat, with the correct Doxygen path.
Update PHPDIR="C:\Program Files\EasyPHP\php", in
./doc_src/generate_htm_from_php_marg.bat, with the correct php path.
Then double-click on ./doc_src/generate_htm_api_from_src_marg.bat, and
./doc_src/generate_htm_from_php_marg.bat.

To build the libraries and exemples, double-click on ./win32/margouilla.dsw.
Choose Build/Batch build in the Menu, and Rebuild all.

This will generate the runtime library, the common blocs library and few tests.
Once compiled, the libraries are located in ./lib, the headers in ./include, and
the binaries in ./bin.




Have fun,

