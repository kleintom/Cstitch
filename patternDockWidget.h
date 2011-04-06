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

#ifndef PATTERNDOCKWIDGET_H
#define PATTERNDOCKWIDGET_H

#include <QtGui/QWidget>

#include "triC.h"

template<class T1, class T2> struct QPair;
class QPixmap;
class QListWidget;
class QLabel;
class QVBoxLayout;

typedef QPair<QRgb, QPixmap> symbolPair;

// the symbol list dock widget for patternWindow: provides a list of
// symbols and a label giving the number of symbols
class patternDockWidget : public QWidget {

  Q_OBJECT

 public:
  // <patternSymbolSize> is the size of the symbol icons displayed
  // on the list
  patternDockWidget(int patternSymbolSize, QWidget* parent);
  void setSymbolList(const QHash<QRgb, QPixmap>& colorSymbols);

 public slots:
  // change the symbol for <color> to <symbol>
  void changeSymbol(QRgb color, const QPixmap& symbol);

 private:
  // create and return the list item icon for <color>, using <symbol>
  QIcon createIcon(QRgb color, const QPixmap& symbol) const;
  // update the symbol count label to indicate <numSymbols> symbols
  void setSymbolCountLabel(int numSymbols);
  // return a list of (color, symbol) pairs, sorted by color intensity and
  // coming from <hash>
  QVector<symbolPair>
    sortHashByIntensity(const QHash<QRgb, QPixmap>& hash) const;
  // generate the number of symbols string from <numSymbols>
  QString numSymbolsStringFromInt(int numSymbols) const;
  // return the size of the list item icons
  QSize listIconSize() const { return QSize(4*symbolSize_ - 1, symbolSize_); }

 private slots:
  void processContextRequest(const QPoint& point);

 signals:
  // emitted when the user makes a change symbol context request on an item
  void changeSymbol(const triC& color);

 private:
  int symbolSize_; // size of the symbol icon for list items
  QListWidget* symbolList_;
  QLabel* numSymbolsLabel_; // displays the number of symbols in the list
  QVBoxLayout* dockLayout_;
  // context action to change the symbol for a list item
  QAction* changeSymbolAction_;
};

#endif
