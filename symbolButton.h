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

#ifndef SYMBOLBUTTON_H
#define SYMBOLBUTTON_H

#include <QtWidgets/QToolButton>

//
// symbolButton is a QToolButton that emits a user-supplied index when
// clicked
//
class symbolButton : public QToolButton {

  Q_OBJECT

 public:
  symbolButton(const QIcon& icon, int index);

 private:
  bool event(QEvent* event);

 private slots:
  void buttonClicked();

 signals:
  void itemClicked(int index);

 private:
  const int index_;
};

#endif
