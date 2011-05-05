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

#ifndef CANCELACCEPTDIALOGBASE_H
#define CANCELACCEPTDIALOGBASE_H

#include <QtGui/QDialog>

class QDialogButtonBox;
class QWidget;

// A base class for dialogs that use a cancel/accept combination.
// Users call cancelAcceptWidget() to get the widget containing the
// buttons - the accept button is by default default (it accepts an
// enter on the dialog).  The dialog calls
// QDialog::done(QDialog::Accepted) on accept and
// QDialog::done(QDialog::Rejected) on cancel [but these are virtual]
// Note: QDialogButtonBox handles the business of ordering the buttons
// per current os guidelines.

class cancelAcceptDialogBase : public QDialog {

  Q_OBJECT

 public:
  explicit cancelAcceptDialogBase(QWidget* parent);

 protected:
  QWidget* cancelAcceptWidget() const;
  // enable or disable the accept button
  void enableAcceptButton(bool b);

 private slots:
  virtual void processCancelClick();
  virtual void processAcceptClick();

 private:
  QDialogButtonBox* cancelAcceptWidget_;
};

#endif
