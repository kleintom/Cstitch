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

# Package icons needed by cstitch into a zip file.

import os.path, re, zipfile

iconDirectory = os.path.dirname(os.path.realpath(__file__))
srcDirectory = os.path.realpath(os.path.join(iconDirectory, ".."))

resourceFile = open(os.path.join(srcDirectory, "iconResources.qrc"), 'r')
resourceString = resourceFile.read() # read entire contents
resourceFile.close()

zipFileName = os.path.join(iconDirectory, "icons.zip")
zipFile = zipfile.ZipFile(zipFileName, 'w')
pngRE = r'icons/(.*\.png)'
matches = re.findall(pngRE, resourceString)
for pngFile in matches:
    zipFile.write(pngFile)

zipFile.close()
print "\n  Wrote " + str(len(matches)) + " files to " + zipFileName + ".\n"





