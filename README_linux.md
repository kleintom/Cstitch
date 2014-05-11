There is an Arch AUR package of cstitch available at
https://aur.archlinux.org/packages/cstitch-git/

The binary linux version of Cstitch available at
http://sourceforge.net/projects/cstitch/files/Cstitch/ was compiled
against a 32-bit static build of Qt 5.2.0 on Fedora 20 (so it
shouldn't matter if your system is 32 or 64 bit).  I've run the binary
on Fedora 20, Fedora 18, and Debian squeeze; Debian lenny is known to
be too early.

If the binary doesn't run on your linux system, your best bet is to
compile the source - see INSTALL_linux.md for instructions.