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

#include "imageUtility.h"

#include <QtCore/QDebug>
#include <QtCore/qmath.h>
#include <QtWidgets/QWidget>
#include <QPainter>
#include <QPen>

extern const int D_MAX;

// color to string: return the rgb values of <color> as a string
// with fixed width for each component
QString ctos(const triC& color) {

  char buff[12];
  sprintf(buff, "%3d %3d %3d", color.r(), color.g(), color.b());
  return buff;
}

void showAndRaise(QWidget* widget) {

  if (widget) {
    // show won't deiconify a window (sigh)
    if (widget->windowState() & Qt::WindowMinimized) {
      widget->setWindowState(widget->windowState() & ~Qt::WindowMinimized);
    }
    widget->show();
    widget->raise();
    widget->activateWindow();
  }
  else {
    qWarning() << "Null pointer in showAndRaise.";
  }
}

void gridImage(QImage* image, int originalSquareDim,
               int originalWidth, int originalHeight,
               QRgb gridColor, qreal gridLineWidth) {

  const int newWidth = image->width();
  const int newHeight = image->height();
  // scaled grid dims
  const qreal xdim = originalSquareDim*newWidth/static_cast<qreal>(originalWidth);
  const qreal ydim = originalSquareDim*newHeight/static_cast<qreal>(originalHeight);
  if (xdim < 1.25 * gridLineWidth || ydim < 1.25 * gridLineWidth) {
    // grid lines will take up nearly all of each square, so refuse to grid
    return;
  }

  QPainter painter(image);
  painter.setPen(QPen(QColor(gridColor), gridLineWidth));
  for (qreal i = 0; i < newWidth; i += xdim) {
    const int iInt = static_cast<int>(i);
    painter.drawLine(QPoint(iInt, 0), QPoint(iInt, newHeight));
  }
  for (qreal j = 0; j < newHeight; j += ydim) {
    const int jInt = static_cast<int>(j);
    painter.drawLine(QPoint(0, jInt), QPoint(newWidth, jInt));
  }
  // grid lines are drawn on the left/top sides of squares, so we have
  // to do the final right/bottom ones by hand
  painter.drawLine(QPoint(newWidth-1, 0), QPoint(newWidth-1, newHeight));
  painter.drawLine(QPoint(0, newHeight-1), QPoint(newWidth, newHeight-1));
}

int computeGridForImageFit(const QSize& imageSize,
                           const QSize& availableSize,
                           int originalSquareSize) {

  // try scaling to width first
  qreal newWidth = availableSize.width();
  qreal newHeight = imageSize.height() * newWidth/imageSize.width();
  if (newHeight <= availableSize.height()) { // scale to width
    const qreal newSquareDim =
      originalSquareSize * newWidth / static_cast<qreal>(imageSize.width());
    return qFloor(newSquareDim);
  }
  else { // scale to height
    const qreal newSquareDim = originalSquareSize *
      availableSize.height() / static_cast<qreal>(imageSize.height());
    return qFloor(newSquareDim);
  }
}

int numberOfColors(const QImage& image) {

  const int width = image.width();
  const int height = image.height();
  // a hash is faster than a set here
  QHash<QRgb, char> usedColors;
  // this is about the smallest estimate of any of the actual values
  // I computed, so it should be an underestimate, but will still
  // help
  usedColors.reserve(qMin(static_cast<int>(width*height*.01), 500000));
  const char c(' ');
  // QSet<QRgb> usedColors;
  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      // don't do look ahead - it's slower on photos
      usedColors[image.pixel(i, j)] = c;
    }
  }
  return usedColors.keys().size();
}

int computeMaxZoomWidth(const QSize scrollSize, const QSize imageSize,
                        int scrollBarDimension) {

  int newWidth = scrollSize.width();
  const int newHeight =
    qCeil(imageSize.height()
             * static_cast<qreal>(scrollSize.width())/imageSize.width());
  if (newHeight > scrollSize.height()) {
    newWidth -= scrollBarDimension;
    // check if we're in the sour spot where making the image full width
    // would lead to a vertical scroll bar, but keeping it at this size
    // won't utilize the region available
    const int newNewHeight =
      qCeil(imageSize.height()
            * static_cast<qreal>(newWidth)/imageSize.width());
    if (newNewHeight < scrollSize.height()) {
      // funny - the best we can do is make it full _height_!
      newWidth = -1;
    }
  }
  return newWidth;
}

int computeMaxZoomHeight(QSize scrollSize, QSize imageSize,
                         int scrollBarDimension) {

  scrollSize.transpose();
  imageSize.transpose();
  return computeMaxZoomWidth(scrollSize, imageSize, scrollBarDimension);
}

void colorCounts(const QImage& image, int squareSize,
                 QHash<QRgb, int>* countHash) {

  QHash<QRgb, int>& hashRef = *countHash; // for notational convenience
  const int w = image.width();
  const int h = image.height();
  for (int j = 0; j < h; j += squareSize) {
    for (int i = 0; i < w; i += squareSize) {
      const QRgb gij = image.pixel(i, j);
      // look ahead
      int dup = squareSize;
      while (i+dup < w && image.pixel(i+dup, j) == gij) {
        dup += squareSize;
      }
      hashRef[gij] += dup/squareSize;
      i += dup - squareSize;
    }
  }
}

bool definiteIntensityCompare(const triC& c1, const triC& c2) {

  const int c1Intensity = c1.intensity();
  const int c2Intensity = c2.intensity();

  if (c1Intensity != c2Intensity) {
    return c1Intensity < c2Intensity;
  }
  else {
    // return whichever is first in the dictionary order
    if (c1.r() != c2.r()) {
      return c1.r() < c2.r();
    }
    else if (c1.g() != c2.g()) {
      return c1.g() < c2.g();
    }
    else {
      return c1.r() < c2.r();
    }
  }
}
