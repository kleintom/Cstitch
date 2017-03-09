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
#include <QCoreApplication>

#include "triC.h"
#include "imageUtility.h"
#include "cancelAcceptDialogBase.h"
#include "floss.h"
#include "buttonGrid.h"

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
  void enable();
  void disable();
  virtual QSize sizeHint() const;
  QSize scrollFrameSize() const;
  int colorsPerRow() const { return grid_->buttonsPerRow(); }
  void setCurrentGridSelection(const pairOfInts& coords) {
    currentGridSelection_ = coords;
  }
  void setGridFocus(const pairOfInts& coordinates);
  pairOfInts getCurrentGridSelection() const { return currentGridSelection_; }
  pairOfInts getMaxGridCoordinates() const { return grid_->getMaxGridCoordinates(); }
  // used by imageDialogMode only
  virtual void updateImageColorLabel(QRgb , bool ) {}
  void setWidget(QWidget* widget) { isNull_ = false; widget_ = widget; }
 protected:
  void constructGrid(QVBoxLayout* dialogLayout, int colorsPerRow,
                     const QVector<triC>& colors, QWidget* parent);
  void constructGrid(QVBoxLayout* dialogLayout, int colorsPerRow,
                     const QVector<floss>& flosses, flossType type, QWidget* parent);
 private:
  // grid_ must be instantiated before calling
  void constructGridHelper(QVBoxLayout* dialogLayout, QWidget* parent);
 private:
  static const int colorSwatchSize_ = 22;
  flossType flossType_;
  QScrollArea* scrollArea_;
  colorButtonGrid* grid_;
  QWidget* widget_;
  pairOfInts currentGridSelection_;
  bool isNull_;
};

// mode for choosing a color from those in "nearby" squares
class nearbySquaresDialogMode : public baseDialogMode {
 public:
  nearbySquaresDialogMode() { }
  nearbySquaresDialogMode(QVBoxLayout* dialogLayout, QVector<triC> colors,
                          flossType type, const triC& inputColor, QWidget* parent);
};

// mode for choosing a color from the project's current color list
class colorListDialogMode : public baseDialogMode {
 public:
  colorListDialogMode() { }
  colorListDialogMode(QVBoxLayout* dialogLayout, QVector<triC> colors,
                      flossType type, const triC& inputColor, QWidget* parent);
};

// base class for modes displaying a grid of color swatches
class colorsBaseDialogMode : public baseDialogMode {
 public:
  colorsBaseDialogMode() { }
  colorsBaseDialogMode(QVBoxLayout* dialogLayout, const triC& inputColor,
                       flossType type, QVector<triC> colorList, QWidget* parent);
};

// mode for choosing a dmc color from a grid of color swatches
class dmcColorsDialogMode : public colorsBaseDialogMode {
 public:
  dmcColorsDialogMode() { }
  dmcColorsDialogMode(QVBoxLayout* dialogLayout, const triC& inputColor,
                      QWidget* parent);
};

// mode for choosing an anchor color from a grid of color swatches
class anchorColorsDialogMode : public colorsBaseDialogMode {
 public:
  anchorColorsDialogMode() { }
  anchorColorsDialogMode(QVBoxLayout* dialogLayout, const triC& inputColor,
                         QWidget* parent);
};

// mode for choosing a dmc color from a grid of color/floss description buttons
class dmcFlossesDialogMode : public baseDialogMode {
 public:
  dmcFlossesDialogMode() { }
  dmcFlossesDialogMode(QVBoxLayout* dialogLayout, QWidget* parent);
};

// mode for choosing an anchor color from a grid of color/floss description buttons
class anchorFlossesDialogMode : public baseDialogMode {
 public:
  anchorFlossesDialogMode() { }
  anchorFlossesDialogMode(QVBoxLayout* dialogLayout, QWidget* parent);
};

// mode for choosing a color from a currently displayed image
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
// The color dialog offers the user a menu of ways in which the user can choose
// a new color.  The <type> parameter passed on construction determines the
// types of colors that can be chosen using the dialog (if DMC then only DMC
// colors can be chosen, etc)
//
// Choose a square color mode:
// This mode option only appears if the user initiated the dialog by clicking on
// a square in the squared image, and then this mode allows the user to choose
// from other colors in the original image for that square; colors are passed in
// on construction (<squareColors>).
//
// Choose a color list color mode:
// Choose a color from the list passed in on construction (should consist of the
// colors on the main window color list dock).
//
// Choose a DMC floss by color mode: Choose a DMC color from color swatches.
// Choose a DMC floss by color/number mode: Choose a DMC color from color
// swatches with accompanying number/name.
//
// Choose an Anchor floss by color mode: Choose an Anchor color from color swatches.
// Choose an Anchor floss by color/number mode: Choose a DMC color from color
// swatches with accompanying number.
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
// Some modes are "scrolling" modes that show their color list inside
// a scroll area: square, list, dmc, and anchor.  In those modes the user is
// presented with a grid of color buttons, below which are arrows for
// browsing the colors and a comparison rectangle showing the original
// color and the currently selected color.
// Mode objects keep track of their own widgets, layouts, etc; baseDialogMode
// provides the interface required for interacting with modes.
// Image mode is its own thing.  New mode hides this dialog and pops
// up a QColorDialog.  Once the user chooses New mode they can't return
// to the other modes (this dialog closes).
//

class colorDialog : public cancelAcceptDialogBase {

 public:
  enum dialogMode {CD_SQUARE, CD_LIST, CD_DMC_COLOR, CD_DMC_FLOSS, 
                   CD_ANCHOR_COLOR, CD_ANCHOR_FLOSS, CD_IMAGE, CD_NEW};
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
  // As for the other constructor, with <squareColors> for
  // Square Color mode (intended to be colors from squares close to the
  // <inputColor> square).
  colorDialog(const QVector<triC>& squareColors,
              const QVector<triC>& listColors, const triC& inputColor,
              flossType type, int frameWidth, int frameHeight);
  // in image mode only, used to notify the dialog that the mouse has
  // moved over the image to <color>
  void updateMouseMove(QRgb color);
  // in image mode only, used to notify the dialog that the mouse has
  // been clicked on <color>
  void updateMouseClick(QRgb color);
  bool modeIsImageMode() const { return curMode_ == &imageMode_; }

 protected slots:
  // Emit finished(QDialog::Rejected, inputColor_, flossColor()) and
  // close() the dialog.
  // We use close() here and in processAcceptClick() instead of using the base
  // class's done() in order to unify behavior with the case where the user
  // closes the dialog by clicking its close button.
  void processCancelClick();
  void processAcceptClick() { processMaybeNewAcceptClick(false); }

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
  dialogMode getModeBoxMode() const;
  // Set the mode for the first time.
  void setInitialMode(dialogMode mode);
  void setCurMode(dialogMode mode);
  // Return true if the current dialog supports the given mode.
  bool dialogSupportsMode(dialogMode mode) const;
  dialogMode currentDefaultMode() const;
  // Emit finished(QDialog::Accepted, inputColor_, flossColor) and close() the
  // dialog, where flossColor's color is colorSelected_ and its type is
  // determined by curMode_ (<fromNewChooser> is true if we're called from the
  // new color dialog).
  void processMaybeNewAcceptClick(bool fromNewChooser = false);

 private slots:
  // Called by color buttons in scrolling modes
  // Set colorSelected_ to <color> and save the coordinates of the selected
  // color in the appropriate mode map for the current mode.
  // Update the color selected portion in the leftRightColorPatch_.
  void setColorSelected(QRgb color, int x, int y);
  // Called when the user chooses a new mode; hide the old mode widgets and load
  // the new mode widgets.
  void processModeChange();
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

  nearbySquaresDialogMode nearbySquaresMode_;
  colorListDialogMode colorListMode_;
  dmcColorsDialogMode dmcColorsMode_;
  dmcFlossesDialogMode dmcFlossesMode_;
  anchorColorsDialogMode anchorColorsMode_;
  anchorFlossesDialogMode anchorFlossesMode_;
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
