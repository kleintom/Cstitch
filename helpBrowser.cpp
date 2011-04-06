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

#include "helpBrowser.h"

#include <QtCore/QtDebug>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>
#include <QtGui/QTextBrowser>
#include <QtGui/QToolBar>
#include <QtGui/QAction>

#include "utility.h"

helpBrowser* helpBrowser::browserWindow_ = NULL;

helpBrowser::helpBrowser() {

  browser_ = new QTextBrowser(this);
  connect(browser_, SIGNAL(forwardAvailable(bool )),
          this, SLOT(forwardAvailable(bool )));
  connect(browser_, SIGNAL(backwardAvailable(bool )),
          this, SLOT(backwardAvailable(bool )));
  setCentralWidget(browser_);

  printAction_ = new QAction(QIcon(":print.png"), tr("Print this help page"), this);
  connect(printAction_, SIGNAL(triggered()),
          this, SLOT(processPrint()));
  printAllAction_ =
    new QAction(QIcon(":print_all.png"), tr("Print all help pages"), this);
  connect(printAllAction_, SIGNAL(triggered()),
          this, SLOT(processPrintAll()));
  backAction_ = new QAction(QIcon(":leftArrow.png"), tr("Back"), this);
  backAction_->setEnabled(false);
  connect(backAction_, SIGNAL(triggered()),
          this, SLOT(processBack()));
  forwardAction_ = new QAction(QIcon(":rightArrow.png"), tr("Forward"), this);
  forwardAction_->setEnabled(false);
  connect(forwardAction_, SIGNAL(triggered()),
          this, SLOT(processForward()));

  toolBar_ = addToolBar(tr("Toolbar"));
  toolBar_->addAction(printAction_);
  toolBar_->addAction(printAllAction_);
  toolBar_->addAction(backAction_);
  toolBar_->addAction(forwardAction_);
  resize(800, 600);
}

void helpBrowser::loadHelp(helpMode mode) {

  if (browserWindow_ == NULL) {
    browserWindow_ = new helpBrowser();
    browserWindow_->setWindowTitle(tr("Help"));
  }
  browserWindow_->loadBrowser(mode);
  browserWindow_->show();
}

void helpBrowser::processPrint() {

  QPrinter printer;
  //printer.setOutputFileName("helpOut.pdf");
  QPrintDialog printDialog(&printer, this);
  const int dialogReturnCode = printDialog.exec();
  if (dialogReturnCode == QDialog::Accepted) {
    browser_->print(&printer);
  }
}

void helpBrowser::processPrintAll() {

  // to print all we prune the individual html help files by hand and then
  // concatenate them
  QRegExp head("<html>.*</head>");
  QRegExp next("Next: *<a.*\\n");
  next.setMinimal(true);
  QRegExp top("Top: *<a.*\\n");
  top.setMinimal(true);
  QRegExp previous("Previous: *<a.*\\n");
  previous.setMinimal(true);
  QRegExp foot("</body>.*");
  QRegExp newUser("<h1>New User.*</p>");
  newUser.setMinimal(true);
  QRegExp dmc("<h1>A Note On DMC.*</p>");
  dmc.setMinimal(true);

  QString overview(::readTextFile(":/overview.html"));
  overview.replace(next, "").replace(top, "").replace(previous, "").replace(foot, "");

  QString colorChooser(::readTextFile(":/colorChooser.html"));
  colorChooser.replace(head, "").replace(top, "").replace(previous, "").replace(next, "").replace(foot, "").replace(newUser, "").replace(dmc, "");

  QString colorcompare(::readTextFile(":/colorCompare.html"));
  colorcompare.replace(head, "").replace(top, "").replace(previous, "").replace(next, "").replace(foot, "");

  QString square(::readTextFile(":/square.html"));
  square.replace(head, "").replace(top, "").replace(previous, "").replace(next, "").replace(foot, "");

  QString pattern(::readTextFile(":/pattern.html"));
  pattern.replace(head, "").replace(top, "").replace(previous, "").replace(next, "");

  QString newText(overview + colorChooser + colorcompare + square + pattern);

  QPrinter printer;
  //printer.setOutputFileName("helpOutAll.pdf");
  QPrintDialog printDialog(&printer, this);
  const int dialogReturnCode = printDialog.exec();
  if (dialogReturnCode == QDialog::Accepted) {
    QTextEdit editor(newText);
    editor.print(&printer);
  }
}

void helpBrowser::processBack() {

  browser_->backward();
}

void helpBrowser::processForward() {

  browser_->forward();
}

void helpBrowser::loadBrowser(helpMode mode) {

  QUrl url;
  switch(mode.value()) {
  case(helpMode::H_COLOR_CHOOSER):
    url = "qrc:/colorChooser.html";
    break;
  case(helpMode::H_COLOR_COMPARE):
    url = "qrc:/colorCompare.html";
    break;
  case(helpMode::H_SQUARE):
    url = "qrc:/square.html";
    break;
  case(helpMode::H_PATTERN):
    url = "qrc:/pattern.html";
    break;
  default:
    qWarning() << "Help mode not yet implemented:" << mode.value();
  }
  browser_->setSource(url);
}

void helpBrowser::backwardAvailable(bool available) const {

  backAction_->setEnabled(available);
}

void helpBrowser::forwardAvailable(bool available) const {

  forwardAction_->setEnabled(available);
}
