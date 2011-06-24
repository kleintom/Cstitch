#!/usr/bin/python

#
# Copyright 2011 Tom Klein.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Automate the packing/archiving process for Windows and Linux

import sys, re, os, shutil, subprocess

if len(sys.argv) < 2 or sys.argv[1] == "--help" or len(sys.argv) != 2:
    print "\n Usage: " + sys.argv[0],
    print "program_file\n"
    sys.exit(-1)

binary = sys.argv[1]

class archiver:
    """Base class for classes that create program archives."""

    def __init__(self, binary, readmes):

        self.readmeDir = "doc/web/"
        self.versionsDir = "versions/"
        # figure out the version of our binary (or quit)
        self.version = ""
        versionRE = re.compile(r'setProgramVersion\("([0-9]+\.[0-9]+\.[0-9]+)\.[0-9]+"')
        for line in open("main.cpp"):
            found = versionRE.search(line)
            if found:
                self.version = found.group(1)
                print "  Version: " + self.version
                break
        else:
            print "Couldn't determine version!"
            sys.exit(-1)
        self.binary = binary
        self.readmes = readmes

class windowsArchiver(archiver):
    """Create a Windows archive for the input binary and README files."""
    
    def __init__(self, binary, readmes):
        
        archiver.__init__(self, binary, readmes)

    def archive(self):

        archiveDir = "Cstitch_" + self.version
        thisVersionDir = os.path.join(self.versionsDir, archiveDir)
        if os.path.exists(thisVersionDir):
            shutil.rmtree(thisVersionDir)
        os.makedirs(thisVersionDir)

        # copy the READMES to thisVersionDir
        for readme in self.readmes:
            # need carriage returns (\r\n) for Windows
            readmeString = open(os.path.join(self.readmeDir, readme)).read()
            newString = readmeString.replace('\n', '\r\n')
            newFile = open(os.path.join(thisVersionDir, readme + ".txt"), 'w')
            newFile.write(newString)
            newFile.close()

        newBinaryName = "cstitch_" + self.version + "_win32.exe"
        newBinaryLocation = os.path.join(thisVersionDir, newBinaryName)
        shutil.copy2(self.binary, newBinaryLocation)

        packStatus = subprocess.call(["upx", newBinaryLocation])
        if packStatus != 0:
            if packStatus == 2:
                print self.binary + " is already packed, so quitting..."
            else:
                print "Unknown packing error on " + newBinaryLocation + ", so quitting..."
            sys.exit(-1)

        # create the zip archive
        os.chdir(self.versionsDir)
        zipFile = "cstitch_" + self.version + "_win32.zip"
        # zip will add files to an archive by default, so make sure we don't
        # have an old version sitting around
        if os.path.exists(zipFile):
            os.remove(zipFile)
        subprocess.call("zip " + zipFile + " " + archiveDir + "/*",
                        shell=True)
        print "  Created " + zipFile

class linuxArchiver(archiver):
    """Create a linux archive for the input binary and README files."""
    
    def __init__(self, binary, readmes):
        
        archiver.__init__(self, binary, readmes)

    def archive(self):

        # change the loader from lsb to libc
        print "  Patching the binary..."
        elfStatus = subprocess.call(["patchelf", "--set-interpreter",
                                     "/lib/ld-linux.so.2", self.binary])
        if elfStatus != 0:
            print "Patch error on " + self.binary + ", so quitting..."
            sys.exit(-1)

        # pack the binary
        print "  Packing the binary..."
        packStatus = subprocess.call(["upx", self.binary])
        if packStatus != 0:
            if packStatus == 2:
                print self.binary + " is already packed, so quitting..."
            else:
                print "Unknown packing error on " + self.binary + ", so quitting..."
            sys.exit(-1)

        # create the archive
        # tar czf cstitch_0.9.1_linux32.tar.gz --transform='s@^@cstitch_0.9.1/@' cstitch -C doc/web/ README_linux README
        archiveName = "cstitch_" + self.version + "_linux32.tar.gz"
        subprocess.call(["tar", "czf", archiveName,
                         "--transform=s@^@cstitch_" + self.version + "/@",
                         self.binary,
                         "-C", self.readmeDir] + self.readmes)
        ##shutil.move isn't overwriting, so copy and delete
        ##shutil.move(archiveName, self.versionsDir)
        shutil.copy2(archiveName, self.versionsDir)
        os.remove(archiveName)
        print "  Created " + archiveName

## main #######################################################################

if binary.endswith("exe"):
    binaryArchive = windowsArchiver(binary, [])
else:
    binaryArchive = linuxArchiver(binary, ["README_linux"])

binaryArchive.archive()
