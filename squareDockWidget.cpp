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

#include "squareDockWidget.h"

#include <QtCore/QDebug>

#include <QtWidgets/QMenu>

#include "squareDockTools.h" // for contextColorAction
#include "triC.h"

squareDockWidget::squareDockWidget(const QVector<triC>& colorList,
                                   QWidget* parent)
  : dockListSwatchWidget(parent) {

  setColorList(colorList);
}

void squareDockWidget::processContextRequest(const QPoint& point) {

  if (itemAtPoint(point)) {
    QMenu* contextMenu = new QMenu();
    for (int i = 0, size = contextActions_.size(); i < size; ++i) {
      contextMenu->addAction(contextActions_[i]);
    }
    const QAction* returnAction = contextMenu->exec(QCursor::pos());
    if (returnAction) {
      for (int i = 0, size = contextActions_.size(); i < size; ++i) {
        if (contextActions_[i] == returnAction) {
          contextActions_[i]->trigger(contextColor());
          break;
        }
      }
    }
    delete contextMenu;
  } else {
    qWarning() << "No context item under point in squareDockWidget.";
  }
}

void squareDockWidget::addContextAction(contextColorAction* action,
                                        bool addToTop) {

  if (addToTop) {
    contextActions_.prepend(action);
  }
  else {
    contextActions_.append(action);
  }
}

void squareDockWidget::removeContextAction(contextColorAction* action) {

  contextActions_.removeOne(action);
}

void squareDockWidget::removeColors(const QVector<triC>& colors) {

  for (QVector<triC>::const_iterator it = colors.begin(),
         end = colors.end(); it != end; ++it) {
    removeColorFromList(*it);
  }
}

void squareDockWidget::addColors(const QVector<triC>& colors) {

  for (QVector<triC>::const_iterator it = colors.begin(),
         end = colors.end(); it != end; ++it) {
    addToList(*it);
  }
  if (colors.size() > 1) {
    deselectCurrentListItem();
  }
}
