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

# Convert an xml file containing documentation data into various
# formats for use in areas such as application help or webpage help.

import xml.dom.minidom, re
from abc import ABCMeta, abstractmethod

## functions ##################################################################
def getText(node, tag):
    """Return the text of the first descendant of <node> with name
    <tag>, or else return ""."""
    textNode = node.getElementsByTagName(tag)[0].firstChild
    if textNode:
        return textNode.data
    else:
        return ""

## classes ####################################################################
#### data classes
class parts:
    """Abstract base class for data holding classes"""

    __metaclass__ = ABCMeta

    def __init__(self, element):
        self.title = getText(element, 'title')
        self.intro = getText(element, 'intro')
        self.mainBody = getText(element, 'mainBody')
        self.outro = getText(element, 'outro')

    @abstractmethod
    def cleanData(): pass

class qtParts(parts):
    """Data handler for data destined for QT docs"""

    def __init__(self, element):
        parts.__init__(self, element)
        self.cleanData()

    def cleanData(self):
        for part in ["intro", "mainBody", "outro"]:
            # image names start with :
            newValue = re.sub(r'(<img[^>]+src=")', r'\1:', getattr(self, part))
            # hrefs may be to different files
            newValue = re.sub(r'(<a href="[\w]+)(#|")', r'\1.html\2', newValue)
            setattr(self, part, newValue)

    def filename(self):
        camelName = self.title.replace(' ', '')
        # lower case first letter
        camelName = camelName[:1].lower() + camelName[1:]
        return camelName + '.html'

    def allTextParts(self):
        return self.intro + self.mainBody + self.outro

class singleWebParts(parts):
    """Data handler for data destined for web docs (single page)."""

    def __init__(self, element):
        parts.__init__(self, element)
        self.cleanData()

    def cleanData(self):
        for part in ["intro", "mainBody", "outro"]:
            # image names start with images/32x32_
            newValue = re.sub(r'(<img[^>]+src=")', r'\1images/32x32_', getattr(self, part))
            # anchor refs are all to anchors in this file
            newValue = re.sub(r'(<a href=")[\w]+#', r'\1#', newValue)
            # non-anchored refs need to become anchored, because they
            # refer to refs on this page
            newValue = re.sub(r'(<a href=")([\w]+")', r'\1#\2', newValue)
            setattr(self, part, newValue)

class sections:
    """Sections consists of a collection of parts, loaded from an
    external xml source"""

    def __init__(self, xmlFilename, partsType):
        """Load "pages" of data from <xmlFilename>; <partsType>
        determines the type of object used to hold the data"""
        if partsType == "multi": # Qt doc: multiple output files
            createParts = lambda element: qtParts(element)
        elif partsType == "singleWeb":
            createParts = lambda element: singleWebParts(element)
        else:
            print "!Unrecognized parts type: " + partsType
            sys.exit(-1)
        dom = xml.dom.minidom.parse(xmlFilename)
        self.pages = []
        for topNode in dom.firstChild.childNodes:
            if topNode.nodeType == xml.dom.minidom.Node.ELEMENT_NODE:
                self.pages.append(createParts(topNode))

#### processor classes
class sectionsProcessor:
    """Abstract base class for classes that handle the formatting and
    writing of doc data"""

    __metaclass__ = ABCMeta

    @abstractmethod
    def processSections(self): pass

    def writeHtml(self, outputFile, htmlTitle, css, useInpageCss, htmlBody):
        """Write html to <outputFile>.  CSS is included directly in
        the head as the string <css> if <useInpageCss> is true,
        otherwise css is external coming from the file name <css> if
        <useInpageCss> is false.  The header title is <htmlTitle> and
        the html body is <htmlBody>."""

        template = open('html_template.html').read()
        template = re.sub(r'@@title', htmlTitle, template)
        if useInpageCss: # put css statements in head
            template = re.sub(r'@@css', ('<style type="text/css">\n' + css +
                                         '\n  </style>'), template)
        else: # load css from file
            template = re.sub(r'@@css', ('<link rel="stylesheet" type="text/css" href="' +
                                         css + '" />'), template)
        template = re.sub(r'@@body', htmlBody, template)
        
        outFile = open(outputFile, 'w')
        outFile.write(template)

class qtPageProcessor(sectionsProcessor):
    """Processor for producing Qt multi page docs"""

    def __init__(self, xmlFilename):
        self.sections = sections(xmlFilename, "multi")

    def css(self):
        return """h1 {color: #00B300; }
h2 {color: #009900; }
h3 {color: #004D00; }
div.h2Div {margin-left: 15px;}
div.h3Div {margin-left: 30px;}
body {
    max-width: 950px;
    margin-left: auto;
    margin-right: auto;
}"""

    def writeNav(self, direction, href, title):
        """Return a "navigation" string used to navigate through the
        documentation"""
        return direction + ': <a href="' + href + '">' + title + '</a>;\n'

    def previousStrings(self, index):
        """Return a [filename, title] navigation pair for the page
        prior to <index>"""
        if index != 0:
            filename = self.sections.pages[index - 1].filename()
            title = self.sections.pages[index - 1].title
            if index != 1: # the first (overview) page is special
                title = "The " + title.lower() + " panel"
            return [filename, title]
        else:
            return ["", ""]

    def nextStrings(self, index, lastIndex):
        """Return a [filename, title] navigation pair for the page
        after <index>"""
        if index != lastIndex:
            filename = self.sections.pages[index + 1].filename()
            title = "The " + self.sections.pages[index + 1].title + " panel"
            return [filename, title]
        else:
            return ["", ""]

    def processSections(self):
        """Write a doc file for each section in self.sections"""
        lastIndex = len(self.sections.pages) - 1
        overviewFile = self.sections.pages[0].filename()
        overviewTitle = self.sections.pages[0].title
        for i, page in enumerate(self.sections.pages):
            #insert nav header/footer
            navLine = ""
            [previousFile, previousTitle] = self.previousStrings(i)
            [nextFile, nextTitle] = self.nextStrings(i, lastIndex)
            if previousFile:
                navLine = navLine + self.writeNav("Previous",
                                                  previousFile,
                                                  previousTitle)
            if nextFile:
                navLine = navLine + self.writeNav("Next",
                                                  nextFile,
                                                  nextTitle)
            if i != 0:
                navLine = navLine + self.writeNav("Top",
                                                  overviewFile,
                                                  overviewTitle)
            navLine = "<p>\n" + navLine.strip().rstrip(";") + "\n</p>"
            outputFile = page.filename()
            self.writeHtml(outputFile, self.sections.pages[i].title,
                           self.css(), True, 
                           navLine + page.allTextParts() + navLine)
            
class singleWebProcessor(sectionsProcessor):
    """Processor for producing single webpage docs"""

    def __init__(self, xmlFilename):
        self.cssFilename = 'doc.css'
        self.sections = sections(xmlFilename, "singleWeb")

    def processSections(self):
        """Combine the data pages into a single output document"""
        htmlBody = ""
        for page in self.sections.pages:
            htmlBody = htmlBody + page.mainBody
        self.writeHtml("cstitchDoc.html", "Cstitch documentation",
                       self.cssFilename, False, htmlBody)

## main #######################################################################

if __name__ == '__main__':

    dataFile = 'docData.xml'

    qtDoc = qtPageProcessor(dataFile)
    qtDoc.processSections()

    webDoc = singleWebProcessor(dataFile)
    webDoc.processSections()

