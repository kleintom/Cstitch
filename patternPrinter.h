//
// Copyright 2010, 2011 Tom Klein.
//
// This file is part of cstitch.
//
// cstitch is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef PATTERNPRINTER_H
#define PATTERNPRINTER_H

#include <QPainter>
#include <QtPrintSupport/QPrinter>
#include <QCoreApplication>

#include "floss.h"
#include "patternImageContainer.h"

class patternMetadata;
class QImage;

// patternPrinter prints a pattern to a pdf.
class patternPrinter {

 public:
  // <image> provides color/symbol information for the pattern
  patternPrinter(patternImagePtr image, const QImage& originalImage);
  // write the pdf - if <usePdfViewer> then use <pdfViewerPath> to open
  // the pdf
  void save(bool usePdfViewer, const QString& pdfViewerPath);

 private:
  // draw the <metadata> and the original and square images
  void drawTitlePage(const patternMetadata& metadata);
  // draw <text> centered at the top of the rectangle
  // <availableTextRect>, using <fontSize> and <bold>;
  // set <availableTextRect>'s top to the bottom of the text rectangle
  // on return
  void drawTitleMetadata(int fontSize, bool bold, const QString& text,
                         QRect* availableTextRect);
  QImage gridedImage(const QImage& image, int originalSquareDim,
                     int originalWidth, int originalHeight,
                     qreal gridLineWidth = 1) const;
  // Set portrait_ (bool) and xPages_, yPages (ints) based on whichever
  // orientation minimizes the total number of pages, where the pattern
  // image is broken up into xPages_ x yPages_ pages (after the image is
  // oriented)
  void computeOrientationAndPageCounts();
  // return the number of pages required to print a pattern of width
  // <w> and height <h>, using <widthPerPage> and <heightPerPage>; on
  // return set the number of horizontal (<xPages>) and vertical
  // (<yPages>) pages used to cover the original pattern
  int computeNumPagesHelper(int w, int h, int widthPerPage, int heightPerPage,
                            int* xPages, int* yPages) const;
  // draw the legend that shows the pattern part <--> page number
  // correspondence; return the height of the legend
  int drawLegend();
  // draw the color list (colors/symbols/names/counts), starting at height
  // <startHeight>
  void drawColorList(int startHeight);
  // draw column headers for a section of the color list
  void drawListHeader(int xStart, int y, int countTab, int codeTab,
                      int nameTab);
  // return true if the user cancelled during drawing
  bool drawPatternPages();
  // return the code representation of floss <f>
  QString flossToCode(const typedFloss& f, bool useCodeAbbreviations) const;
  // return the width of s
  int sWidth(const QString& s) const {
    return fontMetrics_.boundingRect(s).width();
  }
  // return the height of s
  int sHeight(const QString& s) const {
    return fontMetrics_.boundingRect(s).height();
  }
  // return the width of n
  int sWidth(int n) const {
    return sWidth(QString::number(n));
  }
  // return the height of n
  int sHeight(int n) const {
    return sHeight(QString::number(n));
  }
  // Print the color list description.  Adjust <yUsed> to account for the
  // height used, return true if color codes in the color list should be
  // preceded with their code type.
  bool printListDescription(int* yUsed, int fontHeight);

 private:
  QPrinter printer_;
  QPainter painter_;
  patternImagePtr imageContainer_;
  const QImage& squareImage_;
  const int squareDim_;
  const QImage& originalImage_;
  int pdfSymbolDim_;
  const QVector<flossColor> colors_;
  int patternImageWidth_;
  int patternImageHeight_;
  // x and y coordinates of the uppper left corner for pattern drawing
  int margin_;
  QFontMetrics fontMetrics_;

  //// this group of variables depends on whether we're portrait or not
  bool portrait_; // true if we're printing the pattern in portrait mode
  // the number of x-direction blocks the pattern is broken up into
  int xPages_;
  int xBoxes_; // total number of symbol boxes in the x direction
  int xBoxesPerPage_;
  // the number of y-direction blocks the pattern is broken up into
  int yPages_;
  int yBoxes_; // total number of symbol boxes in the y direction
  int yBoxesPerPage_;
  int printerWidth_; 
  int printerHeight_;
  int widthPerPage_; // pattern width per output page
  int heightPerPage_; // pattern height per output page
};

#endif
