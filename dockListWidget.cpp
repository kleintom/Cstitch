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

#include "dockListWidget.h"

#include <algorithm>

#include <QtCore/QDebug>

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QPainter>
#include <QtWidgets/QMenu>

#include "triC.h"
#include "imageProcessing.h"

dockListWidget::dockListWidget(const QVector<triC>& colorList,
                               QWidget* parent)
  : constWidthDock(parent), mainLayout_(new QVBoxLayout(this)),
    colorList_(new QListWidget(this)), numColorsLabel_(new QLabel(this)) {

  colorList_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(colorList_, SIGNAL(customContextMenuRequested(const QPoint& )),
          this, SLOT(processContextRequest(const QPoint& )));
  colorList_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  colorList_->setIconSize(iconSize());
  QFont font = colorList_->font();
  font.setFamily("monaco");
  // apparently pitch means width (as in "fixed width")
  // needed for some families, not for others
  // See http://www.cfcl.com/vlb/h/fontmono.html for many more options
  font.setFixedPitch(true);
  colorList_->setFont(font);

  //// a label for the number of colors in the list
  const QFontMetrics fontMetric(font);
  numColorsLabel_->setFixedHeight(fontMetric.boundingRect("D").height());
  numColorsLabel_->setAlignment(Qt::AlignCenter);
  setNumColors(colorList.size());
  setColorList(colorList);

  //// a layout to contain the list and the labels
  mainLayout_->addWidget(numColorsLabel_);
  mainLayout_->addWidget(colorList_);
  setLayout(mainLayout_);
}

void dockListWidget::setNumColors(int numColors) {

  //: singular/plural
  const QString numColorsString = tr("%n color(s)", "", numColors);
  numColorsLabel_->setText(numColorsString);
}

void dockListWidget::clearList() {

  colorList_->clear();
  setNumColors(0);
}

void dockListWidget::moveTo(const triC& color) {

  const QList<QListWidgetItem *> foundColors =
    colorList_->findItems(::ctos(color), Qt::MatchExactly);
  if (!foundColors.empty()) {
    colorList_->setCurrentItem(foundColors[0]);
  }
  else {
    deselectCurrentListItem();
  }
}

void dockListWidget::setColorList(QVector<triC> colors) {

  colorList_->clear();
  // sort the list by intensity
  std::sort(colors.begin(), colors.end(), triCIntensity());
  // create the list items
  for (int i = 0, size = colors.size(); i < size; ++i) {
    const triC thisColor = colors[i];
    QListWidgetItem* listItem =
      new QListWidgetItem(::ctos(thisColor), colorList_);
    // also store the color values for convenience
    listItem->setData(Qt::UserRole, QVariant(thisColor.qc()));
    listItem->setTextAlignment(Qt::AlignLeft);
    listItem->setIcon(QIcon(generateIconSwatch(thisColor)));
  }
  setNumColors(colorList_->count());
}

QPixmap dockListWidget::generateIconSwatch(const triC& swatchColor) const {

  const QSize size = iconSize();
  QPixmap swatchPixmap(size);
  swatchPixmap.fill(swatchColor.qc());
  QPainter painter(&swatchPixmap);
  painter.drawLine(0, 0, size.width(), 0);
  painter.drawLine(0, 0, 0, size.height());
  return swatchPixmap;
}

void dockListWidget::processContextRequest(const QPoint& point) {

  if (QListWidgetItem* listItem = colorList_->itemAt(point)) {
    QMenu* contextMenu = new QMenu();
    const QAction* listRemoveAction =
      contextMenu->addAction(QIcon(":delete.png"), tr("Remove"));

    const QAction* returnAction = contextMenu->exec(QCursor::pos());
    if (returnAction == listRemoveAction) {
      const QColor colorToRemove =
        listItem->data(Qt::UserRole).value<QColor>();
      delete listItem;
      setNumColors(colorList_->count());
      colorList_->setCurrentItem(NULL);
      emit colorRemoved(colorToRemove);
    }
    delete contextMenu;
  } else {
    qWarning() << "No item under context point.";
  }
}

QColor dockListWidget::contextColor() {

  const QListWidgetItem* listItem = colorList_->currentItem();
  if (listItem) {
    return listItem->data(Qt::UserRole).value<QColor>();
  }
  else {
    return QColor();
  }
}

bool dockListWidget::removeColorFromList(const triC& color) {

  QList<QListWidgetItem *> colorsFound =
    colorList_->findItems(::ctos(color), Qt::MatchExactly);
  if (!colorsFound.empty()) {
    delete colorsFound[0];
    setNumColors(colorList_->count());
    colorList_->setCurrentItem(NULL); // no selection
    return true;
  }
  else {
    qWarning() << "Couldn't find color in remove:" << ctos(color);
    return false;
  }
}

void dockListWidget::prependLayout(QHBoxLayout* layout) {

  mainLayout_->insertLayout(0, layout);
}

void dockListWidget::prependWidget(QWidget* widget) {

  mainLayout_->insertWidget(0, widget);
}

void dockListWidget::addToList(const triC& color) {

  QListWidgetItem* listItem = NULL;
  const QString colorText = ::ctos(color);
  QList<QListWidgetItem*> foundItem =
    colorList_->findItems(colorText, Qt::MatchCaseSensitive);
  if (!foundItem.isEmpty()) {
    listItem = foundItem[0];
  }
  else {
    QListWidgetItem* listItem = new QListWidgetItem(::ctos(color));
    listItem->setData(Qt::UserRole, QVariant(color.qc()));
    listItem->setTextAlignment(Qt::AlignLeft);
    listItem->setIcon(QIcon(generateIconSwatch(color)));
    
    // insert by intensity
    const int inputIntensity = color.intensity();
    bool added = false;
    for (int i = 0, size = colorList_->count(); i < size; ++i) {
      const triC thisColor =
        triC(colorList_->item(i)->data(Qt::UserRole).value<QColor>());
      if (inputIntensity < thisColor.intensity()) {
        added = true;
        // inserts in front of
        colorList_->insertItem(i, listItem);
        break;
      }
    }
    if (!added) {
      colorList_->insertItem(colorList_->count(), listItem);
    }
  }
  colorList_->setCurrentItem(listItem);
  setNumColors(colorList_->count());
}

dockListSwatchWidget::dockListSwatchWidget(const QVector<triC>& colorList,
                                           QWidget* parent)
  : dockListWidget(colorList, parent), colorSwatchLayout_(new QHBoxLayout),
    colorSwatch_(new QLabel(this)), curColorString_(new QLabel(this)) {

  //// a label the user can use to display a color
  colorSwatch_->setFixedSize(swatchSize());
  colorSwatch_->setFrameStyle(QFrame::Panel | QFrame::Raised);
  colorSwatch_->setLineWidth(3);
  colorSwatch_->setAlignment(Qt::AlignCenter);
  QPixmap colorPixmap = QPixmap(swatchSize());
  colorPixmap.fill(Qt::black);
  colorSwatch_->setPixmap(colorPixmap);
  colorSwatchCache_ = QPixmap(swatchSize());
  colorSwatchCacheColor_ = Qt::black;

  colorSwatchLayout_->addStretch(1);
  colorSwatchLayout_->addWidget(colorSwatch_);
  colorSwatchLayout_->addStretch(1);

  //// a label for the current rgb color code
  const QFontMetrics fontMetric(font());
  curColorString_->setFixedHeight(fontMetric.boundingRect("8").height());
  curColorString_->setAlignment(Qt::AlignCenter);
  QFont widgetFont = font();
  widgetFont.setFamily("monaco");
  // apparently pitch means width (as in "fixed width")
  // needed for some families, not for others
  // See http://www.cfcl.com/vlb/h/fontmono.html for many more options
  widgetFont.setFixedPitch(true);
  curColorString_->setFont(widgetFont);
  curColorString_->setText(::ctos(triC(0, 0, 0)));
  prependWidget(curColorString_);
  prependLayout(colorSwatchLayout_);
}

void dockListSwatchWidget::updateColorSwatch(QRgb color) {

  if (colorSwatchCacheColor_ == color) {
    return;
  }
  else {
    colorSwatchCacheColor_ = color;
    colorSwatchCache_.fill(QColor(color));
    colorSwatch_->setPixmap(colorSwatchCache_);
    curColorString_->setText(::ctos(color));
  }
}
