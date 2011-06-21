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

#ifndef TRIC_H
#define TRIC_H

#include <QtCore/QString>

#include <QtGui/QColor>

// triC is a trimmer inlined unchecked version of QColor
// (storing the color coordinates as a QRgb seems to be faster on dmc processing
// but slower on median processing)
class triC {
 public:
  triC() : c1_(0), c2_(0), c3_(0), valid_(false) {}
  triC(int x, int y, int z) : c1_(x), c2_(y), c3_(z), valid_(true) {}
  triC(const triC& color) : c1_(color.c1_), c2_(color.c2_),
    c3_(color.c3_), valid_(color.valid_) {}
  triC(const QRgb& color)
    : c1_(qRed(color)), c2_(qGreen(color)), c3_(qBlue(color)),
    valid_(true) {}
  triC(const QColor& color)
    : c1_(color.red()), c2_(color.green()), c3_(color.blue()),
    valid_(color.isValid()) {}
  int intensity() const { return r() + g() + b(); }
  // opposite returns the opposite color by using the opposite hue
  // and full saturation and value (as an hsv color)
  triC opposite() const {
    QColor c(c1_, c2_, c3_);
    return QColor::fromHsv( (c.hue() + 180)%360, 255, 255);
  }
  bool isValid() const { return valid_ == 1; }
  QRgb qrgb() const { return qRgb(c1_, c2_, c3_); }
  QColor qc() const { return QColor(c1_, c2_, c3_); }
  triC& operator=(const triC& color) {
    c1_ = color.c1_;
    c2_ = color.c2_;
    c3_ = color.c3_;
    valid_ = color.valid_;
    return *this;
  }
  bool operator<(const triC& color) const {
    return (c1_ < color.c1_) ||
      (c1_ == color.c1_ && c2_ < color.c2_) ||
      (c1_ == color.c1_ && c2_ == color.c2_ && c3_ < color.c3_);
  }
  bool operator==(const triC& color) const {
    return c1_ == color.c1_ && c2_ == color.c2_ && c3_ == color.c3_;
  }
  bool operator!=(const triC& color) const {
    return c1_ != color.c1_ || c2_ != color.c2_ || c3_ != color.c3_;
  }
  int rc1() const { return c1_; }
  int rc2() const { return c2_; }
  int rc3() const { return c3_; }
  int r() const { return c1_; }
  int g() const { return c2_; }
  int b() const { return c3_; }

 private:
  quint8 c1_;
  quint8 c2_;
  quint8 c3_;
  quint8 valid_; // 1 for valid, 0 for invalid
};

// "distance squared"
inline int ds(const triC& c1, const triC& c2) {
  return
    qAbs(c1.r() - c2.r()) + qAbs(c1.g() - c2.g()) + qAbs(c1.b() - c2.b());
}

// functor for comparing the intensity of two triCs
class triCIntensity {
 public:
  bool operator()(const triC& c1, const triC& c2) const {
    return c1.intensity() < c2.intensity();
  }
};

// "color to string"
QString ctos(const triC& color);

#endif
