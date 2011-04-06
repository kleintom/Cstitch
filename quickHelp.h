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

#ifndef QUICKHELP_H
#define QUICKHELP_H

#include <QtGui/QDialog>

#include "helpBrowser.h"

class helpMode;
class QWidget;
class QString;
class QCloseEvent;
class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;

class quickHelp : public QDialog {

  Q_OBJECT

 public:
  explicit quickHelp(QWidget* parent);
  static void loadQuickHelp(helpMode mode, QWidget* parent);
  static void closeCurrentWindow();

 private slots:
  void processClose();
  void moreHelpActivated(const QString& url);
  void closeEvent(QCloseEvent* event);

 private:
  void loadModeHelp(helpMode mode);

 private:
  static quickHelp* window_;
  helpMode mode_;
  QLabel* iconLabel_;
  QLabel* titleLabel_;
  QLabel* textLabel_;
  QLabel* moreHelpLabel_;
  QHBoxLayout* titleLayout_;
  QVBoxLayout* layout_;
  QPushButton* closeButton_;
};

#endif
