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

#ifndef GRID_H
#define GRID_H

#include <QtCore/QVector>
#include <QtCore/QDebug>

#include <QtGui/QImage>

#include "triC.h"

// An image class that holds triC pixels, making it faster than
// QImage for operations involving individual rgb accesses
// (like distances).
// For best performance, users should iterate over rows.
class grid {

 public:
 grid() : width_(0), height_(0), g_(0) {}
 grid(int width, int height) : width_(width), height_(height) {
    ////!! mingw on Windows requires an extra dll to include
    ////!! exception handling, so now we die on out of memory
    //    try {
      // QImage seems to be layed out this way, so we'll follow along
      g_ = QVector<QVector<triC> >(height, QVector<triC>(width));
//    }
//    catch(const std::bad_alloc& ) {
//      qWarning() << "memory allocation failure in grid" << width << height;
//      width_ = 0;
//      height_ = 0;
//      g_ = QVector<QVector<triC> >(0);
//    }
  }
  explicit grid(const QImage& image)
    : width_(image.width()), height_(image.height()),
    g_(QVector<QVector<triC> >(height_, QVector<triC>(width_))) {

    for (int j = 0; j < height_; ++j) {
      for (int i = 0; i < width_; ++i) {
        operator()(i, j) = image.pixel(i, j);
      }
    }
  }
  grid(const QImage& image, int squareDimension)
    : width_(image.width()), height_(image.height()),
    g_(QVector<QVector<triC> >(height_, QVector<triC>(width_))) {

    const int boxWidth = width_/squareDimension;
    const int boxHeight = height_/squareDimension;
    for (int yBox = 0; yBox < boxHeight; ++yBox) {
      for (int xBox = 0; xBox < boxWidth; ++xBox) {
        int thisX = xBox * squareDimension;
        const int thisXStart = thisX;
        const int thisXStop = thisX + squareDimension;
        int thisY = yBox * squareDimension;
        const int thisYStop = thisY + squareDimension;
        const QRgb thisSquareColor = image.pixel(thisX, thisY);
        while (thisY < thisYStop) {
          thisX = thisXStart;
          while (thisX < thisXStop) {
            operator()(thisX, thisY) = thisSquareColor;
            ++thisX;
          }
          ++thisY;
        }
      }
    }
  }
  QImage toImage() const {
    QImage returnImage(width_, height_, QImage::Format_RGB32);
    if (returnImage.isNull()) { // ran out of memory
      qWarning() << "Empty image in grid to QImage.";
      return QImage();
    }
    for (int j = 0; j < height_; ++j) {
      for (int i = 0; i < width_; ++i) {
        returnImage.setPixel(i, j, operator()(i, j).qrgb());
      }
    }
    return returnImage;
  }
  bool empty() const { return width_ == 0; }
  int width() const { return width_; }
  int height() const { return height_; }
  const triC& operator()(int i, int j) const { return g_[j][i]; }
  triC& operator()(int i, int j) { return g_[j][i]; }

 private:
  int width_;
  int height_;
  QVector< QVector<triC> > g_;
};

#endif
