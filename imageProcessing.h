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

#ifndef IMAGEPROCESSING_H
#define IMAGEPROCESSING_H

#include <QtCore/QHash>
#include <QtCore/QSharedPointer>

#include "colorLists.h"
#include "floss.h"

class grid;
class triC;
class pixel;
class historyPixel;
class pairOfInts;
class QImage;
template<class T> class QVector;
template<class T> class QList;
template<class T1, class T2> class QHash;

typedef unsigned int QRgb;

// colorCount holds a color and the number of times it appears in some
// image.  It "<" compares with other colorCounts by count and "=="
// compares by color.
class colorCount {
 public:
  colorCount() : count_(-1) {}
  colorCount(QRgb color, int count) : color_(color), count_(count) {}
  QRgb color() const { return color_; }
  int count() const { return count_; }
  bool operator<(const colorCount& cc) const { return count_ < cc.count_; }
  bool operator==(const colorCount& cc) const { return color_ == cc.color_; }
 private:
  QRgb color_;
  int count_;
};

class colorTransformer;

// base class for a functor that takes a color and returns a
// transformed version of that color; this base class performs the
// identity transformation
class colorTransformer {
 public:
  virtual ~colorTransformer() {}
  // create the right kind of transformer based on <type>;
  // caller is responsible for destruction of the returned pointer
  static colorTransformerPtr createColorTransformer(flossType type);
  virtual QRgb transform(QRgb input) const = 0;
  virtual triC transform(const triC& input) const = 0;
  virtual QVector<triC> transform(const QVector<triC>& colors) const = 0;
  // there may already be a DMC transformation hash available; use it if
  // you can
  virtual void setDMCHash(const QHash<QRgb, QRgb>& ) {}
};

// return whatever's given unchanged
class variableTransformer : public colorTransformer {
 public:
  QRgb transform(QRgb input) const { return input; }
  triC transform(const triC& input) const { return input; }
  QVector<triC> transform(const QVector<triC>& colors) const {
    return colors;
  }
};

// transform to the "nearest" dmc color
class dmcTransformer : public colorTransformer {
 public:
  QRgb transform(QRgb input) const {
    if (!colorHash_.isEmpty()) {
      return colorHash_[input];
    }
    else {
      return ::rgbToDmc(input).qrgb();
    }
  }
  triC transform(const triC& input) const {
    return ::rgbToDmc(input);
  }
  QVector<triC> transform(const QVector<triC>& colors) const {
    return ::rgbToDmc(colors);
  }
  void setDMCHash(const QHash<QRgb, QRgb>& hash) {
    colorHash_ = hash;
  }
 private:
  QHash<QRgb, QRgb> colorHash_;
};

// transform to the "nearest" anchor color
class anchorTransformer : public colorTransformer {
 public:
  QRgb transform(QRgb input) const {
    return ::rgbToAnchor(input).qrgb();
  }
  triC transform(const triC& input) const {
    return ::rgbToAnchor(input);
  }
  QVector<triC> transform(const QVector<triC>& colors) const {
    return ::rgbToAnchor(colors);
  }
};

inline triC transformColor(const triC& color, flossType type) {
  
  const colorTransformerPtr transformer =
    colorTransformer::createColorTransformer(type);
  return transformer->transform(color);
}

inline triC transformColor(const flossColor& color) {

  return ::transformColor(color.color(), color.type());
}

inline QVector<triC> transformColors(const QVector<triC>& colors, flossType type) {
  
  const colorTransformerPtr transformer =
    colorTransformer::createColorTransformer(type);
  return transformer->transform(colors);
}

// for each pixel in <newImage>, choose a new color from <colors> closest
// to the pixel's color, and replace the old pixel with the new.
// Returns the new colors actually used.
QVector<triC> segment(QImage* newImage, const QVector<triC>& colors,
                      int numImageColors);

// Segment the squared pixels from <newImage> on <squaresList> against
// <sourceImage>.  <sourceImage> provides the "old" color for each
// square on <squaresList> (each square being of dimension <dimension>),
// <colors> provides the new colors to choose from.
void segment(const QImage& sourceImage, QImage* newImage,
             const QList<pixel>& squaresList,
             int dimension, const QVector<triC>& colors);

// square <newImage> into squares of dimension <dimension>, choosing the
// new color for each square by selecting the color most repeated in that
// square (in the case of a tie, choose the color that minimizes distance
// to that color over the entire square).
// Returns the colors of the new image.
QVector<triC> mode(QImage* newImage, int dimension);

// square <newImage> into squares of dimension <dimension>.  The color for
// a given square is chosen by minimizing the distance sum over a color in
// a given square in <newImage> to all pixels in the corresponding square
// in <originalImage>.  Thus the color chosen comes from the square in
// <newImage>.
// Returns the colors of the new image.
QVector<triC> median(grid* newImage, const grid& originalImage,
                     int dimension);

// perform the previous median processing, except choose the <oldColors>
// color for a given pixel if the old color matches better than the
// median processing color.
// Returns the colors of the new image.
QVector<triC> median(QImage* newImage, const QImage& originalImage,
                     const QList<pixel>& squaresList,
                     const QVector<historyPixel>& oldColors, int dimension);

// choose (up to) <numColors> colors that best represent <image>; make
// them all dmc colors if <dmcOut>.
// See .cpp for the meaning of "best represent".
// Returns the chosen colors.
QVector<triC> chooseColors(const QImage& image, int numColors,
                           const QVector<triC>& seedColors,
                           int numImageColors,
                           const colorTransformerPtr& transformer);

// choose (up to) <numColors> colors that best represent the colors in
// the square regions determined by <squaresList> and <dimension> in
// <image>; make them all dmc colors if <dmcOut>.
// See .cpp for the meaning of "best represent".
// Returns the chosen colors.
QVector<triC> chooseColors(const QImage& image,
                           const QList<pixel>& squaresList,
                           int dimension, int numColors,
                           const colorTransformerPtr& transformer);

// use the <colorCountMap>, which gives the number of pixels with a
// given color, to choose (up to) <numColors> colors, all dmc if
// <dmcOut>.  See .cpp for the algorithm.
QVector<triC> chooseColorsFromList(const QHash<QRgb, int>& colorCountMap,
                                   const QVector<QRgb> seedColors,
                                   int numColors,
                                   const colorTransformerPtr& transformer);

// change any old square (of dimension <dimension>) with color <oldColor>
// to <newColor>.
// Returns a list of the squares changed (box coordinates)
QVector<pairOfInts> changeColor(QImage* newImage, QRgb oldColor,
                                QRgb newColor, int dimension);

// change the block in <newImage> to <color> - the block has <dimension>
// and is located at (<x>, <y>), where x and y are or are not <blockCoords>
void changeOneBlock(QImage* newImage, int x, int y, QRgb color,
                    int dimension, bool blockCoords = false);

// perform changeOneBlock for each square in <points>
// T must have x() and y()
template<class T>
void changeBlocks(QImage* newImage, const QVector<T>& points,
                  QRgb newColor, int dimension, bool blockCoords = false);

// perform changeOneBlock for each square in <pixels>, using the pixel's
// color as the new color for each pixel
void changeBlocks(QImage* newImage, const QVector<pixel>& pixels,
                  int dimension, bool blockCoords = false);

// fill in the region including (<x>,<y>) with <newColor>, where each
// square has dimension <dimension>.  The region is determined by moving
// up, down, left, right, but _not_ diagonal.  (<x>, <y> are pixel
// coordinates, not square.)
// returns the square coordinates of the squares filled
QVector<pairOfInts> fillRegion(QImage* newImage, int x, int y,
                               QRgb newColor, int dimension);

// search <image> for <colors> and return those colors _not_ found
QVector<triC> findColors(const grid& image, const QVector<triC>& colors);

#endif
