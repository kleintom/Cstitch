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

#include "imageLabel.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>

void imageLabelBase::mousePressEvent(QMouseEvent* event) {

  if (hasMouseTracking() && underMouse()) {
    emit announceImageClick(event);
    event->accept();
  }
}

void imageLabelBase::mouseMoveEvent(QMouseEvent* event) {

  // mousemove event is always called for a click drag, no matter where
  // the mouse is
  if (hasMouseTracking()) {
    const int x = event->x();
    const int y = event->y();
    if (x < width() && y < height() && x >= 0 && y >= 0) {
      emit mouseMoved(event);
      event->accept();
    }
  }
}

void imageLabelBase::mouseReleaseEvent(QMouseEvent* event) {

  // this signal _must_ be emitted regardless of whether
  // we're over the image or not
  emit mouseReleased(event);
  event->accept();
}

imageLabel::imageLabel(QWidget* parent)
  : imageLabelBase(parent), originalImage_(QPixmap()), scaledImage_(QPixmap()) {

  // don't clear window before painting
  setAttribute(Qt::WA_OpaquePaintEvent);
}

imageLabel::imageLabel(const QPixmap& image, QWidget* parent)
  : imageLabelBase(parent), originalImage_(image), scaledImage_(image) {

  // don't clear window before painting if there's no transparency
  setAttribute(Qt::WA_OpaquePaintEvent, !originalImage_.hasAlpha());
  resize(image.width(), image.height());
}

void imageLabel::paintEvent(QPaintEvent* event) {

  QPainter painter(this);
  //painter.setRenderHint(QPainter::SmoothPixmapTransform);

  const QRectF viewRectangle(QRectF(event->rect()));
  painter.drawPixmap(viewRectangle, scaledImage_, viewRectangle);
  event->accept();
}

void imageLabel::setImageAndSize(const QPixmap& image) {

  originalImage_ = image;
  setAttribute(Qt::WA_OpaquePaintEvent, !originalImage_.hasAlpha());
  scaledImage_ = image;
  resize(scaledImage_.width(), scaledImage_.height());
  update();
}

void imageLabel::updateImage(const QPixmap& image, const QRect& rectangle) {

  originalImage_ = image;
  setAttribute(Qt::WA_OpaquePaintEvent, !originalImage_.hasAlpha());
  scaledImage_ = originalImage_.scaled(scaledImage_.size());
  rectangle.isNull() ? update() : update(rectangle);
}

void imageLabel::setImageWidth(int width) {

  scaledImage_ = originalImage_.scaledToWidth(width);
  resize(scaledImage_.width(), scaledImage_.height());
  update();
}

void imageLabel::setImageHeight(int height) {

  scaledImage_ = originalImage_.scaledToHeight(height);
  resize(scaledImage_.width(), scaledImage_.height());
  update();
}

void imageLabel::setImageSize(const QSize& size) {

  scaledImage_ = originalImage_.scaled(size);
  resize(scaledImage_.size());
  update();
}
