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

#include "buttonGrid.h"

#include <QtCore/qmath.h>
#include <QtCore/QDebug>

#include <QtWidgets/QApplication>
#include <QPainter>
#include <QPen>
#include <QMouseEvent>
#include <QDesktopWidget> 

#include "triC.h"
#include "imageUtility.h"

buttonGrid::buttonGrid(const QVector<QPixmap>& icons,
                       const QString& windowTitle,
                       QWidget* parent)
  : QWidget(parent), gridX_(-1), gridY_(-1) {

  createSwatchGrid(icons, windowTitle);
}

buttonGrid::buttonGrid(const QVector<triC>& colors, int iconSize,
                       int buttonsPerRow, const QString& windowTitle,
                       QWidget* parent)
  : QWidget(parent), gridX_(-1), gridY_(-1) {

  QVector<QPixmap> colorPixmaps;
  colorPixmaps.reserve(colors.size());
  for (int i = 0, size = colors.size(); i < size; ++i) {
    QPixmap pixmap(iconSize, iconSize);
    pixmap.fill(colors[i].qc());
    colorPixmaps.push_back(pixmap);
  }
  createSwatchGrid(colorPixmaps, windowTitle, buttonsPerRow);
}

buttonGrid::buttonGrid(const QVector<floss>& flosses, flossType type, int iconSize,
                       int buttonsPerRow, const QString& windowTitle,
                       QWidget* parent)
  : QWidget(parent), gridX_(-1), gridY_(-1) {

  createFlossGrid(flosses, type, windowTitle, iconSize, buttonsPerRow);
}

int buttonGrid::getDescriptionWidth(const QString& text) {

  QPixmap testImage(10,10);
  QPainter painter(&testImage);
  painter.setPen(descriptionsPen());
  const QFontMetrics metrics(painter.font());
  return metrics.width(text);
}

void buttonGrid::createIconsDrawSwatches(const QVector<QPixmap>& swatches) {

  const QPalette palette(QApplication::palette());
  const QColor windowColor(palette.color(QPalette::Window));
  const QColor darkColor(palette.color(QPalette::Dark));
  for (int i = 0, size = swatches.size(); i < size; ++i) {
    QPixmap pixmap(buttonWidth_, buttonHeight_);
    pixmap.fill(windowColor);
    QPainter painter(&pixmap);
    painter.setPen(QPen(darkColor, 2));
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPixmap(QPoint(6, 6), swatches[i]);
    painter.drawRoundedRect(QRect(1, 1, buttonWidth_ - 2, buttonHeight_ - 2),
                            3.5, 3.5);

    icons_.push_back(pixmap);
  }
}

void buttonGrid::addDescriptionsToIcons(const QVector<floss>& flosses,
                                        int iconSize) {

  for (int i = 0, size = icons_.size(); i < size; ++i) {
    const floss thisFloss = flosses[i];
    QPainter painter(&icons_[i]);
    painter.setPen(descriptionsPen());
    QString flossDescription;
    if (thisFloss.code() > 0) {
      flossDescription = QString::number(thisFloss.code());
    }
    if (thisFloss.name() != "") {
      if (flossDescription != "") {
        flossDescription += " - ";
      }
      flossDescription += thisFloss.name();
    }
    painter.drawText(QPoint(iconSize + 12, iconSize), flossDescription);
  }
}

void buttonGrid::setGridDimensionsAndTitle(int buttonsPerRow,
                                           const QString& windowTitle) {
  
  const int numIcons = icons_.size();
  buttonsPerRow_ = buttonsPerRow ? buttonsPerRow : ceil(sqrt(numIcons));
  if (buttonsPerRow == 0) {
    buttonsPerRow_ += buttonsPerRow_/4;
  }
  int gridRows = numIcons/buttonsPerRow_;
  if (numIcons % buttonsPerRow_ != 0) {
    ++gridRows;
  }
  setFixedSize(buttonsPerRow_ * buttonWidth_, gridRows * buttonHeight_);
  setWindowTitle(windowTitle);
}

void buttonGrid::createSwatchGrid(const QVector<QPixmap>& swatches,
                                  const QString& windowTitle, int buttonsPerRow) {

  buttonWidth_ = swatches.empty() ? 12 : swatches[0].width() + 12;
  buttonHeight_ = buttonWidth_;
  createIconsDrawSwatches(swatches);
  setGridDimensionsAndTitle(buttonsPerRow, windowTitle);
}

void buttonGrid::createFlossGrid(const QVector<floss>& flosses, flossType type,
                                 const QString& windowTitle,
                                 int iconSize, int buttonsPerRow) {

  int descriptionLength;
  
  if (type == flossDMC) {
    descriptionLength = 180;
    descriptionLength = getDescriptionWidth("8888 - Ultra V DK Emerald Greene ");
  }
  else if (type == flossAnchor) {
    descriptionLength = 50;
    descriptionLength = getDescriptionWidth("8888 ");
  }
  else {
    descriptionLength = 0;
    qWarning() << "Bad floss type in createFlossGrid: " << type.text();
  }
  buttonWidth_ = 6 + iconSize + 6 + descriptionLength + 6;
  buttonHeight_ = iconSize + 12;

  QVector<QPixmap> icons;
  icons.reserve(flosses.size());
  for (int i = 0, size = flosses.size(); i < size; ++i) {
    QPixmap pixmap(iconSize, iconSize);
    pixmap.fill(flosses[i].color().qc());
    icons.push_back(pixmap);
  }
  createIconsDrawSwatches(icons);
  addDescriptionsToIcons(flosses, iconSize);
  setGridDimensionsAndTitle(buttonsPerRow, windowTitle);
}

QSize buttonGrid::sizeHint() const {

  int numRows = icons_.size()/buttonsPerRow_;
  if (icons_.size() % buttonsPerRow_ != 0) {
    ++numRows;
  }
  return QSize(buttonsPerRow_ * buttonWidth_,  numRows * buttonHeight_);
}

void buttonGrid::paintEvent(QPaintEvent* ) {

  int i = 0, j = 0;
  QPainter painter(this);
  for (int k = 0, size = icons_.size(); k < size; ++k) {
    painter.drawPixmap(QPoint(i*buttonWidth_, j*buttonHeight_),
                       icons_[k]);
    ++i;
    if (i == buttonsPerRow_) {
      i = 0;
      ++j;
    }
  }
  if (gridX_ >= 0) { // if a button has been chosen, highlight it
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::black, 1.2, Qt::DotLine));
    painter.drawRoundedRect(QRect(gridX_ * buttonWidth_ + 4,
                                  gridY_ * buttonHeight_ + 4,
                                  buttonWidth_ - 8, buttonHeight_ - 8), 0., 0.);
    painter.restore();
  }
}

void buttonGrid::mousePressEvent(QMouseEvent* event) {

  if (event->button() == Qt::LeftButton) {
    gridX_ = event->x()/buttonWidth_;
    gridY_ = event->y()/buttonHeight_;
    const int index = gridY_ * buttonsPerRow() + gridX_;
    if (index >= icons_.size() || index < 0) {
      return;
    }
    emit buttonSelected(index);
    update();
  }
}

pairOfInts buttonGrid::getMaxGridCoordinates() const {

  const int computationLength = icons_.size() - 1;
  return pairOfInts(computationLength % buttonsPerRow_,
                    computationLength / buttonsPerRow_);
}

colorButtonGrid::colorButtonGrid(const QVector<triC>& colors, int iconSize,
                                 const QString& windowTitle, int buttonsPerRow,
                                 QWidget* parent)
  : buttonGrid(colors, iconSize, buttonsPerRow, windowTitle, parent),
    colors_(colors) { }

colorButtonGrid::colorButtonGrid(const QVector<floss>& flosses, flossType type,
                                 int iconSize, const QString& windowTitle,
                                 int buttonsPerRow, QWidget* parent)
  : buttonGrid(flosses, type, iconSize, buttonsPerRow, windowTitle, parent) {

  colors_.reserve(flosses.size());
  for (int i = 0; i < flosses.size(); ++i) {
    colors_.push_back(flosses[i].color());
  }
}

void colorButtonGrid::mousePressEvent(QMouseEvent* event) {

  if (event->button() == Qt::LeftButton) {
    const int gridX = event->x()/buttonWidth();
    const int gridY = event->y()/buttonHeight();
    const int index = gridY * buttonsPerRow() + gridX;

    // click on an empty grid tile does nothing
    if (index >= colors_.size() || index < 0) {
      return;
    }
    setButtonClickedX(gridX);
    setButtonClickedY(gridY);
    update();
    emit buttonSelected(colors_[index].qrgb(), gridX, gridY);
  }
}

void colorButtonGrid::focusButton(int x, int y) {

  const int buttonCount = y * buttonsPerRow() + x;
  if (buttonCount >= colors_.size() || buttonCount < 0) {
    return;
  }
  setButtonClickedX(x);
  setButtonClickedY(y);
  update();
  emit buttonSelected(colors_[buttonCount].qrgb(), x, y);
}

QSize colorButtonGrid::sizeHint() const {

  return buttonGrid::sizeHint();
}
