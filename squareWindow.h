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

#ifndef SQUAREWINDOW_H
#define SQUAREWINDOW_H

#include "imageCompareBase.h"
#include "squareDockTools.h" // for contextColorAction
#include "squareImageLabel.h"
#include "squareImageContainer.h"
#include "squareTools.h"

class squareToolDock;
class squareDockWidget;
class colorDialog;
class squareImageContainer;
class patternWindowSaver;
class helpMode;
class triC;

// added as a right-click context action on the color list when in
// change all mode
class changeAllContextAction : public contextColorAction {

  Q_OBJECT

 public:
  changeAllContextAction(const QIcon& icon, const QString& actionName,
                         QWidget* parent)
    : contextColorAction(icon, actionName, parent) { }
  void trigger(const QColor& color) const {
    emit changeAllContextTrigger(color);
  }
 signals:
  void changeAllContextTrigger(const QColor& color) const;
};

// class squareWindow
//
// squareWindow is the third of the four main application windows
// (see main.h for more information on the main windows).
// The main purpose of this window is to edit squared images as such (i.e.
// editing tools apply to squares, not individual pixels) in preparation
// for creation of actual patterns.  squareWindow provides the same side
// by side view as colorCompare (see colorCompare.h for more information).
//
// There are five editing modes.
//
// 1.  The pointer mode is the "null" mode under which no editing occurs.
// This mode is used so that the user can use the mouse to select an image
// side without performing any editing action.
//
// 2. Change all mode is used to change a color on the color list and all
// corresponding squares in the currently selected image.
//
// 3. Change one mode is used for "drawing" on the selected image - it
// changes the squares under mouse on a click-drag to the currently
// selected color.
//
// 4. Fill region mode is used to fill in a single-color contiguous
// region (chosen by mouse click within the region) with the currently
// selected color.
//
// 5. Detail squares mode is used to recolor user selected squares with
// a user-selected number of colors.  The idea is to be able to "redo"
// detail areas with small numbers of pixels that may have been maltreated
// by the larger scale processing of colorCompare and colorChooser.
//
// For modes 2,3,4 a right click makes the current color the color under
// the mouse, and a middle click brings up a color selection dialog with
// multiple modes for choosing a color from a dmc list, the current color
// list, from either displayed image via mouse click, or using the
// standard Qt color dialog.
//
// The user selects an image and clicks the "Pattern" button to produce
// a pattern image for the selected image and load it into the
// patternWindow for further processing there.
//
////
// Implementation notes
//
// Zooming is a bit involved, due to the following constraints:
// * squared images can only be shown at dimensions that are multiples
// of the number of squares in that dimension (otherwise you get squares
// of different sizes, which is noticeable even though only off by 1) -
// note that for different square dimensions that means different
// constraints for different images
// ** new images should be shown at the size of the image it just replaced
// on screen - by * that's not always possible; in all such cases we
// choose the largest permissible size less than or equal to the desired
// size
// *** all zoom actions apply to the active side, but if dual zooming is
// on then the opposite side should be zoomed to the same size (with the
// same proviso on actual size as in **)
//
// To achieve such results, setScaled* on squareImageContainers take
// size _hints_ in all cases and returns the actual (next smallest)
// permissible size that is actually used.  In between calls to zoom
// functions, setScaledSize can only be set once on a given image,
// after which it will always return its last saved scaled size (of
// course that's not true - you just have to reset the zoom with
// resetZoom() or by scaling the zoom to 0 before you set a new scaled
// size) - that has the effect that the image keeps track of its
// history and all other code can just blindly request what it thinks
// is best based on the last image that was on screen.  After any zoom
// all on screen sizes need to be adjusted to fit with the new base
// zoomed size, so all of the image's scaledSizes are set to (0,0),
// after which the image will reconsider any size hint sent to it in
// setScaledSize.
//
class squareWindow : public imageCompareBase {

  Q_OBJECT

  // square tools need access to private data
  friend class squareTool;
  friend class nullTool;
  friend class inactiveImageClickBase;
  friend class changeAllTool;
  friend class changeOneTool;
  friend class fillTool;
  friend class detailTool;

 public:
  // newImage is the first image to be added (along with originalImage),
  // with dimension squareDimension, colors <colors> (which are or are not
  // all <dmc>)
  squareWindow(const QImage& newImage, int imageIndex, int squareDimension,
               const QVector<triC>& colors, flossType type,
               windowManager* winManager);
  // add an <image>, to be named based on <index>, with the given
  // <colors>, which are of floss type <type>, and the given
  // dimension.  Index must be unique among all other such indices.
  void addImage(const QImage& image, int squareDimension,
                const QVector<triC>& colors, flossType type, int index);
  // For each image, update the color list if necessary.
  void checkAllColorLists();
  // recreate a pattern image using the data in <saver> as part of a
  // project restore
  void recreatePatternImage(const patternWindowSaver& saver);
  // append the current history and tool floss mode of the image with
  // index <imageIndex> to <appendee> as xml
  void writeCurrentHistory(QDomDocument* doc, QDomElement* appendee,
                           int imageIndex);
  // update the edit history of the image whose information is contained
  // in <xml> and run the back history if it exists
  void updateImageHistory(const QDomElement& xml);
  void appendCurrentSettings(QDomDocument* doc,
                             QDomElement* appendee) const; //override;
  QString updateCurrentSettings(const QDomElement& xml); //override;

 private:
  // constructor helper
  void constructActionsAndMenus();
  // constructor helper - there is a color list dock and a tool dock
  void constructDocks();
  // constructor helper - set all of the signal/slot connections
  void setConnections();
  // for imageCompareBase
  imageLabelBase* leftLabel() const { return leftLabel_; }
  imageLabelBase* rightLabel() const { return rightLabel_; }
  squareImagePtr squareImageFromIndex(int imageIndex) {
    const imagePtr image = getImageFromIndex(imageIndex);
    if (image) {
      return squareImagePtr(image->squareContainer());
    }
    else {
      return squareImagePtr(NULL);
    }
  }
  void setCurImage(imagePtr container) {
    curImage_ = container->squareContainer();
  }
  imagePtr curImage() const { return curImage_; }
  void setLeftImage(imagePtr container) {
    leftImage_ = container->squareContainer();
  }
  imagePtr leftImage() const { return leftImage_; }
  void setRightImage(imagePtr container) {
    rightImage_ = container->squareContainer();
  }
  imagePtr rightImage() const { return rightImage_; }
  // return a new grided version of the <image> (if the currently selected
  // image supports gridding) using the last set grid color.
  // <image> may be scaled, but the other dimensions are from the original
  // unscaled <image>.
  // Returns <image> if <image> doesn't support gridding.
  QImage gridedImage(const QImage& image, int originalSquareDim,
                     int originalWidth, int originalHeight) const;
  // returns true if the current image accepts gridding
  bool gridOn() const;
  // pop up a dialog box with input
  // <oldColor> to let the user choose a new color; originalX, originalY,
  // and squareDim are used to populate the dialog with possible nearby
  // replacement colors from the current image.  The dialog emits a signal
  // to indicate it has finished.
  // (Used when the user chooses to change a color from the color list.)
  void requestSquareColor(int originalX, int originalY, int squareDim,
                          const triC& oldColor);
  // when the color <dialog> is in use we want the user to be able to click
  // on this widget's images to choose colors but not to be able to change
  // the widget's state otherwise, so if <b> is true we turn off buttons
  // and menus that could change state and setup dialog signals to go to
  // the proper place based on the current tool mode.
  // changeAllRequest is true if the colorDialog is being used to request
  // a replacement color for a changeAll action - in that case we direct
  // the dialog to call the changeColor processing code when it completes.
  void colorDialogTracking(bool b, colorDialog* dialog,
                           bool changeAllRequest = false);
  // if <b>, deactivate the subwidgets we don't want active while the
  // color dialog is in use, else turn them back on
  void deactivateWidgetsForColorDialog(bool b);
  bool eventFilter(QObject* watched, QEvent* event);
  // extends imageCompareBase
  bool filterKeyEvent(QKeyEvent* event);
  squareImageLabel* activeSquareLabel() {
    return (curImage_ == leftImage_) ? leftLabel_ : rightLabel_;
  }
  const squareImageLabel* activeSquareLabel() const {
    return (curImage_ == leftImage_) ? leftLabel_ : rightLabel_;
  }
  // return the not-currently-selected label
  squareImageLabel* inactiveSquareLabel() {
    return (curImage_ == leftImage_) ? rightLabel_ : leftLabel_;
  }
  QScrollArea* activeScroll() {
    return  (curImage_ == leftImage_) ? leftScroll() : rightScroll();
  }
  QScrollArea* inactiveScroll() {
    return  (curImage_ == leftImage_) ? rightScroll() : leftScroll();
  }
  squareImagePtr inactiveImage() {
    return (curImage_ == leftImage_) ? rightImage_ : leftImage_;
  }
  // mark <container> as the currently selected image
  // extends imageCompareBase::
  void setCur(imagePtr container);
  // clear all detail squares (if any) from the current image
  void ensureDetailSquaresCleared();
  // update widget enabled states based on whether there is or is not
  // a currently selected image and whether or not that image has a valid
  // color count (too many or no colors makes an image invalid)
  void updateSubWidgetStates();
  // if the color dialog is in "choose from image" mode then pass it the
  // color under mouse (using <lra>) - report it as a mouse <move> or else
  // as a click
  void processDialogMouseEvent(QMouseEvent* event,
                               const leftRightAccessor& lra, bool move);
  // processLeft/RightImageClick helper - process an image click based on
  // the current tool mode and whether the click was on the active image
  // or inactive image
  void processImageClickLR(const leftRightAccessor& lra,
                           QMouseEvent* event);
  // return the current image at the scale the user is seeing, else return
  // a null image
  // implements imageSaverWindow::
  QImage curImageForSaving() const;
  // return approximately one square dim at the current zoom scale
  // used for updating images around a tool edit point
  int roughCurDim() const {
    return curImage_->scaledDimension() + 2;
  }
  // update the back and forward history buttons on the toolbar based on
  // the state of the current image
  void updateHistoryButtonStates();
  void keyPressEvent(QKeyEvent* event);
  // update subwidget states based on the results of a recent tool action,
  // as specified by <update> and <updateRectangle>
  void curImageUpdated(const dockListUpdate& update,
                       const QRect& updateRectangle = QRect());
  // the user just deleted one of our images
  // implements imageCompareBase::
  void imageDeleted(int imageIndex);
  // there are no images left other than the original, so we notify
  // windowManager, which hides us until there's a new image to be added
  void imageListEmpty();
  // set the <label> and the <image> to have width <width>>, and update
  // anything that needs to know about the changes
  // NOTE that, in order to maintain squared images where all of the
  // squares have the same size, these methods will actually set the size
  // to the largest permissible size less than the given size
  void setScaledImageWidth(int width, imageLabelBase* label,
                           imagePtr image);
  void setScaledImageHeight(int height, imageLabelBase* label,
                            imagePtr image);
  void zoomToHelper(const zoomHelperFunction& helperFunction) {
    resetZooms();
    imageCompareBase::zoomToHelper(helperFunction);
  }
  // set all image scaled sizes to (0, 0), excluding the current left and
  // right images if <excludeCurs>
  void resetZooms(bool excludeCurs = false);
  // return the helpMode enum value for this widget
  helpMode getHelpMode() const;
  // update the <updateRectangle> region of the active image label
  // with the current image
  void updateImageLabelImage(const QRect& updateRectangle);
  // update the active image label to view curImage's image
  void updateImageLabelImage();
  // Initialize a colorDialog using the current image, <type>, and
  // <replacementColors>, setup signals/slots for the dialog, and show it.
  // If <changeToolColor> then setup signals/slots to just change the
  // tool color (and not change all <currentColor>)
  void activateColorDialog(const triC& currentColor, flossType type,
                           const QVector<triC>& replacementColors,
                           bool changeToolColor = false);

 private slots:
  // turn gridding on or off globally (for all images)
  void processGridChange(bool checked);
  // pop up a color dialog to get the new color; the new color is global
  // (all images use it).  If gridding is active, all grids change
  // immediately, otherwise the new color is in effect when gridding
  // is activated.
  void processGridColorChange();
  // "asynchronous" change all request: pop up a color dialog to get a new
  // color from the user to replace <oldColor> in the current image;
  // when the dialog finishes it will initiate the actual changeAll
  // processing
  void processChangeAll(const QColor& oldColor);
  // change <oldColor> to <newColor> in the current image
  void processChangeAll(const triC& oldColor, const flossColor& newColor);
  // perform detailing with the given number of colors on the current image
  void processDetailCall(int numColors);
  // remove and clear all detail squares on the current image
  void processDetailClearCall();
  // clear any processing that might have been left unfinished by the
  // previous tool (color dialog or detailing squares)
  void processToolChanged(squareToolCode currentTool);
  // process a mouse click on the left image
  void processLeftImageClick(QMouseEvent* event);
  // process a mouse click on the right image
  void processRightImageClick(QMouseEvent* event);
  // process a mouse click on the active image
  void processActiveImageClick(QMouseEvent* event);
  // process a mouse click on the inactive image
  void processInactiveImageClick(QMouseEvent* event);
  void processInactiveMouseMove(QMouseEvent* event);
  void processActiveMouseMove(QMouseEvent* event);
  void processLeftMouseMove(QMouseEvent* event);
  void processRightMouseMove(QMouseEvent* event);
  // process a mouse release (for the active image)
  void processMouseRelease(QMouseEvent* event);
  // move back one in the edit history (if possible)
  void processBackHistoryAction();
  // move forward one in the edit history (if possible)
  void processForwardHistoryAction();
  // send the current squared image to the window manager for addition
  // to the patternWindow
  void processPatternButton(squareImagePtr image = squareImagePtr(NULL),
                            int patternIndex = -1);
  // the color list can have extra colors if, for example, the user drew
  // over all of a given color with a new color, so remove any colors
  // from the list that don't actually appear in the current image
  void checkColorList();
  // pop up a dialog with useful information about the current image
  void displayImageInfo();
  // the colorChooseDialog_ finished for a changeAll operation - if
  // the dialog was accepted then process the changeAll
  void changeAllDialogFinished(int returnCode, const triC& oldColor,
                               const flossColor& newColor);
  // the colorChooseDialog_ finished for a color change operation - if
  // the dialog was accepted then set the tool dock color swatch to
  // <newColor>
  void changeColorDialogFinished(int returnCode, const triC& ,
                                 const flossColor& newColor);
  // the colorChooserDialog_ has closed, so set it to NULL and reenable
  // subwidgets for standard usage
  void processColorDialogClosing();
  // process a right image mouse move with the colorChooseDialog_ open
  // (mouse events go to the dialog when active)
  void processRightDialogMouseMove(QMouseEvent* event);
  void processLeftDialogMouseMove(QMouseEvent* event);
  void processRightDialogMouseClick(QMouseEvent* event);
  void processLeftDialogMouseClick(QMouseEvent* event);
  // do any processing required for a dual zoom setting change
  // overrides imageCompareBase::
  void updateDualZooming(bool dualZoomOn);
  void zoom(int zoomIncrement);
  void replaceRareColors();
  void toolDockLabelColorRequested(const triC& currentColor, flossType type);
  void processToolFlossTypeChanged(flossType newType);

 private:
  // these pointers track which square image the left, right, and current
  // (i.e. focused) splitter sides are viewing
  squareImagePtr leftImage_;
  squareImagePtr rightImage_;
  squareImagePtr curImage_;

  // the labels hold the left and right images
  squareImageLabel* leftLabel_;
  squareImageLabel* rightLabel_;

  nullTool noopTool_;
  changeOneTool changeOneTool_;
  changeAllTool changeAllTool_;
  fillTool fillTool_;
  detailTool detailTool_;
  // the current tool always refers to one of the concrete tools above
  squareTool* curTool_;

  // used whenever the user needs to select a new color; non-NULL only
  // when actually visible and in use
  colorDialog* colorChooseDialog_;
  // let the user turn gridding on/off (for all images at once)
  QAction* gridAction_;
  // let the user change the grid color (for all images at once)
  QAction* gridColorAction_;
  QAction* removeRareColorsAction_;
  // let the user make sure all of the colors in the current color list
  // actually appear in the image (cf. checkColorList())
  QAction* updateColorListAction_;
  // send the current squared image onto patternWindow
  QPushButton* patternButton_;

  // the color list dock displays the color list for the current image
  squareDockWidget* colorListDock_;

  // the tool dock displays the tool selection widget
  QDockWidget* toolDockHolder_;
  squareToolDock* toolDock_;

  // let the user move backward/forward in the edit history for the
  // current image
  QAction* backHistoryAction_;
  QAction* forwardHistoryAction_;
};

#endif
