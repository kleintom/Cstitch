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
#include "cancelAcceptDialogBase.h"

class patternImageLabel;
class patternDockWidget;
class toolDock;
class windowManager;
class comboBox;
class dockImage;
class triC;
class helpMode;
template<class T> class findActionName;
class QDomDocument;
class QDomElement;
class QScrollArea;
class QPushButton;
class QVBoxLayout;
class QGroupBox;
class QLineEdit;

typedef findActionName<patternImagePtr> findPatternActionName;

// dialog for the user to turn auto pdf-viewer-loading when a pdf
// pattern gets saved on/off, and to set the viewer path
class pdfViewerDialog : public cancelAcceptDialogBase {

  Q_OBJECT

 public:
  pdfViewerDialog(bool useViewer, const QString& curViewerPath,
                  QWidget* parent);
  bool useViewer() const;
  QString viewerPath() const;

 private slots:
  void updateViewerPath();

 private:
  // a checkable group that is checked if we should use the pdf viewer
  // specified by the group's child widgets to view saved pdf patterns
  QGroupBox* useViewer_;
  QVBoxLayout* dialogLayout_;
  QVBoxLayout* groupLayout_;
  QLineEdit* currentPath_;
  QPushButton* choosePath_;
};

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

// Individual images can be deleted from the Image menu, and the user
// can turn gridding on or off and change the grid color.  The user is
// allowed to save pattern images, but because of their typically
// large size, the user will be given a warning if the image is "big".
//
// Initial symbol selection is done automatically.  There are a fixed
// number of symbols, but there are four groups of borders for those
// symbols, meaning there are four times the number of "characters"
// available for use as symbols.  The symbols made available to the
// user are just enough to have a few more than the number of colors
// in the pattern.
//
// The user can change a symbol by left clicking on the image, which
// brings up a dialog allowing selection of any symbol not currently
// taken.  Currently there is no way to switch two symbols without
// swapping with an unused symbol.
//
// Clicking the "To pdf" button produces a black and white pdf version
// of the pattern (with no symbol borders), with as much of the
// pattern as will fit per page, a guide to which part of the pattern
// is on which page, and a list of symbol/color correspondences along
// with symbol codes (for DMC colors) and square counts.
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
  // set up the dialog for the user to choose a pdf viewer
  void constructPdfViewerDialog();
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
  // let the user update the pdf viewer options for displaying saved
  // pdf patterns
  void updatePdfViewerOptions();

 private:
  // a default dimension for pattern symbol squares that should make
  // symbols distinguishable on screen
  int basePatternDim_;
  // cached value of the last user-set pdf symbol dimension
  int pdfSymbolDim_;

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

  // keep fontMetrics for the app font since we use them a lot
  QFontMetrics fontMetrics_;

  // remember whether or not we're loading a pdf viewer to view saved
  // pdf patterns
  bool usePdfViewer_;
  QString pdfViewerPath_;
  QAction* setPdfViewerAction_;
};

#endif
