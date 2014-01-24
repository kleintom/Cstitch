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

#include "sliderSpinBoxDialog.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>

sliderSpinBoxDialog::sliderSpinBoxDialog(const QString& text,
                                         const QString& windowTitle,
                                         int sliderMin, int sliderMax,
                                         QWidget* parent)
  : cancelAcceptDialogBase(parent) {

  imageQualityLabel_ = new QLabel(text, this);

  slider_ = new QSlider(Qt::Horizontal, this);
  slider_->setMinimum(sliderMin);
  slider_->setMaximum(sliderMax);
  slider_->setValue(85); // default quality setting

  spinBox_ = new QSpinBox(this);
  spinBox_->setMinimum(sliderMin);
  spinBox_->setMaximum(sliderMax);
  spinBox_->setValue(85);

  connect(spinBox_, SIGNAL(valueChanged(int )),
          slider_, SLOT(setValue(int )));
  connect(slider_, SIGNAL(valueChanged(int )),
          spinBox_, SLOT(setValue(int )));

  boxLayout_ = new QHBoxLayout;
  boxLayout_->addWidget(slider_);
  boxLayout_->addWidget(spinBox_);

  widgetLayout_ = new QVBoxLayout;
  widgetLayout_->addWidget(imageQualityLabel_);
  widgetLayout_->addLayout(boxLayout_);
  widgetLayout_->addWidget(cancelAcceptWidget());

  setLayout(widgetLayout_);
  setWindowTitle(windowTitle);
}

int sliderSpinBoxDialog::getSliderSpinBoxValue(const QString& text,
                                               const QString& windowTitle,
                                               int sliderMin, int sliderMax,
                                               QWidget* parent) {

  sliderSpinBoxDialog dialog(text, windowTitle, sliderMin, sliderMax,
                             parent);
  const int returnCode = dialog.exec();
  if (returnCode == QDialog::Accepted) {
    return dialog.spinBox_->value();
  }
  else {
    return -1;
  }
}
