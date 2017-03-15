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
#include <QStringBuilder>
#include <QtWidgets/QMenu>

#include "triC.h"
#include "imageProcessing.h"

dockListWidget::dockListWidget(QWidget* parent)
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
  colorList_->setAlternatingRowColors(true);
  // (I thought "show-decoration-selected: 0;" would highlight the text only,
  // but in fact it highlights/tints the icon as well (which it took me a while
  // to recognize).)
  // (It would be nice to add a little top/bottom margin/padding to the list
  // items as well, but all the ways I tried to do so only added bottom (5.8).)
  setStyleSheet(" QListWidget { alternate-background-color: #ecebea; }");

  //// a label for the number of colors in the list
  const QFontMetrics fontMetric(font);
  numColorsLabel_->setFixedHeight(fontMetric.boundingRect("D").height());
  numColorsLabel_->setAlignment(Qt::AlignCenter);

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

QListWidgetItem* dockListWidget::findColorListItem(const triC& color) const {

  const QColor qColor = color.qc();
  for (int i = 0, count = colorList_->count(); i < count; ++i) {
    QListWidgetItem* thisRow = colorList_->item(i);
    if (thisRow != NULL &&
        thisRow->data(Qt::UserRole).value<QColor>() == qColor) {
      return thisRow;
    }
  }

  return NULL;
}

void dockListWidget::moveTo(const triC& color) {

  QListWidgetItem* row = findColorListItem(color);
  if (row) {
    colorList_->setCurrentItem(row);
  }
  else {
    deselectCurrentListItem();
  }
}

QString dockListWidget::getListTextForFloss(const typedFloss& color) const {

  QString colorText;
  if (color.type() == flossDMC) {
    // (Funny note: if you don't convert the code to a QString before
    // concatenation, Qt interprets the concatenated string as a reference to a
    // resource plus the color name.)
    const QString code =
      color.code() < 0 ? "N/A" : QString::number(color.code());
    colorText = code % "\n" % color.name();
  }
  else if (color.type() == flossAnchor) {
    colorText = codeToString(color.code());
  }
  else {
    colorText = ::ctos(color.color());
  }
  return colorText;
}

void dockListWidget::setColorList(QVector<typedFloss> colors) {

  colorList_->clear();
  // sort the list by intensity
  std::sort(colors.begin(), colors.end(), typedFlossIntensity());
  // create the list items
  for (int i = 0, size = colors.size(); i < size; ++i) {
    const typedFloss thisColor = colors[i];
    const QString colorText = getListTextForFloss(thisColor);
    QListWidgetItem* listItem =
      createDockListItem(thisColor.color(), colorText, true);
    maybeAddToolTipToListItem(listItem, thisColor);
  }
  setNumColors(colorList_->count());
}

void dockListWidget::maybeAddToolTipToListItem(QListWidgetItem* item,
                                               const typedFloss& color) const {

  if (color.type() == flossDMC) {
    const QString tooltip = color.name() % "\n" % ::colorToTriple(color.color());
    item->setToolTip(tooltip);
  }
  else if (color.type() == flossAnchor) {
    item->setToolTip(::colorToTriple(color.color()));
  }
}

void dockListWidget::addListItemByIntensity(QListWidgetItem* item,
                                            int itemIntensity) {

  for (int i = 0, size = colorList_->count(); i < size; ++i) {
    const triC thisColor =
      triC(colorList_->item(i)->data(Qt::UserRole).value<QColor>());
    if (itemIntensity < thisColor.intensity()) {
      // Inserts in front of.
      colorList_->insertItem(i, item);
      return;
    }
  }

  // Append it.
  colorList_->insertItem(colorList_->count(), item);
}

void dockListWidget::addToList(const typedFloss& color) {

  QListWidgetItem* listItem = findColorListItem(color.color());
  if (!listItem) {
    const QString colorText = getListTextForFloss(color);
    listItem = createDockListItem(color.color(), colorText, false);
    maybeAddToolTipToListItem(listItem, color);
    addListItemByIntensity(listItem, color.color().intensity());
  }
  colorList_->setCurrentItem(listItem);
  setNumColors(colorList_->count());
}

QListWidgetItem* dockListWidget::createDockListItem(const triC& color,
                                                    const QString& text,
                                                    bool appendItem) {

  QListWidgetItem* listItem = NULL;
  if (appendItem) {
    listItem = new QListWidgetItem(text, colorList_);
  }
  else {
    listItem = new QListWidgetItem(text);
  }
  listItem->setData(Qt::UserRole, QVariant(color.qc()));
  listItem->setTextAlignment(Qt::AlignLeft);
  // Using "show-decoration-selected: 0;" actually highlights separately just
  // the text and just the icon :/  Do this instead(!):
  // https://stackoverflow.com/questions/5044449/how-to-change-qt-qlistview-icon-selection-highlight
  QIcon colorIcon;
  const QPixmap iconPixmap = generateIconSwatch(color);
  colorIcon.addPixmap(iconPixmap, QIcon::Normal);
  colorIcon.addPixmap(iconPixmap, QIcon::Selected);

  listItem->setIcon(colorIcon);

  return listItem;
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

  QListWidgetItem* row = findColorListItem(color);
  if (row) {
    delete row;
    setNumColors(colorList_->count());
    colorList_->setCurrentItem(NULL); // no selection
    return true;
  }
  else {
    qWarning() << "Couldn't find color in remove:" << ::ctos(color);
    return false;
  }
}

void dockListWidget::prependLayout(QHBoxLayout* layout) {

  mainLayout_->insertLayout(0, layout);
}

void dockListWidget::prependWidget(QWidget* widget) {

  mainLayout_->insertWidget(0, widget);
}

dockListSwatchWidget::dockListSwatchWidget(QWidget* parent)
  : dockListWidget(parent), colorSwatchLayout_(new QHBoxLayout),
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
