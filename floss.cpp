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

#include "floss.h"

#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtCore/QDebug>

#include "colorLists.h"
#include "imageProcessing.h"

QString flossType::text() const {

  switch (value_) {
  case flossVariable:
    return QObject::tr("Any colors");
    break;
  case flossDMC:
    return QObject::tr("DMC colors only");
    break;
  case flossAnchor:
    return QObject::tr("Anchor colors only");
    break;
  default:
    qWarning() << "Unhandled floss type text:" << value_;
    return "ERROR";
    break;
  }
}

QString flossType::shortText() const {

  switch (value_) {
  case flossVariable:
    return "";
    break;
  case flossDMC:
    return "DMC";
    break;
  case flossAnchor:
    return "Anchor";
    break;
  default:
    qWarning() << "Unhandled floss type short text:" << value_;
    return "ERROR";
    break;
  }
}

QString flossType::prefix() const {

  switch (value_) {
  case flossVariable:
    return "v";
    break;
  case flossDMC:
    return "d";
    break;
  case flossAnchor:
    return "a";
    break;
  default:
    qWarning() << "Unhandled floss type prefix:" << value_;
    return "v"; // flossVariable
    break;
  }
}

QList<flossType> flossType::flossTypes() {
  
  QList<flossType> types;
  types << flossType(flossVariable) << flossType(flossDMC) <<
    flossType(flossAnchor);
  return types;
}

QList<flossType> flossType::allFlossTypes() {
  
  QList<flossType> types;
  types << flossType(flossDMC) << flossType(flossAnchor) << 
    flossType(flossVariable);
  return types;
}

flossType::flossType(const QString& typePrefix) {

  if (typePrefix == "d") {
    value_ = flossDMC;
  }
  else if (typePrefix == "a") {
    value_ = flossAnchor;
  }
  else {
    value_ = flossVariable;
  }
}

flossType getFlossType(const QVector<flossColor>& colors) {

  const QList<flossType> types = flossType::allFlossTypes();
  for (int i = 0, size = types.size(); i < size; ++i) {
    const flossType thisType = types[i];
    if (colorsAreOfType(colors, thisType)) {
      return thisType;
    }
  }
  return flossVariable;
}

bool colorsAreOfType(const QVector<flossColor>& colors, flossType type) {

  for (int i = 0, size = colors.size(); i < size; ++i) {
    if (colors[i].type() != type) {
      return false;
    }
  }
  return true;
}

bool colorsContainType(const QVector<flossColor>& colors, flossType type) {

  for (int i = 0, size = colors.size(); i < size; ++i) {
    if (colors[i].type() == type) {
      return true;
    }
  }
  return false;
}

bool typedFloss::operator<(const typedFloss& f) const {

  if (f.type() != type()) {
    return type().value() < f.type().value();
  }
  else if (type() != flossVariable) {
    return code() < f.code();
  }
  else { // both are variable
    return color().intensity() < f.color().intensity();
  }
}

QVector<typedFloss> rgbToFloss(const QVector<flossColor>& rgbColors) {

  QVector<typedFloss> returnFloss;
  returnFloss.reserve(rgbColors.size());
  const QVector<floss> dmcFloss = ::initializeDMC();
  const QVector<int> flossCodes = ::rgbToCode(rgbColors);
  for (int i = 0, size = rgbColors.size(); i < size; ++i) {
    const flossColor thisColor = rgbColors[i];
    if (thisColor.type() == flossDMC) {
      const int dmcIndex = dmcFloss.indexOf(floss(thisColor.color()));
      if (dmcIndex != -1) { // (paranoia check)
        const floss thisDmcFloss = dmcFloss[dmcIndex];
        returnFloss.push_back(typedFloss(thisDmcFloss, flossDMC));
      }
      else {
        qWarning() << "DMC color not found in rgbToFloss: " <<
          ::ctos(thisColor.color());
        returnFloss.push_back(typedFloss(-1, "ERROR", thisColor.color(),
                                         thisColor.type()));
      }
    }
    else {
      const triC closestDmcColor = ::transformColor(thisColor.color(),
                                                    flossDMC);
      const int dmcIndex = dmcFloss.indexOf(floss(closestDmcColor));
      if (dmcIndex != -1) { // (paranoia check)
        const floss closestDmcFloss = dmcFloss[dmcIndex];
        const QString dmcApproximation =
          "~" + QString::number(closestDmcFloss.code()) + ":" +
          closestDmcFloss.name();
        returnFloss.push_back(typedFloss(flossCodes[i], dmcApproximation,
                                         thisColor.color(), thisColor.type()));
      }
      else {
        qWarning() << "DMC transform failed in rgbToFloss: " <<
          ::ctos(thisColor.color()) << thisColor.type().value() <<
          ::ctos(closestDmcColor);
        returnFloss.push_back(typedFloss(-1, "ERROR", thisColor.color(),
                                         thisColor.type()));
      }
    }
  }
  return returnFloss;
}

