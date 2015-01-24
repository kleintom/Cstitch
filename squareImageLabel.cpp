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

#include "squareImageLabel.h"
#include "utility.h"

#include <QtCore/QDebug>

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

void squareImageLabel::paintEvent(QPaintEvent* event) {

  QPainter painter(this);
  // if we're not currently drawing a mouse drag edit, draw the underlying
  // image
  if (!drawingSquares_ && !drawingHashes_) {
    if (imageIsFlat()) { // paint the entire image at once
      const QRectF viewRectangle(QRectF(event->rect()));
      painter.drawPixmap(viewRectangle, scaledImage_, viewRectangle);
      if (imageIsOriginal_) {
        // no decorations for the original image
        event->accept();
        return;
      }
    }
    else { // draw a square image
      // we only draw the portion of the image in the viewing rectangle
      const QRect viewRect = event->rect();
      const int xBoxStart = viewRect.x()/scaledDimension_;
      const int xStart = xBoxStart * scaledDimension_;
      const int xEnd = viewRect.x() + viewRect.width();
      const int yBoxStart = viewRect.y()/scaledDimension_;
      const int yStart = yBoxStart * scaledDimension_;
      const int yEnd = viewRect.y() + viewRect.height();
      const int originalDimension = baseImage_.width()/xSquareCount_;
      for (int j = yStart, yBox = yBoxStart;
           j < yEnd; j += scaledDimension_, ++yBox) {
        for (int i = xStart, xBox = xBoxStart;
             i < xEnd; i += scaledDimension_, ++xBox) {
          painter.drawPixmap
            (QPoint(i, j),
             colorSquares_[baseImage_.pixel(xBox*originalDimension,
                                            yBox*originalDimension)]);
        }
      }
    }
  }

  if (drawingSquares_) {
    // there's no need to redraw squares we've already drawn, so start
    // with the new ones
    // NOTE: this code assumes there could be more than one addSquare
    // between paint events.  Our code calls update after each addSquare,
    // so we would expect a paint event after each add, but paintEvent
    // documentation says paintEvents can get bundled for speed
    // optimization, so to be on the safe side, don't assume they haven't
    // been!
    for (int i = lastSquareDrawn_ + 1, size = drawSquares_.size();
         i < size; ++i) {
      const int boxX = drawSquares_[i].x();
      const int boxY = drawSquares_[i].y();
      const int xStart = boxX * scaledDimension_;
      const int xEnd = xStart + scaledDimension_;
      const int xWidth = xEnd - xStart;
      const int yStart = boxY * scaledDimension_;
      const int yEnd = yStart + scaledDimension_;
      const int yWidth = yEnd - yStart;
      painter.fillRect(QRect(xStart, yStart, xWidth, yWidth),
                       QColor(squareColor_));
    }
    lastSquareDrawn_ = drawSquares_.size() - 1;
  }
  else if (!hashSquaresToBeRemoved_.isEmpty()) {
    for (int i = 0, size = hashSquaresToBeRemoved_.size();
         i < size; ++i) {
      const int boxX = hashSquaresToBeRemoved_[i].x();
      const int boxY = hashSquaresToBeRemoved_[i].y();
      const qreal xStart = boxX * scaledDimension_;
      const qreal xEnd = xStart + scaledDimension_;
      const qreal yStart = boxY * scaledDimension_;
      const qreal yEnd = yStart + scaledDimension_;
      const int xStartInt = static_cast<int>(xStart);
      const int xEndInt = static_cast<int>(xEnd);
      const int xWidth = xEndInt - xStartInt;
      const int yStartInt = static_cast<int>(yStart);
      const int yEndInt = static_cast<int>(yEnd);
      const int yWidth = yEndInt - yStartInt;
      QColor fillColor = hashSquaresToBeRemoved_[i].color();
      painter.fillRect(QRect(xStartInt, yStartInt, xWidth, yWidth),
                       fillColor);
    }
    hashSquaresToBeRemoved_.clear();
  }
  else if (!hashSquares_.isEmpty()) { // don't just check drawingHashes_
    // there are two reasons to draw hashes here: either we're actively
    // dragging a detail draw, or the detail tool is active and there are
    // some dragged hashes but we're not actively dragging and we're
    // being redrawn for some other reason (like a scroll)
    int hashSquaresStart = 0;
    if (drawingHashes_) {
      hashSquaresStart = lastHashDrawn_ + 1;
    }
    for (int i = hashSquaresStart, size = hashSquares_.size();
         i < size; ++i) {
      const int boxX = hashSquares_[i].x();
      const int boxY = hashSquares_[i].y();
      const qreal xStart = boxX * scaledDimension_;
      const qreal xEnd = xStart + scaledDimension_;
      const qreal yStart = boxY * scaledDimension_;
      const qreal yEnd = yStart + scaledDimension_;
      const int xStartInt = static_cast<int>(xStart);
      const int xEndInt = static_cast<int>(xEnd);
      const int yStartInt = static_cast<int>(yStart);
      const int yEndInt = static_cast<int>(yEnd);
      painter.setPen(hashSquares_[i].color());
      painter.drawLine(xStartInt, yStartInt,
                       xEndInt, yEndInt);
      painter.drawLine(xStartInt, yEndInt,
                       xEndInt, yStartInt);
    }
    lastHashDrawn_ = hashSquares_.size() - 1;
  }

  if (gridOn_ && scaledDimension_ > 1) {
    painter.setPen(QPen(QColor(gridColor_), 1));
    const QRect eventRectangle(event->rect());
    const int xStart = eventRectangle.x();
    const int xEnd = xStart + eventRectangle.width();
    const int yStart = eventRectangle.y();
    const int yEnd = yStart + eventRectangle.height();
    for (int i = (xStart/scaledDimension_)*scaledDimension_; i <= xEnd;
         i += scaledDimension_) {
      painter.drawLine(QPoint(i, yStart), QPoint(i, yEnd));
    }
    for (int j = (yStart/scaledDimension_)*scaledDimension_; j <= yEnd;
         j += scaledDimension_) {
      painter.drawLine(QPoint(xStart, j), QPoint(xEnd, j));
    }
  }
}

void squareImageLabel::removeHashSquare(const pixel& p) {

  // (pixels identify on coordinates, not colors)
  if (hashSquares_.removeOne(p)) {
    hashSquaresToBeRemoved_.push_back(p);
    --lastHashDrawn_;
  }
  else {
    qWarning() << "Pixel not found on remove" << hashSquares_.size() <<
      p.x() << p.y() << ctos(p.color());
  }
}

bool squareImageLabel::clearHashes() {

  drawingHashes_ = false;
  lastHashDrawn_ = -1;
  if (!hashSquaresToBeRemoved_.isEmpty()) {
    repaint();
    hashSquaresToBeRemoved_.clear();
  }
  if (hashSquares_.isEmpty()) {
    return false;
  }
  else {
    hashSquares_.clear();
    update();
    return true;
  }
}

void squareImageLabel::setNewImage(const QImage& image,
                                   const QList<QRgb>& colors,
                                   int xSquareCount, int ySquareCount,
                                   bool imageIsOriginal) {

  baseImage_ = image;
  imageIsOriginal_ = imageIsOriginal;
  setAttribute(Qt::WA_OpaquePaintEvent, !image.hasAlphaChannel());
  resize(image.size());
  xSquareCount_ = xSquareCount;
  ySquareCount_ = ySquareCount;
  scaledDimension_ = image.width()/xSquareCount_;
  generateColorSquares(colors);
  if (imageIsFlat()) {
    scaledImage_ = QPixmap::fromImage(baseImage_);
  }
  else {
    scaledImage_ = QPixmap();
  }
}

void squareImageLabel::updateImage(const QImage& image, const QList<QRgb>& colors,
                                   const QRect& updateRectangle) {

  baseImage_ = image;
  generateColorSquares(colors);
  if (updateRectangle.isNull()) {
    update();
  }
  else {
    update(updateRectangle);
  }
}

void squareImageLabel::setImageSize(const QSize& newSize) {

  if (imageIsFlat()) {
    xSquareCount_ = newSize.width();
    ySquareCount_ = newSize.height();
    scaledImage_ = QPixmap::fromImage(baseImage_).scaled(newSize);
  }
  else {
    scaledDimension_ = qMax(newSize.width()/xSquareCount_, 1);
    Q_ASSERT_X(newSize.width() % scaledDimension_ == 0 &&
               newSize.height() % scaledDimension_ == 0, "setImageSize",
               QString("Bad scaled square image size: (" +
                       ::itoqs(newSize.width()) +
                       ", " + ::itoqs(newSize.height()) + ") :: " +
                       ::itoqs(xSquareCount_) + "; " +
                       ::itoqs(baseImage_.size().width()) + "x" +
                       ::itoqs(baseImage_.size().height())).
               toStdString().c_str());
    generateColorSquares(colorSquares_.keys());
  }
  resize(newSize);
  update();
}

void squareImageLabel::setImageWidth(int newWidth) {

  scaledDimension_ = qMax(newWidth/xSquareCount_, 1);
  if (imageIsFlat()) {
    scaledImage_ = QPixmap::fromImage(baseImage_).scaledToWidth(newWidth);
  }
  else {
    Q_ASSERT_X(newWidth % scaledDimension_ == 0, "setImageWidth",
               QString("Bad scaled square image width: " +
                       ::itoqs(newWidth) + " " +
                       ::itoqs(scaledDimension_) + "; " +
                       ::itoqs(baseImage_.size().width()) + "x" +
                       ::itoqs(baseImage_.size().height())).
               toStdString().c_str());
    generateColorSquares(colorSquares_.keys());
  }
  resize(size());
  update();
}

void squareImageLabel::setImageHeight(int newHeight) {

  scaledDimension_ = qMax(newHeight/ySquareCount_, 1);
  if (imageIsFlat()) {
    scaledImage_ = QPixmap::fromImage(baseImage_).scaledToHeight(newHeight);
  }
  else {
    Q_ASSERT_X(newHeight % scaledDimension_ == 0, "setImageHeight",
               QString("Bad scaled square image height: " +
                       ::itoqs(newHeight) + " " +
                       ::itoqs(ySquareCount_) + "; " +
                       ::itoqs(baseImage_.size().width()) + "x" +
                       ::itoqs(baseImage_.size().height())).
               toStdString().c_str());
    generateColorSquares(colorSquares_.keys());
  }
  resize(size());
  update();
}

void squareImageLabel::generateColorSquares(const QList<QRgb>& colors) {

  colorSquares_.clear();
  for (int i = 0, size = colors.size(); i < size; ++i) {
    QPixmap thisSquare(scaledDimension_, scaledDimension_);
    const QRgb thisColor = colors[i];
    thisSquare.fill(thisColor);
    colorSquares_[thisColor] = thisSquare;
  }
}
