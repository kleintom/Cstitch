#
# Copyright 2015 Simon Norberg <simon@pthread.se>.
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

# Turns a dmc.xml file in kxstitch format into cpp code ready to insert into
# colorLists.cpp

import xml.etree.ElementTree as ET


class Floss():
    def __init__(self):
        self.code = None
        self.name = None
        self.red = None
        self.green = None
        self.blue = None

    def toCpp(self):
        return ("  dmc.push_back(floss(%s, \"%s\", triC(%d,%d,%d)));" %
                (self.code, self.name, self.red, self.green, self.blue))

floss_list = list()

tree = ET.parse("dmc.xml")
root = tree.getroot()

for elem in root.getchildren():
    newFloss = Floss()
    for flossElement in elem.getchildren():
        tag = flossElement.tag
        text = flossElement.text
        if tag == "description":
            newFloss.name = text
        elif tag == "name":
            newFloss.code = text
        elif tag == "color":
            for colorElement in flossElement.getchildren():
                if colorElement.tag == "red":
                    newFloss.red = int(colorElement.text)
                elif colorElement.tag == "green":
                    newFloss.green = int(colorElement.text)
                elif colorElement.tag == "blue":
                    newFloss.blue = int(colorElement.text)
    if newFloss.name != None:
        floss_list.append(newFloss);

for floss in floss_list:
    print((floss.toCpp()))

print(("\n Count:", len(floss_list)))
