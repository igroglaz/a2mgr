a2mgr
=====

This is client for Rage of Mages 2 - https://rom2.ru


=====

Edit data file (spells, items etc) there:

\postbuild\world\data.xml

## How to build in VS Code

1. Install VS Code
2. Install MSVC Build Tools
3. In VS Code, install extensions: "C/C++" and "CMake"
4. Go to CMake menu on the left, Project Status -> Configure -> Kit: select x86 version of the MSVC Build Tools
5. Run "CMake: Configure" and "CMake: Build" from the top menu
6. The built allods2.exe and a2mgr.dll will be in "postbuild/release/"
