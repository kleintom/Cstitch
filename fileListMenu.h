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

#ifndef FILELISTMENU_H
#define FILELISTMENU_H

#include <QtWidgets/QMenu>

class QStringList;
class QString;
class QAction;

class fileListMenu : public QMenu {

  Q_OBJECT

public:
  fileListMenu(const QString& menuName, int maxLength,
               const QStringList& files);
  // Prepend <file> to the list (removing duplicates if necessary).
  void prependFile(const QString& file);
  // Remove <file> from the file list.
  void remove(const QString& file);
  // Return the list of files on this menu.
  QStringList files() const;
  bool isEmpty() const { return actions().length() == 0; }

signals:
  void triggered(const QString& path) const;

private slots:
  void actionTriggered(QAction* action) const;

private:
  const int maxLength_;
};

#endif
