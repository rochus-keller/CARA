# Computer Aided Resonance Assignment (CARA)
CARA is a software application for the analysis of NMR spectra and computer aided resonance assignment which is particularly suited for biomacromolecules. Dedicated tools for backbone assignment, side chain assignment, and peak integration support the entire process of structure determination.
For more information about CARA please refer to the official website at http://cara.nmr.ch.

## Download and Installation
CARA is deployed as a compressed single-file executable for all major platforms. See http://cara.nmr.ch/doku.php/cara_downloads. The executable is built from the source code accessible here. Of course you can build the executable yourself if you want (see below for instructions). Since CARA is a single executable, it can just be downloaded and unpacked. No installation is necessary. You therefore need no special privileges to run CARA on your machine. 

## How to Build CARA

### Preconditions
CARA requires Qt4.x. Our single-file executables are static builds based on Qt 4.4.3. But CARA compiles also well with the Qt 4.8 series. It does not yet compile with Qt 5 due to the missing Qt3 support library still in use by CARA. Please also note that we didn't try yet to do 64 bit compilations. If you intend to try it we're interested in the results.

You can download the Qt 4.4.3 source tree from here: http://download.qt.io/archive/qt/4.4/qt-all-opensource-src-4.4.3.tar.gz

The source tree also includes documentation and build instructions.

If you intend to do static builds on Windows without dependency on C++ runtime libs and manifest complications, follow the recommendations in this post: http://www.archivum.info/qt-interest@trolltech.com/2007-02/00039/Fed-up-with-Windows-runtime-DLLs-and-manifest-files-Here's-a-solution.html

Here is the summary on how to do independent Qt Win32 static builds:
1. in Qt/mkspecs/win32-msvc2005/qmake.conf replace MD with MT and MDd with MTd
2. in Qt/mkspecs/features clear the content of the two embed_manifest_*.prf files (but don't delete the files)
3. run configure -release -static -platform win32-msvc2005

### Build Steps
Follow these steps if you inted to build CARA yourself (don't forget to meet the preconditions before you start):

1. Create a directory; let's call it BUILD_DIR
2. Download the CARA source code from https://github.com/rochus-keller/CARA/archive/master.zip and unpack it to the BUILD_DIR; rename "CARA-Master" to "CARA".
3. Download the NAF source code from https://github.com/rochus-keller/NAF/archive/master.zip and unpack it to the BUILD_DIR; rename "NAF-Master" to "NAF".
4. Create the subdirectory "Lua" in BUILD_DIR; download the modified Lua source from http://cara.nmr-software.org/download/Lua_5.1.5_CARA_modified.tar.gz and unpack it to the subdirectory.
5. Create the subdirectory "Newmat" in BUILD_DIR; download the modified Newmat source from http://cara.nmr-software.org/download/Newmat_10_CARA_modified.tar.gz and unpack it to the subdirectory.
6. Create the subdirectory "Expat" in BUILD_DIR; download the modified Expat source from http://cara.nmr-software.org/download/Expat_2_CARA_modified.tar.gz and unpack it to the subdirectory.
7. Goto the BUILD_DIR/CARA subdirectory and execute `QTDIR/bin/qmake Cara2.pro` (see the Qt documentation concerning QTDIR).
8. Run make; after a couple of minutes you will find the executable in the tmp subdirectory.

Alternatively you can open Cara2.pro using QtCreator and build it there.

## Support
If you need support or would like to post issues or feature requests please use our forum: http://forum.cara.nmr.ch/

Documentation is available under http://cara.nmr.ch/doku.php/cara_documentation. 


