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

#ifndef PATTERNIMAGELABEL_H
#define PATTERNIMAGELABEL_H

#include <QtCore/QHash>

#include <QtGui/QWidget>

//
// Display a square or a pattern image.  Since pattern images have to be
// really large in order to read the symbols, we can't pass around entire
// images, so instead pattern and square images are set by providing an
// initial square image and then reconstituting (zoomed) square and
// pattern images using a color->square and color->symbol hash
// respectively - the user provides the hashes instead of actual images.
//
class patternImageLabel : public QWidget {

  Q_OBJECT

 public:
  explicit patternImageLabel(QWidget* parent) : QWidget(parent),
    squareDim_(0), viewingSquareImage_(false), width_(0), height_(0),
    patternDim_(0), gridOn_(false), gridColor_(Qt::black) {}
  // set the square dimension to use for the pattern image and update
  // the widget to show the new image
  void setPatternDim(int patternDim) {
    patternDim_ = patternDim;
    setWidthAndHeight();
    update();
  }
  // set the base square <image> with <squareDim>, and use <patternDim>
  // as the square size for viewing - redraws the current image
  void setImage(const QImage& image, int squareDim, int patternDim) {
    squareImage_ = image;
    squareDim_ = squareDim;
    patternDim_ = patternDim;
    setWidthAndHeight();
    update();
  }
  // display the square image if <b>, otherwise display the pattern image
  void viewSquareImage(bool b) { viewingSquareImage_ = b; update(); }
  void setSquares(const QHash<QRgb, QPixmap>& squares) {
    squares_ = squares;
  }
  void setSymbols(const QHash<QRgb, QPixmap>& symbols) {
    symbols_ = symbols;
  }
  QRgb gridColor() const { return gridColor_; }
  void setGridColor(QRgb color) { gridColor_ = color; update(); }
  void setGridOn(bool b) { gridOn_ = b; update(); }
  bool gridOn() const { return gridOn_; }
  // return the width of the image being viewed
  int width() const { return width_; }
  // return the height of the image being viewed
  int height() const { return height_; }

 private:
  // determine and store the width and height of the current image
  void setWidthAndHeight() {
    width_ = (squareImage_.width()/squareDim_)*patternDim_;
    height_ = (squareImage_.height()/squareDim_)*patternDim_;
    resize(width_, height_);
  }
  QSize sizeHint() const;
  // draws the current image directly on the widget
  void paintEvent(QPaintEvent* event);
  void mousePressEvent(QMouseEvent* event);

 signals:
  void announceImageClick(QMouseEvent* event);

 private:
  QImage squareImage_;
  int squareDim_; // always the square size of squareImage_
  bool viewingSquareImage_;
  int width_;
  int height_;
  int patternDim_; // changes depending on zoom level
  bool gridOn_;
  QRgb gridColor_;
  QHash<QRgb, QPixmap> squares_;
  QHash<QRgb, QPixmap> symbols_;
};

#endif
