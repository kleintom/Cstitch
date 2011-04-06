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

#ifndef SLIDERSPINBOXDIALOG_H
#define SLIDERSPINBOXDIALOG_H

#include "cancelAcceptDialogBase.h"

class QSlider;
class QSpinBox;
class QLabel;
class QHBoxLayout;
class QVBoxLayout;

//
// A small widget that displays a string followed by a slider and a
// spinbox for setting a value; users should call the static
// getSliderSpinBoxValue, which returns the value
// of the spinbox on an "accept" click or -1 on a "cancel" click.
//
class sliderSpinBoxDialog : public cancelAcceptDialogBase {

 public:
  // <text> is the string to display before the spinbox, which has
  // min <sliderMin> and max <sliderMax>
  sliderSpinBoxDialog(const QString& text, const QString& windowTitle,
                      int sliderMin, int sliderMax, QWidget* parent);
  // returns -1 on cancel
  static int getSliderSpinBoxValue(const QString& text,
                                   const QString& windowTitle,
                                   int sliderMin, int sliderMax,
                                   QWidget* parent);

 private:
  QSlider* slider_;
  QSpinBox* spinBox_;
  QLabel* imageQualityLabel_;
  QHBoxLayout* boxLayout_;
  QVBoxLayout* widgetLayout_;
};

#endif
