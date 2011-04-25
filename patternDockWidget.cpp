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

#include "patternDockWidget.h"

#include <QtCore/QDebug>

#include <QtGui/QListWidget>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMenu>
#include <QtGui/QPainter>

patternDockWidget::patternDockWidget(int patternSymbolSize, QWidget* parent)
  : constWidthDock(parent), symbolSize_(patternSymbolSize) {

  symbolList_ = new QListWidget(this);
  symbolList_->setIconSize(listIconSize() + QSize(0, 4));
  symbolList_->setContextMenuPolicy(Qt::CustomContextMenu);
  symbolList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  changeSymbolAction_ = new QAction(tr("Change symbol"), this);

  // a label for the number of symbols in the list
  numSymbolsLabel_ = new QLabel(numSymbolsStringFromInt(0), this);
  const QFontMetrics fontMetric(font());
  numSymbolsLabel_->setFixedHeight(fontMetric.boundingRect("D").height() + 12);
  numSymbolsLabel_->setAlignment(Qt::AlignCenter);

  dockLayout_ = new QVBoxLayout(this);
  dockLayout_->addWidget(numSymbolsLabel_);
  dockLayout_->addWidget(symbolList_);
  setLayout(dockLayout_);

  connect(symbolList_, SIGNAL(customContextMenuRequested(const QPoint &)),
          this, SLOT(processContextRequest(const QPoint &)));
}

void patternDockWidget::
setSymbolList(const QHash<QRgb, QPixmap>& colorSymbols) {

  symbolList_->clear();
  const QVector<symbolPair> sortedList = sortHashByIntensity(colorSymbols);
  for (QVector<symbolPair>::const_iterator it = sortedList.begin(),
          end = sortedList.end(); it != end; ++it) {
    QListWidgetItem* listItem =
      new QListWidgetItem(createIcon((*it).first, (*it).second), "",
                          symbolList_);
    // also store the color values for convenience
    listItem->setData(Qt::UserRole, QVariant(QColor((*it).first)));
  }
  setSymbolCountLabel(symbolList_->count());
}

class symbolPairIntensity {
 public:
  bool operator()(const symbolPair& p1, const symbolPair& p2) const {
    return triC(p1.first).intensity() < triC(p2.first).intensity();
  }
};

QVector<symbolPair> patternDockWidget::
sortHashByIntensity(const QHash<QRgb, QPixmap>& hash) const {

  QVector<symbolPair> returnVector;
  returnVector.reserve(hash.size());
  for (QHash<QRgb, QPixmap>::const_iterator it = hash.begin(),
         end = hash.end(); it != end; ++it) {
    returnVector.push_back(symbolPair(it.key(), it.value()));
  }
  qSort(returnVector.begin(), returnVector.end(), symbolPairIntensity());
  return returnVector;
}

void patternDockWidget::processContextRequest(const QPoint& point) {

  if (QListWidgetItem* listItem = symbolList_->itemAt(point)) {
    QMenu* contextMenu = new QMenu();
    contextMenu->addAction(changeSymbolAction_);
    const QAction* returnAction = contextMenu->exec(QCursor::pos());
    if (returnAction == changeSymbolAction_) {
      const QColor colorToChange =
        listItem->data(Qt::UserRole).value<QColor>();
      emit changeSymbol(colorToChange);
    }
    delete contextMenu;
  } else {
    qWarning() << "No context item under point in pattern." << point;
  }
}

void patternDockWidget::changeSymbol(QRgb color,
                                     const QPixmap& symbol) {

  const QColor qColor(color);
  QList<QListWidgetItem*> listItems =
    symbolList_->findItems("", Qt::MatchStartsWith); // get them all
  for (QList<QListWidgetItem*>::iterator it = listItems.begin(),
         end = listItems.end(); it != end; ++it) {
    if ((*it)->data(Qt::UserRole).value<QColor>() == qColor) {
      (*it)->setIcon(createIcon(color, symbol));
      symbolList_->setCurrentItem(*it);
      break;
    }
  }
}

QIcon patternDockWidget::createIcon(QRgb color,
                                    const QPixmap& symbol) const {

  QPixmap listImage(listIconSize());
  listImage.fill(color);
  QPainter painter(&listImage);
  painter.drawPixmap(0, 0, symbol);
  QPixmap iconPixmap(listIconSize() + QSize(0, 4));
  iconPixmap.fill(Qt::white);
  QPainter painter2(&iconPixmap);
  painter2.drawPixmap(0, 2, listImage);
  return QIcon(iconPixmap);
}

void patternDockWidget::setSymbolCountLabel(int numSymbols) {

  numSymbolsLabel_->setText(numSymbolsStringFromInt(numSymbols));
}

QString patternDockWidget::numSymbolsStringFromInt(int numSymbols) const {

  return QString::number(numSymbols).append((numSymbols > 1) ? " symbols" : " symbol");
}
