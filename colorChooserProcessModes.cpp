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

#include "colorChooserProcessModes.h"

#include <QtXml/QDomElement>

#include "colorLists.h"
#include "imageProcessing.h"
#include "utility.h"
#include "xmlUtility.h"


processModeGroup::processModeGroup() {

  // (add in the order they should appear in the widget comboBox)
  activeModes_.push_back(processModePtr(new numberOfColorsToDmcMode()));
  activeModes_.push_back(processModePtr(new numberOfColorsToAnchorMode()));
  activeModes_.push_back(processModePtr(new numberOfColorsMode()));
  activeModes_.push_back(processModePtr(new dmcMode()));
  activeModes_.push_back(processModePtr(new anchorMode()));
}

QList<QStringPair> processModeGroup::modeStrings() const {

  QList<QStringPair> returnList;
  for (int i = 0, size = activeModes_.size(); i < size; ++i) {
    processModePtr thisModePtr = activeModes_[i];
    returnList.push_back(qMakePair(thisModePtr->modeText(),
                                   thisModePtr->toolTip()));
  }
  return returnList;
}

void processModeGroup::setNewMode(const QString& mode) {

  for (int i = 0, size = activeModes_.size(); i < size; ++i) {
    if (activeModes_[i]->modeText() == mode) {
      curMode_ = activeModes_[i];
      break;
    }
  }
}

void processModeGroup::clearColorLists() {

  for (int i = 0, size = activeModes_.size(); i < size; ++i) {
    activeModes_[i]->resetColorList();
  }
}

void processModeGroup::appendColorLists(QDomDocument* doc,
                                        QDomElement* appendee) const {

  for (int i = 0, size = activeModes_.size(); i < size; ++i) {
    activeModes_[i]->appendColorList(doc, appendee);
  }
}

void processModeGroup::setColorLists(const QDomElement& element) {

  QDomNodeList lists = element.elementsByTagName("color_list");
  for (int i = 0, size = lists.size(); i < size; ++i) {
    QDomElement thisElement(lists.item(i).toElement());
    const QString mode(thisElement.attribute("mode"));
    const QVector<triC> thisList =
      ::loadColorListFromText(thisElement.text());
    for (int j = 0, jSize = activeModes_.size(); j < jSize; ++j) {
      if (activeModes_[j]->modeText() == mode) {
        activeModes_[j]->setClickedColorList(thisList);
        break;
      }
    }
  }
}

triState processModeGroup::performProcessing(QImage* image, int numColors,
                                             int numImageColors) {

  return curMode_->performProcessing(image, numColors, numImageColors);
}

QString processModeGroup::toolTip(const QString& modeText) const {

  for (int i = 0, size = activeModes_.size(); i < size; ++i) {
    if (activeModes_[i]->modeText() == modeText) {
      return activeModes_[i]->toolTip();
    }
  }
  return QString();
}

QVector<triC> colorChooserProcessMode::colorList() const {

  // returnColors = clickedColors_ + generatedColors_
  QVector<triC> returnColors;
  returnColors.reserve(clickedColors_.size() + generatedColors_.size());
  for (int i = 0, size = clickedColors_.size(); i < size; ++i) {
    returnColors.push_back(clickedColors_[i]);
  }
  for (int i = 0, size = generatedColors_.size(); i < size; ++i) {
    returnColors.push_back(generatedColors_[i]);
  }
  return returnColors;
}

void colorChooserProcessMode::restoreSavedImage(QImage* originalImage,
                                                const QVector<triC>& colors,
                                                int numImageColors) {

  ::segment(originalImage, colors, numImageColors);
}

void numColorsBaseModes::appendColorList(QDomDocument* doc,
                                         QDomElement* appendee) {

  if (!clickedColorList().isEmpty()) {
    ::appendColorList(doc, clickedColorList(), appendee, "mode", modeText());
  }
}

dmcMode::dmcMode() : fixedListBaseMode(loadDMC()) {}

anchorMode::anchorMode() : fixedListBaseMode(loadAnchor()) {}

triC colorChooserProcessMode::addColor(const triC& color, bool* added) {

  if (!clickedColors_.contains(color)) {
    clickedColors_.push_back(color);
    *added = true;
  }
  else {
    *added = false;
  }
  return color;
}

flossType processModeGroup::flossMode() const {
  return curMode_->flossMode();
}

flossType numberOfColorsMode::flossMode() const {
  return flossVariable;
}

flossType numberOfColorsToDmcMode::flossMode() const {
  return flossDMC;
}

flossType numberOfColorsToAnchorMode::flossMode() const {
  return flossAnchor;
}

flossType dmcMode::flossMode() const {
  return flossDMC;
}

flossType anchorMode::flossMode() const {
  return flossAnchor;
}

triC numberOfColorsToDmcMode::addColor(const triC& color, bool* added) {

  return colorChooserProcessMode::addColor(::rgbToDmc(color), added);
}

triC numberOfColorsToAnchorMode::addColor(const triC& color, bool* added) {

  return colorChooserProcessMode::addColor(::rgbToAnchor(color), added);
}

bool colorChooserProcessMode::removeColor(const triC& color) {

  int indexOfColor = clickedColors_.indexOf(color);
  if (indexOfColor != -1) {
    clickedColors_.remove(indexOfColor);
  }
  return !clickedColors_.empty();
}

fixedListBaseMode::fixedListBaseMode(const QVector<triC>& colors)
  : colorChooserProcessMode(colors) {}

triState fixedListBaseMode::performProcessing(QImage* image, int ,
                                              int numImageColors) {

  QVector<triC> segmentColors = ::segment(image, clickedColorList(),
                                          numImageColors);
  if (!segmentColors.empty()) {
    setGeneratedColorList(segmentColors);
    return triTrue;
  }
  else {
    return triNoop;
  }
}

triState numColorsBaseModes::performProcessing(QImage* image, int numColors,
                                               int numImageColors) {

  colorTransformerPtr transformer =
    colorTransformer::createColorTransformer(flossMode());
  QVector<triC> newColors = ::chooseColors(*image, numColors,
                                           clickedColorList(),
                                           numImageColors,
                                           transformer);
  if (!newColors.empty()) {
    // remove the seed colors from newColors to create generatedColors
    const QVector<triC>& seedColors = clickedColorList();
    QVector<triC> generatedColors = newColors;
    for (int i = 0, size = seedColors.size(); i < size; ++i) {
      generatedColors.remove(generatedColors.indexOf(seedColors[i]));
    }
    setGeneratedColorList(generatedColors);
  }
  else {
    return triNoop;
  }
  if (!::segment(image, newColors, numImageColors).empty()) {
    //return triState(colorList().size() != savedColorsSize);
    return triTrue;
  }
  else {
    return triNoop;
  }
}
