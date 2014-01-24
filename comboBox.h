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

#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QtWidgets/QComboBox>

// History:
// comboBox used to be a combination of a QComboBox and a QSpinDialog,
// allowing the user to click on the current item to get a drop down list
// of other items or to browse the items on the list using up/down
// spin dialog buttons, but the UI implementation didn't work cross
// platform, so I scrapped it.
//
// Also removed keyPressEvent code for up/down since the main window
// widget always steals focus after each key press (and the main window
// already implements shift-up/down for the same behavior)

// Now comboBox is just a QComboBox with convenience previous and next
// item methods that wrap around
class comboBox : public QComboBox {

 public:
  explicit comboBox(QWidget* parent) : QComboBox(parent) {}
  // make the previous item the current item (circular)
  void moveToPreviousItem();
  void moveToNextItem();
};

#endif
