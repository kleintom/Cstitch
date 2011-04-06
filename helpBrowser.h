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

#ifndef HELPBROWSER_H
#define HELPBROWSER_H

#include <QtGui/QMainWindow>

class QTextBrowser;
class QToolBar;
class QAction;

// naked enums can't be forward declared (yet!), so wrap it
class helpMode {
 public:
  enum helpValue {H_INVALID, H_COLOR_CHOOSER, H_COLOR_COMPARE, H_SQUARE,
                  H_PATTERN};
  helpMode(helpValue mode) : mode_(mode) {}
  helpValue value() const { return mode_; }
  bool operator==(helpMode otherMode) const {
    return otherMode.mode_ == mode_;
  }
 private:
  helpValue mode_;
};

//
// The helpBrowser is a main window for displaying help text for the four
// main windows; each window gets its own page, and there is also an
// introduction page giving an overview of the program and the main
// windows.
//
//// Implementation notes
//
// Users call the static loadHelp with one of the helpModes to load the
// help text for the requested mode and display the help window.  The
// help texts are ordered from overview and then as helpMode is; each
// help text includes links to the previous and next help text as well
// as the overview.
//
class helpBrowser : public QMainWindow {

  Q_OBJECT

 public:
  helpBrowser();
  // load the help page for <mode> and show it
  static void loadHelp(helpMode mode);

 private slots:
  // show the print dialog to print the current page of help text
  void processPrint();
  // show the print dialog to print all help text
  void processPrintAll();
  // move back in the browser history
  void processBack();
  // move forward in the browser history
  void processForward();
  // enable the back button if <available>
  void backwardAvailable(bool available) const;
  // enable the forward button if <available>
  void forwardAvailable(bool available) const;

 private:
  // load the help browser with the text for <mode>
  void loadBrowser(helpMode mode);

 private:
  // the actual helpBrowser that gets displayed
  static helpBrowser* browserWindow_;
  // where the actual help text is displayed (supports limited html)
  QTextBrowser* browser_;

  QToolBar* toolBar_;

  // print the current page
  QAction* printAction_;
  // print all pages
  QAction* printAllAction_;
  // move back in the browser history
  QAction* backAction_;
  // move forward in the browser history
  QAction* forwardAction_;
};

#endif
