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

#ifndef COLORCOMPARE_H
#define COLORCOMPARE_H

#include "imageCompareBase.h"

class dockListSwatchWidget;
class windowManager;
class squareWindowSaver;
class helpMode;
class imageLabelBase;
class imageLabel;
class QDomDocument;
class QDomElement;
class QComboBox;
class QSpinBox;

// class colorCompare
//
// colorCompare is the second of the four main application windows
// (see main.h for more information on the main windows).
// The main purpose of this window is to allow the user to compare the
// outputs from colorChooser to decide which selection mode and color
// choices from that window are most useful.  To that end colorCompare
// uses a side by side two image mode (a QSplitter) to allow the user to
// compare any two available images side by side.  The images available
// are the original image (which can't be deleted) and any image the user
// processed from the colorChooser.
//
// The same image can't be shown on both sides of the splitter.  Actions
// apply to the currently selected side of the splitter (visible by an
// outline around the active side and a bold "L:" or "R:" (for Left or
// Right) in the toolbar).  From the menus the user can choose to hide/show
// a side of the splitter or delete an image (permanently).
// The nth image processed from ImageChooserWindow is named "Image n" (the
// original image has index 0).
//
// The user selects a processing mode which determines how to "square"
// the currently selected image based on the user's selected square
// size.  If the user selects a 10 pixel square size on a 205x102 pixel
// image then the processed image will be 200x100 pixels and will consist
// of 20x10 squares (only full squares are kept).  The color of each
// square in the new image is what is determined by the process mode.
//
// There are two squaring modes.  Under median mode, roughly speaking, the
// representative color of a given square is chosen to be that color in
// the original square that is "least far" in total from all of the other
// colors in the block.  This can be thought of as choosing the "most
// representative" color in a given square, and generally gives the best
// results.
//
// Mode mode chooses the representative color of a given square to be the
// color that appears most often in that square.
//
// Once the user has selected an image, a processing mode, and a square
// size, they click a button to proceed to the next stage, the
// squareWindow.  (The original image can be selected at this stage, but
// it may be disallowed for final pattern processing if it has too many
// colors.)
//
class colorCompare : public imageCompareBase {

  Q_OBJECT
  //processing modes
  enum {SQ_MODE, SQ_MEDIAN};

 public:
  // <index> is for the image description text (as in "Image 1");
  // <colors> is the list of colors in <image>, of floss type <type>
  colorCompare(const QImage& image, int index, const QVector<triC>& colors,
               flossType type, windowManager* winMgr);
  // add an <image>, to be named based on <index>, with the given <colors>,
  // which are of floss type <type>.
  // Index must be unique among all other such indices.
  void addImage(const QImage& image, int index,
                const QVector<triC>& colors, flossType type);
  // recreate a square image using the data in <saver> as part of a project
  // restore
  int recreateImage(const squareWindowSaver& saver);
  void appendCurrentSettings(QDomDocument* doc,
                             QDomElement* appendee) const; //override;
  QString updateCurrentSettings(const QDomElement& xml); //override;

 private:
  // constructor helper
  void createSquaringOptions();
  // constructor helper
  void populateToolbar();
  // make the input image the selected image, or if NULL, update the widget
  // to indicate no image is active
  // extends imageCompareBase::setCur
  void setCur(imagePtr ic);
  // return the current image at the scale the user is seeing, else
  // return a null image
  // implements imageSaverWindow::
  QImage curImageForSaving() const;
  // the user just deleted one of our images
  // implements imageCompareBase::
  void imageDeleted(int imageIndex);
  imageLabelBase* leftLabel() const;
  imageLabelBase* rightLabel() const;
  imageLabel* activeImageLabel() const {
    return (curImage_ == leftImage_) ? leftLabel_ : rightLabel_;
  }
  void setCurImage(imagePtr container) { curImage_ = container; }
  imagePtr curImage() const { return curImage_; }
  void setLeftImage(imagePtr container) { leftImage_ = container; }
  imagePtr leftImage() const { return leftImage_; }
  void setRightImage(imagePtr container) { rightImage_ = container; }
  imagePtr rightImage() const { return rightImage_; }
  // there are no images left other than the original, so we notify
  // windowManager, which hides us until there's a new image to be added
  void imageListEmpty();
  // return the helpMode enum value for this mode
  helpMode getHelpMode() const;
  // set the initial square box size based on the <imageSize>
  void setInitialSquareSize(const QSize imageSize);
  // update the active image label to view curImage's image
  void updateImageLabelImage();

 private slots:
  // display useful information in a popup dialog about the current image
  // (if any)
  void displayImageInfo();
  // process the current image (if any) based on the current square
  // dimension and square mode, and pass the new squared image to the
  // window manager for addition to squareWindow.
  void processSquareButton(imagePtr container = imagePtr(NULL),
                           QString squareModeString = "",
                           int squareSize = 0, int newIndex = -1);
  // process a mouse move on the left side of the splitter
  void processLeftMouseMove(QMouseEvent* event);
  // process a mouse move on the right side of the splitter
  void processRightMouseMove(QMouseEvent* event);
  // update the square mode - squareBoxIndex is the index of the user's
  // selected entry in the square mode box
  void changeProcessMode(int squareBoxIndex);
  // pop up a dimension computer widget
  void showDimensions();

 private:
  // the color list dock widget
  dockListSwatchWidget* listDock_;
  // clicked to create a new squared image and proceed to squareWindow
  QPushButton* squareButton_;
  // the square mode selection box
  QComboBox* squareModeBox_;
  // the square size selection box
  QSpinBox* squareSizeBox_;
  // menu action to show the dimension computer
  QAction* dimensionsAction_;
  // the current square mode
  int squareMode_;
  // the image label for the left side of the splitter
  imageLabel* leftLabel_;
  // the image label for the right side of the splitter
  imageLabel* rightLabel_;
  // the image in the left side of the splitter
  imagePtr leftImage_;
  // the image in the right side of the splitter
  imagePtr rightImage_;
  // either leftImage_ or rightImage_, whichever currently has focus
  imagePtr curImage_;
};

#endif
