QtCBM
=====

GUI Frontend for the CBM utils

This project is intended as a modern replacement for the GUI4CBM4WIN tool that ships with the CBM utils.
It features a modern looking UI and a progress bar for copying disk images.

This fork is based on: https://github.com/mveinot/QtCBM


QtCBM depends on:

* https://github.com/qtproject/qt-solutions/tree/master/qtsingleapplication

and of course

* https://github.com/OpenCBM/OpenCBM


Building (under Linux - Debian 12.7 here)

* git clone this repository
* cd QtCBM/diskimage && cp Makefile.unix Makefile
* make
* cd ..
* qmake -makefile QtCBM.pro
* make
* ./QtCBM

Alternatively you may install/run "qtcreator" and load "QtCBM.pro".


Recently fixed/changed:

* Rename works now and lowercase target filenames are properly uppercased
* cbmforng options for 40 Tracks, verify, no bump and original pattern formatting added
* Some fixes to displayed values
* More sane directory display (blocks, filename, type and THEN "size").
* root filesystem default instead of homedir
* option for error map added

TODO:

* Format regex fixing - entering more than 2 chars for the ID or 16 for name will lead to failure (by cbmctrl)
* add retry-count option?
* Remove MORSE stuff? Currently its already partially executed when hovering over it in the pulldown menu. Also - WHAT does it actually morse and why?
