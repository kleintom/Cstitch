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

#ifndef DETAILTOOLDOCK_H
#define DETAILTOOLDOCK_H

#include <QtGui/QWidget>

class QHBoxLayout;
class QVBoxLayout;
class QSpinBox;
class QLabel;
class QPushButton;

// a widget containing the detail tool controls to perform detailing,
// clear detail squares, and select the number of detailing colors
class detailToolDock : public QWidget {

  Q_OBJECT

 public:
  explicit detailToolDock(QWidget* parent);
  void detailListIsEmpty(bool b);

 private slots:
  // someone pressed the detail button: emit it and reset this widget
  void processDetailCall();
  // someone pressed the clear button: emit it and reset this widget
  void processClearCall();

 signals:
  void detailCalled(int numColors) const;
  void clearCalled() const;

 private:
  // a spin box for selecting the number of colors the user wants to use
  // for detailing
  QHBoxLayout* numColorsLayout_;
  QLabel* numColorsLabel_;
  QSpinBox* numColorsBox_;

  QHBoxLayout* buttonLayout_;
  QPushButton* clearButton_;
  QPushButton* detailButton_;

  QVBoxLayout* dockLayout_;
};

#endif
