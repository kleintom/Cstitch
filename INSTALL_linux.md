## Compiling Cstitch from source on Linux ##

1.  Install the QT (gui toolkit) headers and libraries.

    Starting with version 0.9.4, cstitch is built using Qt 5 (I've tested on
5.2.0).  Qt include locations were changed in going from Qt4 to Qt5, so >= 0,9.4
cstitch versions will not compile under Qt4 (though I suspect that if
you simply change the includes back to their Qt4 locations, you would
be very close to compiling).

    On Fedora-like systems the package you're after is probably called
**qt5-qtbase-devel** - it provides the QT header files and pulls in the
required librairies and build tools.  If your distribution does not
yet support Qt5, you may need to download directly from Qt.
     
2.  Download the Cstitch source files.

    It's possible to download the files as a tarball from the project
page (the "snapshot" link at
http://cstitch.git.sourceforge.net/git/gitweb.cgi?p=cstitch/cstitch;a=summary ),
but I recommend you download them instead as a git repository.
Downloading in repository form will make it simple to update to the
latest version at any time.

    To populate the initial git repository (run one time only):
        git clone git://cstitch.git.sourceforge.net/gitroot/cstitch/cstitch

    The git command will create a directory named cstitch (or something
similar) and download the latest version of the Cstitch files into that
directory.

    If you decide you want to download a new version of the Cstitch files
at some later point, just run **git pull** from the cstitch directory
and all files will be updated to the latest version.

3.  Download the program icon files.

    Download the icons zip file from
http://sourceforge.net/projects/cstitch/files/Cstitch/Icons/
and unzip it in the **cstitch/icons/** directory.

4.  Generate the documentation.

    From the cstitch directory run
        doc/createDocs.py

5.  Generate the build files.

    From the cstitch directory run the following:
        qmake -project (generates a cstitch.pro file)
        ./progen.py (adds some options to the cstitch.pro file)
        qmake (generates a Makefile based on the cstitch.pro contents)

6.  Compile.

    From the cstitch directory run **make** (generates the 'cstitch'
executable).  That's it.

7.  (Optional) Install cstitch.desktop.

    **cstitch.desktop** describes how and where cstitch should appear in
application menus.

    #### To install in system folders: ####
Copy your local cstitch executable to a system bin directory contained in your
PATH (such as /usr/bin/ or /usr/local/bin); either cp your executable to
"cstitch" or change the "Exec" label in cstitch.desktop to the name of your
executable.  Copy **icons/cstitch.svg** to the system icons directory (at least
for KDE and Gnome):
        cp icons/cstitch.svg /usr/share/icons/hicolor/scalable/apps/
Copy **cstitch.desktop** to the system desktop files folders (at least for KDE
and Gnome):
        cp cstitch.desktop /usr/share/applications

    You may need to log out and back in to update menus.

    #### To install in your home directory: ####
Copy your local cstitch executable to a directory in your PATH; either cp your
executable to "cstitch" or change the "Exec" label in cstitch.desktop to the
name of your executable.
Copy **icons/cstitch.svg** to wherever you'd like to have it, and update the
"Icon" label in cstitch.desktop to reflect that location using an *absolute*
path with file extension (e.g. /home/user/icon.svg).  Then
        cp cstitch.desktop ~/.local/share/applications

    You may need to log out and back in to update menus.


### A note on translations if you're making changes to the Cstitch base: ###
The translations directory contains pre-generated translation files
cstitch_en.qm and cstitch_it.qm which should work as is for the current code
base, at least if you're using Qt 5.2.

If you would like to generate your own qm files, translation tools are provided
by qt5-qttools-devel in Fedora.

Note that **cstitch_en.ts** contains only plural translations, generated by

    lupdate -no-obsolete -pluralonly cstitch.pro -ts translations/cstitch_en.ts

while **cstitch_it.ts** contains all translations for the cstitch code,
generated by

    lupdate -no-obsolete cstitch.pro -ts translations/cstitch_it.ts

If you want to support a full Italian translation, your **cstitch_it.qm** file
will need to additionally include translations for the builtin parts of Qt used
by cstitch - for example, the "Okay" and "Cancel" buttons on dialogs are strings
built in to Qt, not set by cstitch code.  Qt provides its own translations for
its code, in this case to be found in **qtbase_it.qm** (if you've downloaded the
full source, you should be able to locate qtbase_it.qm in the source, otherwise
you may want to install a package such as qt5-qttranslations (for Fedora) which
provides qm translation files).  Once you've located **qtbase_it.qm** and
generated a new **cstitch_it.ts** file from your new code and linguist, you'll
want to generate a qm file from **cstitch_it.ts** for the cstitch code, say

    lrelease cstitch_it.ts -qm cstitchNoBase_it.qm

and then combine that qm file with **qtbase_it.qm** to produce the final qm
file:

    lconvert -o cstitch_it.qm qtbase_it.qm cstitchNoBase_it.qm

You're now ready to compile cstitch with the updated translations.