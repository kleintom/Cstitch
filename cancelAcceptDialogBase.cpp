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

#include "cancelAcceptDialogBase.h"

#include <QtGui/QDialogButtonBox>
#include <QtGui/QWidget>
#include <QtGui/QPushButton>

cancelAcceptDialogBase::cancelAcceptDialogBase(QWidget* parent)
  : QDialog(parent),
    cancelAcceptWidget_(new QDialogButtonBox(QDialogButtonBox::Ok |
                                             QDialogButtonBox::Cancel,
                                             Qt::Horizontal, this)) {

  connect(cancelAcceptWidget_, SIGNAL(accepted()),
          this, SLOT(processAcceptClick()));

  connect(cancelAcceptWidget_, SIGNAL(rejected()),
          this, SLOT(processCancelClick()));
}

void cancelAcceptDialogBase::processCancelClick() {

  done(QDialog::Rejected);
}

void cancelAcceptDialogBase::processAcceptClick() {

  done(QDialog::Accepted);
}

void cancelAcceptDialogBase::enableAcceptButton(bool b) {
  
  cancelAcceptWidget_->button(QDialogButtonBox::Ok)->setEnabled(b);
}

QWidget* cancelAcceptDialogBase::cancelAcceptWidget() const {
  
  return cancelAcceptWidget_; 
}
