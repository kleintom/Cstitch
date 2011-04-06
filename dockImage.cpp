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

#include "dockImage.h"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>

extern const int DOCK_WIDTH;

dockImage::dockImage(const QImage& originalImage, QWidget* parent)
  : QWidget(parent), originalImageRef_(originalImage),
    showingOriginal_(true), dragging_(false) {

  setFixedSize(DOCK_WIDTH, DOCK_WIDTH);
}

void dockImage::setImage(const QImage& image) {

  const int dockWidth = DOCK_WIDTH - 5; // leave a border of 5 on the right
  originalImage_ = QPixmap::fromImage(
    originalImageRef_.copy(0, 0, image.width(), image.height()).
    scaledToWidth(dockWidth));
  image_ = QPixmap::fromImage(image.scaledToWidth(dockWidth));
  setFixedSize(dockWidth, image_.height());
}

void dockImage::paintEvent(QPaintEvent* ) {

  const QPixmap pic = showingOriginal_ ? originalImage_ : image_;
  QPainter painter(this);
  painter.drawPixmap(0, 0, pic);
  painter.setPen(Qt::red);
  painter.drawRect(viewport_);
}

void dockImage::mousePressEvent(QMouseEvent* event) {

  if (event->button() == Qt::LeftButton) {
    moveViewport(event->pos());
    dragging_ = true;
  }
  else if (event->button() == Qt::RightButton) {
    showingOriginal_ = showingOriginal_ ? false : true;
    update();
  }
  event->accept();
}

void dockImage::mouseReleaseEvent(QMouseEvent* ) {

  dragging_ = false;
}

void dockImage::updateViewport(qreal xRatio1, qreal xRatio2,
                               qreal yRatio1, qreal yRatio2) {

  if (!dragging_) {
    const int x1 = xRatio1 * image_.width();
    const int x2 = xRatio2 * image_.width();
    const int y1 = yRatio1 * image_.height();
    const int y2 = yRatio2 * image_.height();
    viewport_ = QRect(QPoint(x1, y1), QPoint(x2, y2));
    maybeCorrectViewport();
    update();
  }
}

void dockImage::moveViewport(const QPoint& newCenter) {

  viewport_.moveCenter(newCenter);
  if (viewport_.x() < 0) {
    viewport_.moveLeft(0);
  }
  if (viewport_.y() < 0) {
    viewport_.moveTop(0);
  }
  maybeCorrectViewport();
  update();
  bool bottomEdge = false, rightEdge = false;
  if (viewport_.x() + viewport_.width() == image_.width() - 1) {
    rightEdge = true;
  }
  if (viewport_.y() + viewport_.height() == image_.height() - 1) {
    bottomEdge = true;
  }
  emit viewportUpdated(static_cast<qreal>(viewport_.x())/(image_.width()-1),
                       static_cast<qreal>(viewport_.y())/(image_.height()-1),
                       rightEdge, bottomEdge);
}

void dockImage::maybeCorrectViewport() {

  // moveRight apparently sets QRect.right(), which is 1 less than the
  // actual right edge (misleading Qt doc for moveRight), so we need
  // minus 2, not minus 1
  if (viewport_.x() + viewport_.width() >= width()) {
    viewport_.moveRight(width() - 2);
  }
  if (viewport_.y() + viewport_.height() >= height()) {
    viewport_.moveBottom(height() - 2);
  }
  if (viewport_.x() < 0) {
    viewport_.setLeft(0);
  }
  if (viewport_.y() < 0) {
    viewport_.setTop(0);
  }
}

void dockImage::mouseMoveEvent(QMouseEvent* event) {

  if (event->buttons() & Qt::LeftButton) {
    moveViewport(event->pos());
  }
}
