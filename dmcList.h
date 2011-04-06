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

#ifndef DMCLIST_H
#define DMCLIST_H

#include "triC.h"

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

// return floss data for the DMC colors
QVector<floss> initializeDMC();

// return the DMC colors
QVector<triC> loadDMC();
// load DMC colors with intensities; assign to <intensitySpread> the
// (smallest) distance that guarantess that an arbitrary color will be
// within that distance of a color on the list in intensity
QVector<iColor> loadIDMC(int* intensitySpread);
// return the "gray" dmc colors
QVector<iColor> loadDmcGray(int* intensitySpread);
// return the dmc colors with color component order r >= g >= b;
// assign to <intensitySpread> the (smallest) distance that guarantess
// that for an arbitrary  color with r >= g >= b, the closest color on
// the list will be within <intensitySpread> of the arbitrary color in
// intensity
QVector<iColor> loadDmcRGB(int* intensitySpread);
QVector<iColor> loadDmcRBG(int* intensitySpread);
QVector<iColor> loadDmcGRB(int* intensitySpread);
QVector<iColor> loadDmcGBR(int* intensitySpread);
QVector<iColor> loadDmcBRG(int* intensitySpread);
QVector<iColor> loadDmcBGR(int* intensitySpread);

// return true if the color(s) is(are) DMC
bool colorIsDmc(const triC& color);
bool colorsAreDmc(const QVector<triC>& colors);

// first looks for a "good" match to a color on <colorList> with the same
// rgb value order as <color>; if that fails then it just returns the
// closest color to <color> on <colorList>
QRgb closestMatch(const triC& color, const QList<QRgb>& colorList);

// return the color in <colorList> that is (Euclidean) closest to <color>
// <colorList> MUST be non-empty and sorted by increasing intensity
// and the intensity interval
// [color.intensity() - intensitySpread, color.intensity() + intensitySpread]
// BETTER contain an element of colorList (or you'll get nonsense)
// <intensitySpread> should have the property that for an arbitary color,
// the color on <colorList> at min distance from the arbitrary color
// should be within <intensitySpread> intensity distance from the arbitrary
// color
triC closestMatch(const triC& color, const QVector<iColor>& colorList,
                  int intensitySpread);
// return the DMC color closest to <color>
triC rgbToDmc(const triC& color);
// for each color, determine the DMC color closest to it.  Return those
// colors in the same order as <colors>, keeping only the first of a
// given DMC color if <noDups>
QVector<triC> rgbToDmc(const QVector<triC>& colors, bool noDups = false);
// return floss information for the given <colors>, in the same order
// if a color isn't DMC, give the floss the name of the closest DMC color
// in the form ~[DMCcode]:[name], and make the code -1
QVector<floss> rgbToFloss(const QVector<triC>& colors);

#endif
