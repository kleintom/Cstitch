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

#include "fileListMenu.h"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

#include <QtWidgets/QAction>

#include "utility.h"

fileListMenu::fileListMenu(const QString& menuName, int maxLength,
                           const QStringList& files)
  : QMenu(menuName), maxLength_(maxLength) {

  const int listSize = qMin(maxLength_, files.size());
  for (int i = 0; i < listSize; ++i) {
    const QString thisFile = files[i];
    const QFileInfo thisInfo(thisFile);
    QAction* thisAction = new QAction(thisInfo.fileName(), this);
    thisAction->setData(QVariant::fromValue(thisFile));
    addAction(thisAction);
  }

  connect(this, SIGNAL(triggered(QAction* )),
          this, SLOT(actionTriggered(QAction* )));
}

void fileListMenu::prependFile(const QString& file) {
  
  QList<QAction*> oldList = actions();
  // remove any duplicate
  for (int i = 0, size = oldList.size(); i < size; ++i) {
    const QString thisPath = oldList[i]->data().toString();
    if (thisPath == file) {
      removeAction(oldList[i]);
      oldList.removeAt(i);
      break;
    }
  }

  // prepend the new action
  const QFileInfo info(file);
  QAction* fileAction = new QAction(info.fileName(), this);
  fileAction->setData(QVariant::fromValue(file));
  if (oldList.isEmpty()) {
    addAction(fileAction);
  }
  else {
    insertAction(oldList[0], fileAction);
  }

  // enforce the length limit
  while (oldList.size() + 1 > maxLength_) {
    removeAction(oldList[oldList.size() - 1]);
    oldList.removeLast();
  }
}

void fileListMenu::remove(const QString& file) {

  const QList<QAction*> actionList = actions();
  for (int i = 0, size = actionList.size(); i < size; ++i) {
    const QString thisPath = actionList[i]->data().toString();
    if (thisPath == file) {
      removeAction(actionList[i]);
      return;
    }
  }
}

void fileListMenu::actionTriggered(QAction* action) const {
 
  emit triggered(action->data().toString());
}

QStringList fileListMenu::files() const {

  QStringList returnList;
  const QList<QAction*> actionList = actions();
  for (int i = 0, size = actionList.size(); i < size; ++i) {
    returnList.push_back(actionList[i]->data().toString());
  }
  
  return returnList;
}
  
