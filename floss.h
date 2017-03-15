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

#ifndef FLOSS_H
#define FLOSS_H

#include <QtCore/QMetaType>

#include "triC.h"

enum flossTypeValue {flossDMC, flossAnchor, flossVariable};
// enums can't be forward declared (yet), so wrap it
class flossType {
 public:
  flossType() : value_(flossVariable) {}
  flossType(flossTypeValue value) : value_(value) {}
  flossType(const QString& typePrefix);
  flossTypeValue value() const { return value_; }
  bool operator==(const flossTypeValue& value) const {
    return value == value_;
  }
  bool operator==(const flossType& otherType) const {
    return otherType.value() == value_;
  }
  bool operator!=(const flossType& otherType) const {
    return otherType.value() != value_;
  }
  // There's a partial order on floss types: a <= b if b contains a's colors.
  bool operator<=(const flossType& otherType) const {
    if (otherType == flossVariable || otherType == value_) {
      return true;
    }
    else {
      return false;
    }
  }
  // Return the text representation of this floss type.
  QString text() const;
  // Return the short text representation of this floss type - returns
  // "" for flossVariable.
  QString shortText() const;
  // Return a one letter representation of this floss type.
  QString prefix() const;
  // Return the types to be allowed for square tool colors, in the order
  // their text should appear in a combo box.
  static QList<flossType> flossTypes();
  // Return _all_ of the floss types, flossVariable last.
  static QList<flossType> allFlossTypes();

 private:
  flossTypeValue value_;
};
// make flossType known to QVariant
Q_DECLARE_METATYPE(flossType)

class flossColor {
 public:
  flossColor() : color_(), type_(flossVariable) {}
  explicit flossColor(const triC& color)
    : color_(color), type_(flossVariable) {}
  flossColor(const triC& color, flossType type)
    : color_(color), type_(type) {}
  flossColor(const triC& color, const QString& typePrefix)
    : color_(color), type_(typePrefix) {}
  flossColor(const flossColor& other)
    : color_(other.color()), type_(other.type()) {}
  triC color() const { return color_; }
  QRgb qrgb() const { return color_.qrgb(); }
  flossType type() const { return type_; }
  QString prefix() const { return type_.prefix(); }
  bool operator==(const flossColor& other) const {
    return color_ == other.color_;
  }
  bool operator==(const triC& other) const {
    return color_ == other;
  }

 private:
  triC color_;
  flossType type_;
};

// Return flossDMC/flossAnchor/... if all colors are the same floss
// type, otherwise return flossVariable.
flossType getFlossType(const QVector<flossColor>& colors);
// Return true if all of the <colors> are of type <type>.
bool colorsAreOfType(const QVector<flossColor>& colors, flossType type);
// Return true if at least one of the <colors> is of type <type>.
bool colorsContainType(const QVector<flossColor>& colors, flossType type);

// a floss consists of a color, a name, and a DMC code (-1 for non-dmc)
// (cross stitch "thread" is called and sold as floss)
class floss {

 public:
  floss() : code_(-1), name_("N/A"), color_() {}
  explicit floss(const triC& color)
    : code_(-1), name_("N/A"), color_(color) {}
  floss(int code, const QString& name, const triC& color)
    : code_(code), name_(name), color_(color) {}
  floss(const floss& otherFloss)
    : code_(otherFloss.code()), name_(otherFloss.name()),
      color_(otherFloss.color()) {}
  bool operator==(const triC& color) const { return color == color_; }
  bool operator==(const floss& otherFloss) const {
    return otherFloss.color_ == color_;
  }
  int code() const { return code_; }
  QString name() const { return name_; }
  triC color() const { return color_; }

 private:
  int code_;
  QString name_;
  triC color_;
};

class typedFloss : public floss {

 public:
  typedFloss() : floss(), type_(flossVariable) {}
  typedFloss(int code, const QString& name, const triC& color,
             flossType type)
    : floss(code, name, color), type_(type) {}
  typedFloss(const floss& untypedFloss, flossType type)
    : floss(untypedFloss), type_(type) {}
  flossType type() const { return type_; }
  bool operator<(const typedFloss& f) const;

 private:
  flossType type_;
};

// functor for comparing the intensity of two typedFlosses
class typedFlossIntensity {
 public:
  bool operator()(const typedFloss& c1, const typedFloss& c2) const {
    return c1.color().intensity() < c2.color().intensity();
  }
};

// Return floss information for the given <rgbColors>, in the same order.
QVector<typedFloss> rgbToFloss(const QVector<flossColor>& rgbColors);
QVector<typedFloss> rgbToFloss(const QVector<triC>& colors, flossType type);
typedFloss rgbToFloss(const flossColor& color);
typedFloss rgbToFloss(const triC& color, flossType type);

// Return floss information for the given <rgbColors>, in the same order.
// If a color isn't DMC, give the floss the name of the closest DMC color
// in the form ~[DMCcode]:[name], and make the code -1
QVector<typedFloss> rgbToVerboseFloss(const QVector<flossColor>& rgbColors);

// Return the DMC or Anchor color code as a fixed width string.
QString codeToString(int code);

#endif
