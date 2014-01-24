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

#ifndef COLORDIALOG_H
#define COLORDIALOG_H

#include <QtCore/QMetaType>

#include "triC.h"
#include "imageUtility.h"
#include "cancelAcceptDialogBase.h"
#include "floss.h"

class imageLabel;
class colorButtonGrid;
class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QComboBox;
class QLabel;

class baseDialogMode {

 public:
  baseDialogMode() : scrollArea_(NULL), grid_(NULL), widget_(NULL),
                     isNull_(true) { }
  baseDialogMode(flossType type) : flossType_(type), scrollArea_(NULL),
                                   grid_(NULL), widget_(NULL),
                                   isNull_(true) { }
  flossType flossMode() const { return flossType_; }
  bool isNull() const { return isNull_; }
  void constructGrid(QVBoxLayout* dialogLayout, int gridWidth,
                     const QVector<triC>& colors, QWidget* parent);
  void enable();
  void disable();
  virtual QSize sizeHint() const;
  QSize scrollFrameSize() const;
  void setGridWidth(int width) { gridWidth_ = width; }
  int getGridWidth() const { return gridWidth_; }
  void setCurrentGridSelection(const pairOfInts& coords) {
    currentGridSelection_ = coords;
  }
  void setGridFocus(const pairOfInts& coordinates);
  pairOfInts getCurrentGridSelection() const { return currentGridSelection_; }
  pairOfInts getMaxGridCoordinates() const { return maxCoordinates_; }
  // used by imageDialogMode only
  virtual void updateImageColorLabel(QRgb , bool ) {}
  void setWidget(QWidget* widget) { isNull_ = false; widget_ = widget; }
 private:
  flossType flossType_;
  QScrollArea* scrollArea_;
  colorButtonGrid* grid_;
  QWidget* widget_;
  pairOfInts currentGridSelection_;
  pairOfInts maxCoordinates_;
  int gridWidth_;
  bool isNull_;
};

class squareDialogMode : public baseDialogMode {
 public:
  squareDialogMode() { }
  squareDialogMode(QVBoxLayout* dialogLayout, QVector<triC> colors,
                   flossType type, const triC& inputColor, QWidget* parent);
};

class listDialogMode : public baseDialogMode {
 public:
  listDialogMode() { }
  listDialogMode(QVBoxLayout* dialogLayout, QVector<triC> colors,
                 flossType type, const triC& inputColor, QWidget* parent);
};

class fixedListBaseDialogMode : public baseDialogMode {
 public:
  fixedListBaseDialogMode() { }
  fixedListBaseDialogMode(QVBoxLayout* dialogLayout, const triC& inputColor,
                          flossType type, QVector<triC> colorList,
                          QWidget* parent);
};

class dmcDialogMode : public fixedListBaseDialogMode {
 public:
  dmcDialogMode() { }
  dmcDialogMode(QVBoxLayout* dialogLayout, const triC& inputColor,
                QWidget* parent);
};

class anchorDialogMode : public fixedListBaseDialogMode {
 public:
  anchorDialogMode() { }
  anchorDialogMode(QVBoxLayout* dialogLayout, const triC& inputColor,
                   QWidget* parent);
};

class imageDialogMode : public baseDialogMode {
 private:
  // sizes for the "Choose From Image" dialog
  static const int CFI_WIDTH = 70;
  static const int CFI_BORDER = 5;
  static const int CFI_INNER_WIDTH = CFI_WIDTH - 2*CFI_BORDER;
 public:
  imageDialogMode() : colorLabel_(NULL) { }
  imageDialogMode(QVBoxLayout* dialogLayout, const triC& inputColor,
                  flossType type, QWidget* parent);
  QSize sizeHint() const { return QSize(CFI_WIDTH, CFI_WIDTH + 30); }
  void updateImageColorLabel(QRgb color, bool mouseClick);
 private:
  QLabel* colorLabel_;
};

// class colorDialog
//
// The color dialog offers the user up to five ways to choose a new
// color, from which the user chooses using a menu.  The <type>
// parameter passed on construction determines the types of colors
// that can be chosen using the dialog (if DMC then only DMC colors
// can be chosen, etc)
//
// Choose a Square Color mode:
// Choose from "nearby" colors passed in on construction (<squareColors>).
//
// Choose a List Color mode:
// Choose a color from the <listColors> passed in on construction
// (should consist of the colors on the main window color list dock).
//
// Choose a DMC Color mode: Choose a DMC color.
// Choose an Anchor Color mode: Choose an Anchor color.
//
// Choose From an Image mode: Choose a color by clicking on one of the
// images on view in the main window.
//
// Choose a New Color mode: pop up the Qt colorDialog, allowing for
// the choice of an arbitrary color (may be unavailable if only
// certain colors are allowed).
//
////
// Implementation notes
// The user chooses the mode from a drop down list; when the mode changes
// the old mode is hidden and the new mode is shown (except when New Color
// is chosen, in which case the Qt color dialog is popped up).
// There are four "scrolling" modes that show their color list inside
// a scroll area: square, list, dmc, and anchor.  In those modes the user is
// presented with a grid of color buttons, below which are arrows for
// browsing the colors and a comparison rectangle showing the original
// color and the currently selected color.
// Mode objects keep track of their own widgets, layouts, etc; baseDialogMode
// provides the interface required for interacting with modes.
// Image mode is its own thing and New mode hides this dialog and pops
// up a QColorDialog.  Once the user chooses New mode they can't return
// to the other modes (this dialog closes).
//

class colorDialog : public cancelAcceptDialogBase {

 public:
  enum dialogMode {CD_SQUARE, CD_LIST, CD_DMC, CD_ANCHOR, CD_IMAGE, CD_NEW};
 private:
  // sizes for the left/right color comparison box
  enum {LR_BOX = 30, LR_BORDER = 5};

Q_OBJECT

 public:
  // <listColors> for list color mode, <inputColor> is the old color
  // the new color will replace, if <type> isn't flossVariable then
  // the dialog will only allow the user to choose a color of the
  // specified type
  colorDialog(const QVector<triC>& listColors, const triC& inputColor,
              flossType type, int frameWidth, int frameHeight);
  // As for the other constructor, with squareColors for
  // Square Color mode (intended to be colors from squares close to the
  // <inputColor> square).
  colorDialog(const QVector<triC>& squareColors,
              const QVector<triC>& listColors, const triC& inputColor,
              flossType type, int frameWidth, int frameHeight);
  // in image mode only, used to notify the dialog that the mouse has
  // moved over the image to <color>
  void updateMouseMove(QRgb color);
  // inb image mode only, used to notify the dialog that the mouse has
  // been clicked on <color>
  void updateMouseClick(QRgb color);
  bool modeIsImageMode() const { return curMode_ == &imageMode_; }

 private:
  // if <useSquareColors> is true than we include the Square Colors mode
  void constructorHelper(bool useSquareColors, flossType type);
  // emits colorDialogClosing()
  void closeEvent(QCloseEvent* event);
  QSize sizeHint() const;
  // size the dialog to fit the current mode's widgets
  void fitDialog();
  // Hide this dialog and pop up a QColorDialog - we save the user's color
  // choice if they made one and then call processAcceptClick(), else we
  // call processCancelClick().  In either case this dialog finishes.
  void activateNewChooser();
  void keyPressEvent(QKeyEvent* event);
  void setCurMode(dialogMode mode);
  // Emit finished(QDialog::Accepted, inputColor_, flossColor) and
  // close() the dialog, where flossColor's color is colorSelected_
  // and its type is determined by curMode_ (<fromNewChooser> is true
  // if we're called from the new color dialog).
  void processMaybeNewAcceptClick(bool fromNewChooser = false);

 private slots:
  // Called by color buttons in scrolling modes
  // Set colorSelected_ to <color> and save the coordinates of the selected
  // color in the appropriate mode map for the current mode.
  // Update the color selected portion in the leftRightColorPatch_.
  void setColorSelected(QRgb color, int x, int y);
  // Emit finished(QDialog::Rejected, inputColor_, flossColor()) and
  // close() the dialog.
  void processCancelClick();
  void processAcceptClick() { processMaybeNewAcceptClick(false); }
  // Called when the user chooses a new mode; <index> is the index of the
  // user's choice from the drop down menu.  Return if the mode is already
  // in use, otherwise hide the old mode widgets and load the new mode
  // widgets.
  void processModeChange(int index);
  // Process a left click on the leftRight widget: "click" on the color
  // one to the left in the grid, setting its focus and making it the
  // current color; update the color browse buttons if there are now no
  // more colors to the left.
  void processLeftClick();
  void processRightClick();

 signals:
  // emitted when this dialog is finished - returnCode is either
  // QDialog::Accepted or Rejected, oldColor is the color the user
  // submitted to this dialog to be changed, newColor is the last color
  // the user chose from this dialog
  void finished(int returnCode, const triC& oldColor,
                const flossColor& newColor);
  void colorDialogClosing();

 private:
  QVBoxLayout* dialogLayout_; // the main dialog layout

  // the old color the user is changing
  triC inputColor_;
  // the last color selected (to be returned to the user on accept)
  triC colorSelected_;

  flossType flossType_;
  QVector<triC> listColors_;
  QVector<triC> squareColors_;

  squareDialogMode squareMode_;
  listDialogMode listMode_;
  dmcDialogMode dmcMode_;
  anchorDialogMode anchorMode_;
  imageDialogMode imageMode_;
  baseDialogMode* curMode_;

  // this widget can't know them until it's already visible, so we pass
  // them in to get the initial dimensions right...
  int frameWidth_, frameHeight_;

  //// left right arrows for the "scrolling" modes
  QPushButton* leftButton_;
  QPushButton* rightButton_;
  QLabel* leftRightColorPatch_;
  QHBoxLayout* leftRightLayout_;
  QWidget* leftRightHolder_;

  // the drop down list the user chooses the mode from
  QComboBox* modeChoiceBox_;
  QHBoxLayout* buttonsLayout_;
};

Q_DECLARE_METATYPE(colorDialog::dialogMode);

#endif
