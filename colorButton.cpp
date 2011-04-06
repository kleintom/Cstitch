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

#include "colorButton.h"

#include <QtCore/QEvent>

colorButton::colorButton(const QIcon& icon, QRgb color, 
			 const pairOfInts& coordinates)
  : color_(color), coordinates_(coordinates) {

  setIcon(icon);
  connect(this, SIGNAL(clicked()),
	  this, SLOT(buttonClicked()));
}
  
void colorButton::buttonClicked() {
  setFocus();
}

bool colorButton::event(QEvent* event) {

  if (event->type() == QEvent::FocusIn) {
    emit colorClicked(color_, coordinates_);
    return true;
  }
  return QToolButton::event(event);
}
