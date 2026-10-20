# Computer Aided Resonance Assignment (CARA)
CARA is a software application for the analysis of NMR spectra and computer aided resonance assignment which is particularly suited for biomacromolecules. Dedicated tools for backbone assignment, side chain assignment, and peak integration support the entire process of structure determination.
For more information about CARA please refer to the official website at http://cara.nmr.ch.

NOTE: This is the Qt5 branch which includes a minimal Qt3 compatibility layer (a subset of the one in Qt4) on top of Qt5. I hope for community tests so some day it will replace the
official release; send me feedback to the issue tracker or my email.

## Download and Installation

NOTE: For this Qt5 branch there are no precompiled versions so far. If you need one or have problems building CARA yourself, post an issue.

See http://cara.nmr.ch/doku.php/cara_downloads for other precompiled versions.

## How to Build CARA

### Preconditions
This CARA version requires Qt5. It is supposed to compile and run on both 32 and 64 bit systems.

The source tree also includes documentation and build instructions.

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


