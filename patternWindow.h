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

#ifndef PATTERNWINDOW_H
#define PATTERNWINDOW_H

#include "imageSaverWindow.h"
#include "patternImageContainer.h"

class patternImageLabel;
class patternDockWidget;
class toolDock;
class windowManager;
class comboBox;
class dockImage;
class triC;
class helpMode;
class patternMetadata;
template<class T> class findActionName;
class QDomDocument;
class QDomElement;
class QScrollArea;
class QPushButton;

typedef findActionName<patternImagePtr> findPatternActionName;

// class patternWindow
//
// patternWindow is the fourth of the four main application windows
// (see main.h for more information on the main windows).  The widget
// displays patterns generated from squared images in squareWindow.
// Because of the size of the squares necessary to be able to distinguish
// different pattern symbols on the screen, typical images are quite large,
// so we don't provide a side by side comparison.  Instead the user can
// switch between two views for a given image: a pattern image which
// displays pattern symbols with a border of their corresponding color, and
// the squared image that generated the pattern.  A dock widget provides
// a scaled down image with a rectangle indicating the current full size
// view that the user can drag to move the full size view.

// Individual images can be
// deleted from the Image menu, and the user can turn gridding on or off
// and change the grid color.  The user is allowed to save pattern images,
// but because of their typically large size, the user will be given a
// warning if the image is "big".
//
// Initial symbol selection is done automatically - symbols come in "light"
// and "dark" versions and are assigned to colors based on whether the
// color's intensity is smaller or larger than the average intensity.
// There are a fixed number of symbols, but there are four groups of
// borders for those symbols, meaning there are four times the number of
// "characters" available for use as symbols.  The symbols made available
// to the user are just enough (from 1 to 4) to have a few more than the
// number of colors in the pattern.
//
// The user can change a symbol by left clicking on the image, which
// brings up a dialog allowing selection of any symbol not currently taken.
// Currently there is no way to switch two symbols without swapping with
// an unused symbol.
//
// Clicking the "To pdf" button produces a black and white pdf version of
// the pattern at the current zoom level (with no symbol borders), with
// as much of the pattern as will fit per page, a guide to which part of
// the pattern is on which page, and a list of symbol/color
// correspondences along with symbol codes (for DMC colors) and square
// counts.
//
////
// Implementation notes
//
// A scroll area holds an imageLabel, which holds the image being viewed
// (either a pattern or a square image).  All actions operate on the
// image being shown.
//

class patternWindow : public imageSaverWindow {

  Q_OBJECT

 public:
  explicit patternWindow(windowManager* winMgr);
  // Add <squareImage> to the list of images with the given data, named
  // from <imageIndex>.  Set the global grid color to <gridColor>.
  void addImage(const QImage& squareImage, int squareDimension,
                const QVector<triC>& colors, QRgb gridColor,
                int imageIndex);
  // append the current edit history for the image with index <imageIndex>
  // to appendee
  void writeCurrentHistory(QDomDocument* doc, QDomElement* appendee,
                           int imageIndex);
  // set the history for the image whose information is contained in <xml>
  // and run the backward history if any
  void updateHistory(const QDomElement& xml);
  // set the grid to be on/off, with color <color>
  void setGrid(QRgb color, bool gridOn);
  // append "global" xml settings to <appendee> for this widget
  void appendCurrentSettings(QDomDocument* doc, QDomElement* appendee) const;
  // restore "global" settings for this widget from <xml>
  void updateCurrentSettings(const QDomElement& xml);

 private:
  // constructor helper
  void constructActions();
  // constructor helper
  void constructMenus();
  // constructor helper
  void constructToolbar();
  // return a grided copy of <image>, where the original dimensions are
  // for <image> (which is possibly scaled), using <gridColor>
  QImage gridedImage(const QImage& image, int originalSquareDim,
                     int originalWidth, int originalHeight,
                     QRgb gridColor = Qt::black) const;
  patternImagePtr getImageFromIndex(int index) const;
  // zoom in or out on the image by the given pixel amount
  void zoom(int zoomIncrement);
  // reset the current image size using basePatternDim_
  void originalSize();
  // reset the label's symbols using the current image, and redraw the
  // label
  void updateImageLabelSymbols();
  // update the label symbols and squares using the current image's
  // current symbol dimension and redraw the label
  void updateImageLabelSize();
  // return the current size of the image in the label
  // implements imageSaverWindow::
  QSize curImageViewSize() const;
  // return the image currently being viewed in the label
  // implements imageSaverWindow::
  QImage curImageForSaving() const;
  // make the image in container the new image
  // does nothing if container is already current
  void setCur(patternImagePtr container);
  // enable/disable this history buttons in the toolbar based on the
  // history status of the current image
  void updateHistoryButtonStates();
  void resizeEvent(QResizeEvent* event);
  bool eventFilter(QObject* watched, QEvent* event);
  void keyPressEvent(QKeyEvent* event);
  void processFirstShow();
  //// helpers for producing the pdf pattern output
  // draw the <metadata> and the original and square images on the printer
  // using the painter
  void drawTitlePage(QPainter* painter, QPrinter* printer,
                     const patternMetadata& metadata) const;
  // draw <text> on <painter> centered at the top of the rectangle
  // <availableTextRect>, using <fontSize> and <bold>
  void drawTitleMetadata(QPainter* painter, int fontSize, bool bold,
                         const QString& text,
                         QRect* availableTextRect) const;
  // draw <image> on <painter> centered horizontally, starting at
  // <startHeight>, and as large as possible to fit into
  // <imageWidth> by <imageHeight>
  void drawTitlePageImage(QPainter* painter,
                          const QImage& image, int startHeight,
                          int usableWidth, int usableHeight) const;
  // compute the minimum number of pages required to print the pattern
  // with width <w> and height <h>, using <widthPerPage> and
  // <heightPerPage> and box dimension <pdfDim>, choosing between printing
  // the pattern in horizontal or vertical mode.
  // <xpages> is set to the number of horizontal pages the pattern will be
  // divided into and <ypages> is set to the number of vertical pages the
  // pattern will be divided into - in other words the pattern images will
  // be segmented into <xpages> by <ypages> boxes, for a total of
  // <xpages> * <ypages> pages.
  bool computeNumPages(int w, int h, int widthPerPage,
                       int heightPerPage, int pdfDim,
                       int* xpages, int* ypages) const;
  // return the total number of pages required for the input variables
  // (as in computeNumPages)
  int computeNumPagesHelper(int w, int h, int widthPerPage,
                            int heightPerPage, int pdfDim,
                            int* xpages, int* ypages) const;
  // draw the pdf pattern on the <printer> using the <painter>, where the
  // pattern image has the given dimensions.  Set the last four pointers
  // to their appropriate values on return (cf. computeNumPages).
  bool drawPdfImage(QPainter* painter, QPrinter* printer, int pdfDim,
                    int patternImageWidth, int patternImageHeight,
                    int* xpages, int* ypages,
                    int* widthPerPage, int* heightPerPage);
  // drawPdfImage helper: (xstart, ystart) is where on each page to start
  // drawing; x/yBoxesPerPages is how many hor/vert boxes per page to draw,
  // x/yBoxes is the total number of hor/vert boxes, width/heightPerPage
  // is the number of hor/vert pixels per page, and <portrait> is true
  // if the image is to be drawn vertically (else landscape).
  bool actuallyDrawPdfImage(QPainter* painter, QPrinter* printer,
                            int patternImageWidth, int patternImageHeight,
                            int pdfDim, int xpages, int ypages,
                            int xstart, int ystart, int xBoxes, int yBoxes,
                            int xBoxesPerPage, int yBoxesPerPage,
                            int widthPerPage, int heightPerPage,
                            bool portrait);
  // draw the legend box which indicates which pdf page number corresponds
  // to which portion of the image; pImageWidth/Height are pattern image
  // width/height, width/heightPerPage are pattern image dimensions per
  // printer page, printerWidth/Height are printer page dimensions.
  int drawPdfLegend(QPainter* painter, int pdfDim, int xpages, int ypages,
                    int pImageWidth, int pImageHeight,
                    int widthPerPage, int heightPerPage,
                    int printerWidth, int printerHeight) const;
  // draw the symbol/color correspondence list; pageNum is the
  // page number of the pdf so far, yused is the amount of y space
  // used on <pageNum>, and pdfDim is the dimension of the symbols in the
  // pdf output.
  void drawPdfColorList(QPainter* painter, QPrinter* printer,
                        int pageNum, int yused, int pdfDim) const;
  // draw column headers for a section of the color list
  void drawListHeader(QPainter* painter, int xStart, int y,
                      int countTab, int codeTab, int nameTab) const;
  // return the helpMode enum constant for this mode
  helpMode getHelpMode() const;

 private slots:
  // pop up a dialog for the user to select an unused new symbol for
  // <color>
  void changeSymbolSlot(const triC& color);
  // process an image click on the image: a right button switches between
  // the pattern and square images, a left button initiates a symbol change
  // for the symbol under mouse
  void imageClickSlot(QMouseEvent* event);
  // save a pdf pattern for the current image
  void saveSlot();
  // patternWindow doesn't support the zoomTo methods since in the vast
  // majority of cases they would make the symbols too small to
  // distinguish
  void zoomToWidth() {}
  void zoomToHeight() {}
  void zoomToImage() {}
  // pop up a dialog displaying useful information for the current image
  void displayImageInfo();
  // switch between the pattern and square images (maintaining dimensions)
  void switchActionSlot();
  // turn gridding on or off (for all images)
  void processGridChange(bool checked);
  // move back one on the current image's history list
  void backHistoryActionSlot();
  void forwardHistoryActionSlot();
  // change curImage to the image indicated by <action>
  void processImageListMenu(QAction* action);
  // change curImage to the image indicated by <imageName>
  void processImageBox(const QString& imageName);
  // delete the current image, updating the image lists
  void processDelete();
  // change the current grid color for all images
  void processGridColorChange();
  // process a label scroll change by updating the dock preview rectangle
  // to match the current label viewing area
  void labelScrollChange();
  // process a dock preview image rectangle change by scrolling the
  // main image to start at <x/yPercentage>.  If <rightEdge>, scroll all
  // the way to the right, if <bottomEdge>, scroll all the way to the
  // bottom (x/yPercentage aren't precise enough to handle edge cases).
  void processDockImageUpdate(qreal xPercentage, qreal yPercentage,
                              bool rightEdge, bool bottomEdge);

 private:
  // a default dimension for pattern symbol squares that should make
  // symbols distinguishable on screen and pdf
  int basePatternDim_;

  QScrollArea* scroll_;
  patternImageLabel* imageLabel_;
  // the symbol list dock
  patternDockWidget* listDock_;
  // the dockImage holds the scaled down full image with a rectangle
  // the user can drag to move the full size image in imageLabel_
  QDockWidget* dockImageHolder_;
  dockImage* dockImage_;

  QPushButton* saveToPdfButton_;
  // switch between the square and pattern images
  QAction* switchAction_;
  // turn gridding on/off
  QAction* gridAction_;
  // delete the current image
  QAction* deleteImageAction_;
  // change the grid color for all images
  QAction* gridColorAction_;

  // forward/backward edit history buttons for the current image
  QAction* backHistoryAction_;
  QAction* forwardHistoryAction_;

  // the image currently being viewed in imageLabel_
  patternImagePtr curImage_;
  QMenu* imageListMenu_;
  comboBox* imageListBox_;
};

#endif
