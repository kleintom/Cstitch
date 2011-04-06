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

#include "detailToolDock.h"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QSpinBox>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

#include "utility.h"

detailToolDock::detailToolDock(QWidget* parent)
  : QWidget(parent) {

  numColorsLabel_ = new QLabel(tr("Colors"), this);
  numColorsBox_ = new QSpinBox(this);
  numColorsBox_->setRange(1, 99);
  numColorsBox_->setValue(8);
  numColorsLayout_ = new QHBoxLayout();
  numColorsLayout_->addWidget(numColorsLabel_);
  numColorsLayout_->addWidget(numColorsBox_);

  clearButton_ = new QPushButton(tr("Clear"), this);
  clearButton_->setEnabled(false);
  detailButton_ = new QPushButton(tr("Detail"), this);
  detailButton_->setEnabled(false);
  buttonLayout_ = new QHBoxLayout();
  buttonLayout_->addWidget(clearButton_);
  buttonLayout_->addWidget(detailButton_);
  connect(clearButton_, SIGNAL(clicked()),
          this, SLOT(processClearCall()));
  connect(detailButton_, SIGNAL(clicked()),
          this, SLOT(processDetailCall()));

  dockLayout_ = new QVBoxLayout();
  dockLayout_->addLayout(numColorsLayout_);
  dockLayout_->addLayout(buttonLayout_);

  setLayout(dockLayout_);

  int fixedHeight = 2*sHeight("D") + 40;
  setMinimumHeight(fixedHeight);
  setMaximumHeight(fixedHeight);
}

void detailToolDock::processDetailCall() {

  emit detailCalled(numColorsBox_->value());
  detailListIsEmpty(true);
}

void detailToolDock::processClearCall() {

  emit clearCalled();
  detailListIsEmpty(true);
}

void detailToolDock::detailListIsEmpty(bool b) {

  clearButton_->setEnabled(!b);
  detailButton_->setEnabled(!b);
}
