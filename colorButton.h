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

#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include "imageUtility.h"

#include <QtGui/QToolButton>
#include <QtGui/QColor>

class QIcon;
class QEvent;

// a QToolButton that emits a color and a pair of ints when the button
// receives focus (in particular when it's clicked).
// [Used in colorDialog, where the buttons represent colors and are
// arranged in a grid.]
class colorButton : public QToolButton {

  Q_OBJECT

 public:
  // Create a QToolButton with the given <icon> and associated <color>
  // and <coordinates>
  colorButton(const QIcon& icon, QRgb color,
	      const pairOfInts& coordinates);
  QRgb color() const { return color_; }
  pairOfInts coordinates() const { return coordinates_; }

 private:
  // emit colorClicked(color_, coordinates_) on a focus in event
  bool event(QEvent* event);

 private slots:
  // used to capture the QToolButton's clicked() signal.  Emits
  // colorClicked(color_, coordinates_).
  void buttonClicked();

 signals:
  // emits color_, coordinates_ when this button is clicked
  void colorClicked(QRgb color, const pairOfInts& coordinates);

 private:
  QRgb color_;
  pairOfInts coordinates_;
};

#endif
