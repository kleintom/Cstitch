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

#ifndef COLORLISTS_H
#define COLORLISTS_H

#include <QtCore/QVector>

#include "triC.h"
#include "floss.h"

class colorTransformer;
template<class T> class QSharedPointer;
typedef QSharedPointer<colorTransformer> colorTransformerPtr;

const int GRAY_DIFF = 12;
enum colorOrder {O_GRAY, O_RGB, O_RBG, O_GRB, O_GBR, O_BGR, O_BRG};

// a color with a saved intensity (triC computes but doesn't save its
// intensity)
// [Could have derived from triC, but don't want to make triC::intensity
// virtual or shadow it]
class iColor {

 public:
  iColor() : color_(), intensity_(-1) {}
  explicit iColor(const triC& c) : color_(c), intensity_(c.intensity()) {}
  triC color() const { return color_; }
  int intensity() const { return intensity_; }
  bool operator<(const iColor& other) const {
    return intensity_ < other.intensity_;
  }
 private:
  triC color_;
  int intensity_;
};

class orderComparator {
public:
  virtual ~orderComparator() {}
  virtual bool operator()(const triC& ) const = 0;
};

class grayComparator : public orderComparator {
public:
  bool operator()(const triC& color) const {
    return (qAbs(color.r() - color.g()) < GRAY_DIFF &&
            qAbs(color.r() - color.b()) < GRAY_DIFF &&
            qAbs(color.g() - color.b()) < GRAY_DIFF);
  }
};

class rgbComparator : public orderComparator {
public:
  bool operator()(const triC& color) const {
    return (color.r() >= color.g() && color.g() >= color.b());
  }
};

class rbgComparator : public orderComparator {
public:
  bool operator()(const triC& color) const {
    return (color.r() >= color.b() && color.b() >= color.g());
  }
};

class grbComparator : public orderComparator {
public:
  bool operator()(const triC& color) const {
    return (color.g() >= color.r() && color.r() >= color.b());
  }
};

class gbrComparator : public orderComparator {
public:
  bool operator()(const triC& color) const {
    return (color.g() >= color.b() && color.b() >= color.r());
  }
};

class brgComparator : public orderComparator {
public:
  bool operator()(const triC& color) const {
    return (color.b() >= color.r() && color.r() >= color.g());
  }
};

class bgrComparator : public orderComparator {
public:
  bool operator()(const triC& color) const {
    return (color.b() >= color.g() && color.g() >= color.r());
  }
};

// colorMatcher's job is to find the closest match to a given color on either
// the dmc or anchor color list.
////
// Implementation notes: colorMatcher only searches for matches amongst colors
// on the color list with the same order type as color_ (dmcColorsHash_ and
// anchorColorsHash_ track those sublists).  Things are further sped up by
// precomputing *IntensitySpreads_, which, for a given color order, compute the
// max distance from color_.intensity() you need to look on a dmc or anchor
// color list for a closest distance-squared match (i.e. if c_match is the
// closest distance_squared color on the color list to color_, then
// |c_match.intensity() - color_.intensity()| < intensity_spread_value). Those
// values allow us to quickly narrow down our search by binary searching on
// intensity rather than on the slower and non-linearly ordered distance
// squared.
class colorMatcher {
 public:
  colorMatcher(flossType type, const triC& color);
  triC closestMatch() const;
  static void resetDataSources();
 private:
  void loadColorList(const orderComparator* comparator,
                     const QVector<triC>& colors,
                     QVector<iColor>* newList) const;
 private:
  const triC color_;
  // The list of colors on the dmc or anchor list matching the color order of
  // color_.
  QVector<iColor> colorList_;
  // The intensity spread value to be used with color_.
  int intensitySpread_;
  // Key is a color order, value is all dmc colors having that color order.
  static QHash<colorOrder, QVector<iColor> > dmcColorsHash_;
  // Key is a color order, value is an int <d> with the property that an
  // arbitrary color with the given color order is, with respect to intensity,
  // within <d> of some dmc color with the same color order.
  static QHash<colorOrder, int> dmcIntensitySpreads_;
  static QHash<colorOrder, QVector<iColor> > anchorColorsHash_;
  static QHash<colorOrder, int> anchorIntensitySpreads_;
};

inline colorOrder getColorOrder(const triC& color) {

  // is c "gray"?
  if (qAbs(color.r() - color.g()) < GRAY_DIFF &&
      qAbs(color.g() - color.b()) < GRAY_DIFF &&
      qAbs(color.r() - color.b()) < GRAY_DIFF) {
    return O_GRAY;
  }
  else { // not "gray"
    // compute the order type of color
    const int r = color.r();
    const int g = color.g();
    const int b = color.b();
    colorOrder order = O_RGB; // for starters
    // Note: if two coordinates are equal, the first will always be
    // be considered larger, which can lead to a suboptimal choice
    // later on
    if (g > r) {
      order = O_GRB;
      if (b > g) {
        order = O_BGR;
      }
      else if (b > r) {
        order = O_GBR;
      }
    }
    else {
      if (b > r) {
        order = O_BRG;
      }
      else if (b > g) {
        order = O_RBG;
      }
    }
    return order;
  }
}

QVector<floss> initializeDMC();
QVector<floss> initializeAnchor();

// return the DMC colors
QVector<triC> loadDMC();
QVector<triC> loadAnchor();

// return true if the color(s) is(are) DMC
bool colorIsDmc(const triC& color);
bool colorsAreDmc(const QVector<triC>& colors);

// Return the color in <colorList> that is (Euclidean) closest to <color>.
QRgb closestMatch(const triC& color, const QList<QRgb>& colorList);

// return the DMC color closest to <color>
triC rgbToDmc(const triC& color);
triC rgbToAnchor(const triC& color);
// for each color, determine the DMC color closest to it.  Return those
// colors in the same order as <colors>, keeping only the first of a
// given DMC color if <noDups>.
QVector<triC> rgbToDmc(const QVector<triC>& colors, bool noDups = false);
QVector<triC> rgbToAnchor(const QVector<triC>& colors, bool noDups = false);
// general implementation for rgbToDmc/rgbToAnchor/etc.
QVector<triC> rgbToColorList(const QVector<triC>& rgbColors,
                             const colorTransformerPtr& transformer,
                             bool noDups = false);

// Return the floss code for each flossColor in <colors> in the same order.
QVector<int> rgbToCode(const QVector<flossColor>& colors);

#endif
