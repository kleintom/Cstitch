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

#include <QtWidgets/QApplication>
#include <QTranslator>

#include "colorChooser.h"
#include "windowManager.h"
#include "imageUtility.h"
#include "colorLists.h"

 
int main(int argc, char *argv[]) {

#ifdef Q_OS_LINUX
  QApplication::setStyle("plastique"); // motif is never the correct choice...
#endif
  QApplication app(argc, argv);
  const QString trDirectory = ":/translations";
  QTranslator translator;
  translator.load("cstitch_" + QLocale::system().name(), trDirectory);
  //translator.load("cstitch_en", trDirectory);
  app.installTranslator(&translator);

  // seed the static scroll bar width value
  ::scrollbarWidth(app.style());
  colorMatcher::initializeIntensitySpreads();

  windowManager winManager;

  // the version is updated by the git pre-commit script based on the
  // current git label and the current git revision count 
  // (if you change "@GIT-VERSION" you'll need to change the script too)
  winManager.setProgramVersion("0.9.5.26"); // @GIT-VERSION - don't touch this comment

  colorChooser colorChooserWindow(&winManager);
  colorChooserWindow.show();

  return app.exec();
}
