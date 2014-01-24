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

#ifndef IMAGEUTILITY_H
#define IMAGEUTILITY_H

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QHash>
#include <QColor>
#include <QPixmap>
#include <QtWidgets/QStyle>

#include "triC.h"
#include "floss.h"

class grid;
class QImage;

class pairOfInts {
 public:
  pairOfInts() : x_(0), y_(0) {}
  pairOfInts(const int& n1, const int& n2) : x_(n1), y_(n2) { }
  bool operator<(const pairOfInts& otherPair) const {
    return x_ < otherPair.x_ || (x_ == otherPair.x_ && y_ < otherPair.y_);
  }
  bool operator==(const pairOfInts& otherPair) const {
    return x_ == otherPair.x_ && y_ == otherPair.y_;
  }
  int x() const { return x_; }
  int y() const { return y_; }
 private:
  int x_;
  int y_;
};

// an image pixel: coordinates and color
// NOTE: == and < compare only on coordinates, so you shouldn't rely on these
// if your collection of pixels can have different pixels with the same
// coordinates
class pixel {
 public:
  pixel() : coordinates_(), color_() {}
  explicit pixel(const pairOfInts& coordinates) : coordinates_(coordinates),
    color_() {}
  pixel(QRgb color, const pairOfInts& coordinates = pairOfInts(0, 0))
   : coordinates_(coordinates), color_(color) {}
  void setColor(QRgb color) { color_ = color; }
  int x() const { return coordinates_.x(); }
  int y() const { return coordinates_.y(); }
  QRgb color() const { return color_; }
  pairOfInts coordinates() const { return coordinates_; }
  pixel& operator=(const pairOfInts& coordinates) {
    coordinates_ = coordinates;
    return *this;
  }
  // NOTE: == only compares on coordinates
  bool operator==(const pixel& other) const {
    return (coordinates_ == other.coordinates_);
  }
  // NOTE: < only compares on coordinates
  bool operator<(const pixel& other) const {
    return coordinates_ < other.coordinates_;
  }
 private:
  pairOfInts coordinates_;
  QRgb color_;
};

// records a pixel change: coordinates plus old color and new color, and
// whether or not the new color existed in the image before this pixel
class historyPixel {
 public:
  historyPixel() : newColor_(), pixel_(), newColorIsNew_(false) {}
  explicit historyPixel(const pixel& pix) : newColor_(), pixel_(pix),
    newColorIsNew_(false) {}
  historyPixel(const pairOfInts& coords, QRgb oldColor, QRgb newColor,
               bool newColorIsNew) : newColor_(newColor),
    pixel_(oldColor, coords), newColorIsNew_(newColorIsNew) {}
  triC oldColor() const { return pixel_.color(); }
  triC newColor() const { return newColor_; }
  void setNewColor(QRgb color) { newColor_ = color; }
  void setNewColorIsNew(bool b) { newColorIsNew_ = b; }
  bool newColorIsNew() const { return newColorIsNew_; }
  int x() const { return pixel_.x(); }
  int y() const { return pixel_.y(); }
 private:
  QRgb newColor_;
  pixel pixel_; // contains the old color
  bool newColorIsNew_; // i.e. newColor_ didn't exist before this
};

// <oldColor> changed to <newColor> at <coordinates>
class colorChange {

 public:
  colorChange(QRgb oldColor, QRgb newColor,
                  const QVector<pairOfInts>& coordinates)
    : oldColor_(oldColor), newColor_(newColor), coordinates_(coordinates)
  {}
  QRgb oldColor() const { return oldColor_; }
  QRgb newColor() const { return newColor_; }
  QVector<pairOfInts> coordinates() const { return coordinates_; }
 private:
  QRgb oldColor_;
  QRgb newColor_;
  QVector<pairOfInts> coordinates_;
};


// description of a pattern symbol and its border
class patternSymbolIndex {
 public:
  patternSymbolIndex() : symbolIndex_(-1), symbolBorderWidth_(-1),
    symbolDimension_(-1) {}
  patternSymbolIndex(const QPixmap& symbol, int index, int borderWidth,
                    int symbolDimension)
   : symbol_(symbol), symbolIndex_(index), symbolBorderWidth_(borderWidth),
    symbolDimension_(symbolDimension) {}
  const QPixmap& symbol() const { return symbol_; }
  int index() const { return symbolIndex_; }
  int borderWidth() const { return symbolBorderWidth_; }
  int symbolDimension() const { return symbolDimension_; }
 private:
  QPixmap symbol_;
  int symbolIndex_;
  int symbolBorderWidth_; // just the width of the border
  int symbolDimension_; // the total symbol icon dimension
};

inline QString colorToPrettyString(const triC& color) {
  return "(" + QString::number(color.r()) + ", " +
    QString::number(color.g()) + ", " +
    QString::number(color.b()) + ")";
}

// given an unscaled <image> and scaled coordinates for the scaled image,
// return the color at the scaled coordinates
inline QRgb colorFromScaledImageCoords(int scaledX, int scaledY,
                                       int scaledWidth, int scaledHeight,
                                       const QImage& image) {

  int imageX = (scaledX * image.width())/scaledWidth;
  int imageY = (scaledY * image.height())/scaledHeight;
  return image.pixel(imageX, imageY);
}

// Return whether c1.intensity() < c2.intensity(), or if they have the same
// intensity, compare them in some other definite way (for use in sort
// procedures)
bool definiteIntensityCompare(const triC& c1, const triC& c2);

// return the number of distinct colors in the image
int numberOfColors(const QImage& image);

// display <widget> at the top level
void showAndRaise(QWidget* widget);

// grid <image> using <gridColor>; <image> may be scaled, so use
// <original> parameters to determine the proper grid spacing for the
// scaled gridding
void gridImage(QImage* image, int originalSquareDim,
               int originalWidth, int originalHeight, QRgb gridColor);

// return the largest square size that would allow an image with
// original size <imageSize> and original square size <originalSquareSize>
// to fit into a region of size <availableSize>
int computeGridForImageFit(const QSize& imageSize,
                           const QSize& availableSize,
                           int originalSquareSize);

// construct the hash with keys the colors in <image> and values the
// number of squares of a given color, where <image> has square
// size <squareSize>
void colorCounts(const QImage& image, int squareSize,
                 QHash<QRgb, int>* countHash);

inline int scrollbarWidth(QStyle* appStyle = NULL) {

  static int width = 0;
  if (appStyle) {
    width = appStyle->pixelMetric(QStyle::PM_ScrollBarExtent);
  }
  return width;
}

// compute the maximum scaled width of an image of <imageSize> (preserving
// aspect) that will fit in a scroll area of <scrollSize>, taking into
// account a horizontal scroll bar of height <scrollBarDimension> if
// necessary
int computeMaxZoomWidth(const QSize scrollSize, const QSize imageSize,
                        int scrollBarDimension);

int computeMaxZoomHeight(QSize scrollSize, QSize imageSize,
                         int scrollBarDimension);

inline uint qHash(const pairOfInts& key) {
  return qHash(key.x()) ^ key.y();
}
inline uint qHash(const triC& key) {
  return qHash(key.qrgb());
}
inline uint qHash(const pixel& key) {
  return qHash(key.coordinates());
}
inline uint qHash(const flossColor& key) {
  return qHash(key.qrgb());
}

#endif
