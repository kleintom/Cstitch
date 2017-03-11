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

#ifndef SQUAREDOCKWIDGET_H
#define SQUAREDOCKWIDGET_H


#include "dockListWidget.h"

class contextColorAction;
class triC;

// squareDockWidget extends dockListSwatchWidget by providing controls
// to add and remove context actions (in the form of contextColorActions)
class squareDockWidget : public dockListSwatchWidget {

  Q_OBJECT

 public:
  squareDockWidget(const QVector<triC>& colorList, QWidget* parent);
  void removeColors(const QVector<triC>& colors);
  void addColors(const QVector<triC>& colors);

 public slots:
  void addContextAction(contextColorAction* action, bool addToTop = false);
  void removeContextAction(contextColorAction* action);

 private slots:
  void processContextRequest(const QPoint& point);

 private:
  QList<contextColorAction*> contextActions_;
};

#endif
