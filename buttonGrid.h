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


#ifndef BUTTONGRID_H
#define BUTTONGRID_H

#include <QtWidgets/QWidget>

#include "triC.h"

//
// buttonGrid is a widget that displays a grid of square "buttons" with
// icons provided by the user; when the user clicks on a button in the
// grid the widget emits the index of the button, where index is the 0-based
// sequence number of the symbol from the original list of symbols
//
class buttonGrid : public QWidget {

  Q_OBJECT

 public:
  // icons are assumed to be square
  buttonGrid(const QVector<QPixmap>& icons, const QString& windowTitle,
             QWidget* parent);
  buttonGrid(const QVector<triC>& colors, int iconSize, int rowWidth,
             const QString& windowTitle, QWidget* parent);
  virtual QSize sizeHint() const;

 protected:
  void setButtonClickedX(int x) { gridX_ = x; }
  void setButtonClickedY(int y) { gridY_ = y; }
  int buttonsPerRow() const { return buttonsPerRow_; }
  int buttonDim() const { return buttonDim_; }

 private:
  // called by all constructors; <rowWidth> is the number of buttons
  // per row
  void constructorHelper(const QVector<QPixmap>& icons,
                         const QString& windowTitle,
                         int rowWidth = 0);
  // the grid is painted on the widget during the paint event
  void paintEvent(QPaintEvent* event);
  // emits the (0-based) index of the button clicked for a left click
  // on an occupied grid space
  virtual void mousePressEvent(QMouseEvent* event);

 signals:
  // emit the index of a button that was clicked
  void buttonSelected(int index);

 private:
  QVector<QPixmap> icons_;
  int buttonDim_; // square size of the buttons in the grid
  int buttonsPerRow_; // number of button per row in the grid
  int gridX_; // x button coordinate of the last button clicked
  int gridY_; // y button coordinate of the last button clicked
};

//
// colorButtonGrid specializes buttonGrid by specifying that the buttons
// will present color swatches, and that a click on a button will
// emit that button's color as well as its x,y position
//
class colorButtonGrid : public buttonGrid {

  Q_OBJECT

 public:
  // make the buttons solid square <colors> of size <iconSize>,
  // <rowWidth> to a row
  colorButtonGrid(const QVector<triC>& colors, int iconSize,
                  const QString& windowTitle, int rowWidth,
                  QWidget* parent);
  // "click" the button at coordinates (<x>, <y>)
  void focusButton(int x, int y);
  QSize sizeHint() const;

 private:
  // emits the button x,y coordinates and the color of the button
  // clicked for a left click on an occupied grid space
  void mousePressEvent(QMouseEvent* event);

 signals:
  // emit the x,y button coordinates and the color of a button that was
  // clicked
  void buttonSelected(QRgb color, int x, int y);

 private:
  const QVector<triC> colors_;
};

#endif
