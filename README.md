QtCBM
=====

GUI Frontend for the openCBM utils

This project is intended as a modern replacement for the GUI4CBM4WIN tool that ships with the CBM utils.
It features a modern looking UI and a progress bar for copying disk images.

This fork is based on: https://github.com/mveinot/QtCBM


QtCBM depends on:

* https://github.com/qtproject/qt-solutions

and of course

* https://github.com/OpenCBM/OpenCBM


Building (under Linux - Debian 12.8 here)

* git clone https://github.com/qtproject/qt-solutions.git
* cd qt-solutions/qtsingleapplication && qmake && make && cd ../..
* git clone this repository
* cd QtCBM/diskimage && cp Makefile.unix Makefile && make && cd ..
* qmake -makefile QtCBM.pro
* make
* ./QtCBM

Alternatively you may install/run "qtcreator" and load "QtCBM.pro".


Recently fixed/changed:

* Rename works now and lowercase target filenames are properly uppercased
* cbmforng options for 40 Tracks, verify, no bump and original pattern formatting added
* Format regex changes - entering more than 2 chars for the ID or 16 for name will lead to failure (by cbmforng)
* Some fixes to displayed values
* More sane directory display (blocks, filename, type and THEN "size").
* root filesystem default instead of homedir
* option for error map added
* Filerequester enhancement on transfer from CBM (more image types added)
* MORSE removed
* Directory parsing improved to handle header and id better (e.g. 5 byte IDs and headers with spacings). Added Status display on directory reading to display read errors.
* added "--" option to cbmcopy commands before filenames as e.g. "- sumthg! file -" errors out otherwise on cbmcopy
* nibwrite/nibread support added (most options - for special nibwrites revert to cmdline)

TODO:

* Fix nibread/nibwrite progress bar
* Fix crash after trying to call "opencbm status" after validate and initialize (
* filename with "dir art" cannot easily be copied. Take the "scratch" filename option over to here?
* formatng doesnt work with "- diskname -" and the "--" option seems to be missing. Issue raised: https://github.com/OpenCBM/OpenCBM/issues/121
* CBMroutines::stringToPETSCII could use a major overhaul but I definately dont feel like it


Thanx:

* McMartin of the vice-dev team for helping me out on qt issues :)
