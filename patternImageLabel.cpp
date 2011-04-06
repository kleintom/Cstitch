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

#include "patternImageLabel.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

void patternImageLabel::paintEvent(QPaintEvent* event) {

  // we only draw the portion of the image in the viewing rectangle
  const QRect viewRect = event->rect();
  const int xBoxStart = viewRect.x()/patternDim_;
  const int xStart = xBoxStart * patternDim_;
  const int xEnd = viewRect.x() + viewRect.width();
  const int yBoxStart = viewRect.y()/patternDim_;
  const int yStart = yBoxStart * patternDim_;
  const int yEnd = viewRect.y() + viewRect.height();
  QPainter painter(this);
  const QHash<QRgb, QPixmap> imageHash =
    viewingSquareImage_ ? squares_ : symbols_;
  for (int j = yStart, yBox = yBoxStart;
       j < yEnd; j += patternDim_, ++yBox) {
    for (int i = xStart, xBox = xBoxStart;
          i < xEnd; i += patternDim_, ++xBox) {
      painter.drawPixmap(QPoint(i, j),
                         imageHash[squareImage_.pixel(xBox*squareDim_,
                                                      yBox*squareDim_)]);
    }
  }
  if (gridOn_) {
    painter.setPen(QColor(gridColor_));
    for (int i = xStart; i < xEnd; i += patternDim_) {
      painter.drawLine(QPoint(i, yStart), QPoint(i, yEnd));
    }
    for (int j = yStart; j < yEnd; j += patternDim_) {
      painter.drawLine(QPoint(xStart, j), QPoint(xEnd, j));
    }
  }
}

void patternImageLabel::mousePressEvent(QMouseEvent* event) {

  if (underMouse()) {
    emit announceImageClick(event);
    event->accept();
  }
}

QSize patternImageLabel::sizeHint() const {

  return QSize(width_, height_);
}
