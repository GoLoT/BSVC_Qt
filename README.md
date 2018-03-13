BSVC: A Microprocessor Simulation Framework
===========================================
BSVC is a microprocessor simulation framework written in C++ and Tcl/Tk.
It was developed as a senior design project at North Carolina State University by Bradford W. Mott (see the Docs/Credits.txt file for copyright information and in the Credits section below).

# BSVC Qt

This version of BSVC is currently under development. It removes the Tcl/Tk interface and replaces it with a new and improved interface written in Qt.
Parts of the original BSVC have been rewritten to support the new interface while keeping almost all of the original code intact.

Currently it only supports (hopefully) the sim68000 version of the original project.

The project has to be dinamically compiled agains the Qt 5.6 libraries.

# Contact

Main repository: https://github.com/GoLoT/BSVC_Qt

# Previous versions

Information on previous versions can be found inside the Docs/ directory.

# License 

BSVC2.2 by Dan Cross is licensed strictly under LGPL 2.1 (Full license text can be found in Docs/LICENSE.txt).
This project is also licensed under LGPL 2.1 (Full license text can be found in LICENSE.txt).

Qt is required to compile this project. Qt 5.6 is the last version compatible with LGPL 2.1 and also the first version including the features required by this project. The project must be compiled with a Qt 5.6 version with an open-source license.

# Credits

BSVC was originally written by Bradford W. Mott and came with this copyright notice:

BSVC is Copyright (C) 1993 - 1998 by Bradford W. Mott.

The assembler carried this notice:

Copyright 1990-1991 North Carolina State University. All Rights Reserved.

The assembler seems to have been originally written by Paul Franzon and Tom Miller, with changes by Tan Phan in 1991 and Bradford Mott in 1994.

MC68360 support was provded by Jorand Didier.  Much work was done by Xavier Plasencia at SDSU.

On April 29, 2015, BSVC was relicensed under the GNU Public License, version 2.1.

On March, 2018, it was partially rewritten with Qt support and licensed under LGPL 2.1.

# Installing Qt

Qt can be downloaded from: https://www.qt.io/download-qt-installer
Follow the steps in the installer tool to set up the Qt environment and install the Qt 5.6 version required for this project.

# Compiling the project

The project includes a Qmake .pro file that automatically sets up the enviroment and generates Makefiles for the current Qt version and toolchain.

Download the source files and run the qmake tool:

```
cd /path/to/project
qmake
```
Run either make or Nmake:

```
make
```
Currently there is no installer script provided. You can run /GUI/GUI to start the application.