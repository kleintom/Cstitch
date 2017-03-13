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

#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QListWidget>

#include "constWidthDock.h"

class triC;
class typedFloss;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;

//
// A widget for displaying a list of colors and a "number of colors" label.
//
// Implementation note: Different methods for adding colors to the list display
// the colors in different ways, but each add method is required to add the
// color of each list item as data on that item, which is the sum total of what
// other methods get to assume about the structure of list items.
//
class dockListWidget : public constWidthDock {

  Q_OBJECT

 public:
  dockListWidget(QWidget* parent);
  bool removeColorFromList(const triC& color);
  // move to and select the entry for <color> if it exists
  void moveTo(const triC& color);
  void clearList();
  void deselectCurrentListItem() { colorList_->setCurrentItem(NULL); }
  void setColorList(QVector<triC> colors);
  void setColorList(QVector<typedFloss> colors);
  // add <color> to the list and highlight it
  void addToList(const triC& color);
  void addToList(const typedFloss& color);
  // update the numColorsLabel_
  void setNumColors(int numColors);
  // enable or disable context menus on the color list
  void enableContextMenu(bool b) {
    Qt::ContextMenuPolicy policy =
      b ? Qt::CustomContextMenu : Qt::PreventContextMenu;
    colorList_->setContextMenuPolicy(policy);
  }

 protected:
  // return the currently highlighted color
  QColor contextColor();
  bool itemAtPoint(const QPoint& point) const {
    return colorList_->itemAt(point);
  }
  void prependLayout(QHBoxLayout* layout);
  void prependWidget(QWidget* widget);

 protected slots:
  // handle a context request originated at <point>
  virtual void processContextRequest(const QPoint& point);

 private:
  // Create and return a new dock list item using the given data.  If
  // <appendItem> is true then the new item is appended to the color list;
  // otherwise the caller is responsible for inserting the item.
  QListWidgetItem* createDockListItem(const triC& color, const QString& text,
                                      bool appendItem);
  // generate a pixmap with solid color <swatchColor> to be used as an
  // icon for a list item
  QPixmap generateIconSwatch(const triC& swatchColor) const;
  // Find an item in the color list based on its color.  Returns a pointer to
  // the list item if the search was successful, otherwise returns NULL.
  QListWidgetItem* findColorListItem(const triC& color) const;
  QString getListTextForFloss(const typedFloss& color) const;
  void maybeAddToolTipToListItem(QListWidgetItem* item,
                                 const typedFloss& color) const;
  void addListItemByIntensity(QListWidgetItem* item, int itemIntensity);

 signals:
  void colorRemoved(const triC& color);

 private:
  QVBoxLayout* mainLayout_;
  QListWidget* colorList_;
  QLabel* numColorsLabel_;
};

// a dockListWidget plus a color swatch
class dockListSwatchWidget : public dockListWidget {

  Q_OBJECT

 public:
  dockListSwatchWidget(QWidget* parent = NULL);

 public slots:
  // change the color on the color swatch
  void updateColorSwatch(QRgb color);

 private:
  QHBoxLayout* colorSwatchLayout_;
  QLabel* colorSwatch_;
  QPixmap colorSwatchCache_;
  QRgb colorSwatchCacheColor_; // the current swatch color
  QLabel* curColorString_;
};

#endif
