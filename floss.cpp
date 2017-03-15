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

QString codeToString(int code) {

  if (code < 0) {
    return QString("    ");
  }

  char buff[4];
  sprintf(buff, "%4d", code);
  return buff;
}

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

typedFloss rgbToFloss(const flossColor& color) {

  // Keep this in sync with rgbToFloss(const QVector<flossColor>& colors).
  const flossTypeValue type = color.type().value();
  if (type == flossDMC) {
    const QVector<floss> dmcColors = ::initializeDMC();
    const int index = dmcColors.indexOf(floss(color.color()));
    if (index != -1) {
      const floss thisDmcFloss = dmcColors[index];
      return typedFloss(thisDmcFloss, flossDMC);
    }
  }
  else if (type == flossAnchor) {
    const QVector<floss> anchorColors = ::initializeAnchor();
    const int index = anchorColors.indexOf(floss(color.color()));
    if (index != -1) {
      const floss thisAnchorFloss = anchorColors[index];
      return typedFloss(thisAnchorFloss, flossAnchor);
    }
  }

  return typedFloss(floss(color.color()), flossVariable);
}

typedFloss rgbToFloss(const triC& color, flossType type) {

  return ::rgbToFloss(flossColor(color, type));
}

QVector<typedFloss> rgbToFloss(const QVector<flossColor>& colors) {

  // Keep this in sync with rgbToFloss(const flossColor& color) - we're keeping
  // this version separate so that we don't need to load the DMC/Anchor color
  // list for each color on this list.
  // Also, WARNING: there was a bug in an old version of cstitch which in
  // certain cases allowed a non-floss color to be labeled as floss, so now
  // forevermore we need to handle that case just for any projects saved with
  // the bug. :(
  QVector<typedFloss> returnFloss;
  returnFloss.reserve(colors.size());
  QVector<floss> dmcColors;
  QVector<floss> anchorColors;
  for (int i = 0, size = colors.size(); i < size; ++i) {
    const flossColor color = colors[i];
    switch (color.type().value()) {
      case flossDMC: {
        if (dmcColors.isEmpty()) {
          dmcColors = ::initializeDMC();
        }
        const int index = dmcColors.indexOf(floss(color.color()));
        if (index != -1) {
          const floss thisDmcFloss = dmcColors[index];
          returnFloss.push_back(typedFloss(thisDmcFloss, flossDMC));
        }
        else {
          returnFloss.push_back(typedFloss(floss(color.color()), flossVariable));
        }
        break;
      }
      case flossAnchor: {
        if (anchorColors.isEmpty()) {
          anchorColors = ::initializeAnchor();
        }
        const int index = anchorColors.indexOf(floss(color.color()));
        if (index != -1) {
          const floss thisAnchorFloss = anchorColors[index];
          returnFloss.push_back(typedFloss(thisAnchorFloss, flossAnchor));
        }
        else {
          returnFloss.push_back(typedFloss(floss(color.color()), flossVariable));
        }
        break;
      }
      case flossVariable:
        returnFloss.push_back(typedFloss(floss(color.color()), flossVariable));
        break;
    }
  }
  return returnFloss;
}

QVector<typedFloss> rgbToFloss(const QVector<triC>& colors, flossType type) {

  // We could just add the type to each color and then call the flossColor
  // version of this function, but I expect this code to be stable, so in this
  // case I think it's better to special case in order to avoid the memory/time
  // hit.
  QVector<typedFloss> returnFloss;
  returnFloss.reserve(colors.size());
  if (type == flossDMC || type == flossAnchor) {
    const QVector<floss> sourceFloss =
      type == flossDMC ? ::initializeDMC() : ::initializeAnchor();
    for (int i = 0, size = colors.size(); i < size; ++i) {
      const triC color = colors[i];
      const int index = sourceFloss.indexOf(floss(color));
      if (index != -1) {
        returnFloss.push_back(typedFloss(sourceFloss[index], type));
      }
      else {
        returnFloss.push_back(typedFloss(floss(color), flossVariable));
      }
    }
  }
  else {
    for (int i = 0, size = colors.size(); i < size; ++i) {
      returnFloss.push_back(typedFloss(floss(colors[i]), flossVariable));
    }
  }

  return returnFloss;
}

QVector<typedFloss> rgbToVerboseFloss(const QVector<flossColor>& rgbColors) {

  QVector<typedFloss> returnFloss;
  returnFloss.reserve(rgbColors.size());
  const QVector<int> flossCodes = ::rgbToCode(rgbColors);
  const QVector<floss> dmcFloss = ::initializeDMC();
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

