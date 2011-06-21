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

#include "versionProcessing.h"

versionProcessor* versionProcessor::processor_ = NULL;

bool CVersion::operator>(const CVersion& otherVersion) const {

  const QStringList thisList = version_.split('.');
  const QStringList otherList = otherVersion.version_.split('.');
  for (int i = 0, size = thisList.size(); i < size; ++i) {
    const int thisVersion = thisList[i].toInt();
    const int otherVersion = otherList[i].toInt();
    if (thisVersion > otherVersion) {
      return true;
    }
    else if (thisVersion == otherVersion) {
      continue;
    }
    else { // thisVersion < otherVersion
      return false;
    }
  }
  return false; // they're equal
}

flossColor
addSquareColorVersion_0_9_2_16::transform(const flossColor& color) const {

  if (color.type() == flossDMC || !::colorIsDmc(color.color())) {
    return color;
  }
  else { // it's a dmc color not marked as DMC
    return flossColor(color.color(), flossDMC);
  }
}

void versionProcessor::setProcessor(const QString& versionString) {

  if (processor_) {
    delete processor_;
    processor_ = NULL;
  }

  const CVersion version(versionString);
  if (version > CVersion("0.9.2.16")) {
    processor_ = new versionProcessor_current;
  }
  else {
    processor_ = new versionProcessor_0_9_2_16;
  }
}
