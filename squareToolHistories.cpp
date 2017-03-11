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

#include "squareToolHistories.h"

#include <QtCore/QDebug>

#include "squareImageContainer.h"
#include "xmlUtility.h"
#include "imageUtility.h"
#include "imageProcessing.h"

historyItemPtr historyItem::xmlToHistoryItem(const QDomElement& xml) {

  const QString tool(::getElementText(xml, "tool"));
  historyItem* item = NULL;
  if (tool == "change all") {
    item = new changeAllHistoryItem(xml);
  }
  else if (tool == "change one") {
    item = new changeOneHistoryItem(xml);
  }
  else if (tool == "fill region") {
    item = new fillRegionHistoryItem(xml);
  }
  else if (tool == "detail") {
    item = new detailHistoryItem(xml);
  }
  else if (tool == "rare colors") {
    item = new rareColorsHistoryItem(xml);
  }
  else {
    qWarning() << "Bad tool in xmlToHistoryItem:" << tool;
  }
  return historyItemPtr(item);
}

changeAllHistoryItem::changeAllHistoryItem(const QDomElement& xmlHistory)
  : toolColor_(::xmlStringToFlossColor(::getElementText(xmlHistory,
                                                        "tool_color"))),
    toolColorIsNew_(::stringToBool(::getElementText(xmlHistory,
                                                    "color_is_new"))),
    priorColor_(::xmlStringToFlossColor(::getElementText(xmlHistory,
                                                         "old_color"))),
    coordinates_(::xmlToCoordinatesList(::getElementText(xmlHistory,
                                                         "coordinate_list")))
{ }

void changeAllHistoryItem::toXml(QDomDocument* doc, QDomElement* appendee)
  const {

  QDomElement itemElement(doc->createElement("history_item"));
  appendee->appendChild(itemElement);
  ::appendTextElement(doc, "tool",  "change all", &itemElement);
  ::appendTextElement(doc, "tool_color",  ::flossColorToString(toolColor_),
                      &itemElement);
  ::appendTextElement(doc, "color_is_new",
                      ::boolToString(toolColorIsNew_),
                      &itemElement);
  ::appendTextElement(doc, "old_color", ::flossColorToString(priorColor_),
                        &itemElement);
  ::appendCoordinatesList(doc, coordinates_, &itemElement);
}

dockListUpdate changeAllHistoryItem::
performHistoryEdit(mutableSquareImageContainer* container,
                   historyDirection direction) const {

  flossColor addedColor, removedColor;
  if (direction == H_BACK) {
    addedColor = priorColor_;
    removedColor = toolColor_;
  }
  else {
    addedColor = toolColor_;
    removedColor = priorColor_;
  }

  ::changeBlocks(&container->image_, coordinates_,
                 addedColor.qrgb(), container->originalDimension_, true);

  if (toolColorIsNew_) {
    container->addColor(addedColor);
    container->removeColor(removedColor);
    return dockListUpdate(addedColor, true, removedColor.color());
  }
  else {
    if (direction == H_BACK) {
      container->addColor(addedColor);
      return dockListUpdate(addedColor, true);
    }
    else {
      container->removeColor(removedColor);
      return dockListUpdate(addedColor, false, removedColor.color());
    }
  }
}

changeOneHistoryItem::changeOneHistoryItem(const QDomElement& xmlHistory)
  : toolColor_(::xmlStringToFlossColor(::getElementText(xmlHistory,
                                                        "tool_color"))),
    toolColorIsNew_(::stringToBool(::getElementText(xmlHistory,
                                                    "color_is_new"))),
    pixels_(::xmlToPixelList(::getElementText(xmlHistory, "pixel_list")))
{ }

void changeOneHistoryItem::toXml(QDomDocument* doc, QDomElement* appendee)
  const {

  QDomElement itemElement(doc->createElement("history_item"));
  appendee->appendChild(itemElement);
  ::appendTextElement(doc, "tool",  "change one", &itemElement);
  ::appendTextElement(doc, "tool_color",  ::flossColorToString(toolColor_),
                      &itemElement);
  ::appendTextElement(doc, "color_is_new",
                      ::boolToString(toolColorIsNew_),
                      &itemElement);
  ::appendPixelList(doc, pixels_, &itemElement);
}

dockListUpdate changeOneHistoryItem::
performHistoryEdit(mutableSquareImageContainer* container,
                   historyDirection direction) const {

  const flossColor newColor = toolColor_;
  if (direction == H_BACK) {
    ::changeBlocks(&container->image_, pixels_, container->originalDimension_);
  }
  else { // forward
    ::changeBlocks(&container->image_, pixels_, newColor.qrgb(),
                   container->originalDimension_);
    container->colorListCheckNeeded_ = true;
  }
  if (toolColorIsNew_) {
    if (direction == H_BACK) {
      container->removeColor(newColor);
      return dockListUpdate(flossColor(), false, newColor.color());
    }
    else { // forward
      container->addColor(newColor);
      return dockListUpdate(newColor, true);
    }
  }
  else {
    if (direction == H_BACK) {
      return dockListUpdate();
    }
    else { // forward
      return dockListUpdate(newColor, false);
    }
  }
}

fillRegionHistoryItem::fillRegionHistoryItem(const QDomElement& xmlHistory)
  : toolColor_(::xmlStringToFlossColor(::getElementText(xmlHistory,
                                                        "tool_color"))),
    toolColorIsNew_(::stringToBool(::getElementText(xmlHistory,
                                                    "color_is_new"))),
    priorColor_(::xmlStringToFlossColor(::getElementText(xmlHistory,
                                                         "old_color"))),
    coordinates_(::xmlToCoordinatesList(::getElementText(xmlHistory,
                                                         "coordinate_list")))
{ }

void fillRegionHistoryItem::toXml(QDomDocument* doc, QDomElement* appendee)
  const {

  QDomElement itemElement(doc->createElement("history_item"));
  appendee->appendChild(itemElement);
  ::appendTextElement(doc, "tool", "fill region", &itemElement);
  ::appendTextElement(doc, "tool_color",  ::flossColorToString(toolColor_),
                      &itemElement);
  ::appendTextElement(doc, "color_is_new",
                      ::boolToString(toolColorIsNew_),
                      &itemElement);
  ::appendTextElement(doc, "old_color", ::flossColorToString(priorColor_),
                        &itemElement);
  ::appendCoordinatesList(doc, coordinates_, &itemElement);
}

dockListUpdate fillRegionHistoryItem::
performHistoryEdit(mutableSquareImageContainer* container,
                   historyDirection direction) const {

  const flossColor newColor = toolColor_;
  if (direction == H_BACK) {
    ::changeBlocks(&container->image_, coordinates_, priorColor_.qrgb(),
                   container->originalDimension_, true);
  }
  else { // forward
    ::changeBlocks(&container->image_, coordinates_, newColor.qrgb(),
                   container->originalDimension_, true);
    container->colorListCheckNeeded_ = true;
  }
  if (toolColorIsNew_) {
    if (direction == H_BACK) {
      container->removeColor(newColor);
      // the add will be an empty operation unless this history's fill
      // completely overwrote the old color and the color list was
      // updated to reflect that
      bool colorAdded = container->addColor(priorColor_);
      return dockListUpdate(priorColor_, colorAdded, newColor.color());
    }
    else { // forward
      container->addColor(newColor);
      return dockListUpdate(newColor, true);
    }
  }
  else {
    if (direction == H_BACK) {
      bool colorAdded = container->addColor(priorColor_);
      return dockListUpdate(priorColor_, colorAdded);
    }
    else { // forward
      return dockListUpdate(newColor, false);
    }
  }
}

detailHistoryItem::detailHistoryItem(const QDomElement& xmlHistory)
  : detailPixels_(::xmlToHistoryPixelList(::getElementText(xmlHistory,
                                                           "history_pixel_list"))),
    newColorsType_(::getElementText(xmlHistory, "new_colors_type"))
{}

void detailHistoryItem::toXml(QDomDocument* doc, QDomElement* appendee)
  const {

  QDomElement itemElement(doc->createElement("history_item"));
  appendee->appendChild(itemElement);
  ::appendTextElement(doc, "tool", "detail", &itemElement);
  ::appendHistoryPixelList(doc, detailPixels_, &itemElement);
  ::appendTextElement(doc, "new_colors_type", newColorsType_.prefix(),
                      &itemElement);
}

dockListUpdate detailHistoryItem::
performHistoryEdit(mutableSquareImageContainer* container,
                   historyDirection direction) const {

  if (direction == H_BACK) {
    QVector<triC> colorsToRemove;
    for (int i = 0, size = detailPixels_.size(); i < size; ++i) {
      const historyPixel thisPixel = detailPixels_[i];
      ::changeOneBlock(&container->image_, thisPixel.x(), thisPixel.y(),
                       thisPixel.oldColor().qrgb(),
                       container->originalDimension_, true);
      if (thisPixel.newColorIsNew()) {
        colorsToRemove.push_back(thisPixel.newColor());
      }
    }
    container->removeColors(colorsToRemove);
    return dockListUpdate(flossColor(), false, colorsToRemove);
  }
  else { // forward
    QVector<flossColor> colorsToAdd;
    for (int i = 0, size = detailPixels_.size(); i < size; ++i) {
      const historyPixel thisPixel = detailPixels_[i];
      ::changeOneBlock(&container->image_, thisPixel.x(), thisPixel.y(),
                       thisPixel.newColor().qrgb(),
                       container->originalDimension_, true);
      if (thisPixel.newColorIsNew()) {
        const triC newColor = thisPixel.newColor();
        colorsToAdd.push_back(flossColor(newColor, newColorsType_));
      }
    }
    container->addColors(colorsToAdd);
    container->colorListCheckNeeded_ = true;
    return dockListUpdate(colorsToAdd);
  }
}

rareColorsHistoryItem::rareColorsHistoryItem(const QDomElement& xmlHistory)
  : items_(::xmlToColorChangeList(::getElementText(xmlHistory,
                                                   "color_change_list"))),
    rareColorTypes_(::xmlStringToFlossSet(::getElementText(xmlHistory,
                                                           "floss_list"))) {}

void rareColorsHistoryItem::toXml(QDomDocument* doc, QDomElement* appendee)
  const {

  QDomElement itemElement(doc->createElement("history_item"));
  appendee->appendChild(itemElement);
  ::appendTextElement(doc, "tool", "rare colors", &itemElement);
  ::appendColorChangeHistoryList(doc, items_, &itemElement);
  ::appendFlossList(doc, rareColorTypes_, &itemElement);
}

dockListUpdate rareColorsHistoryItem::
performHistoryEdit(mutableSquareImageContainer* container,
                   historyDirection direction) const {

  if (direction == H_BACK) {
    QVector<flossColor> colorsToAdd;
    for (int i = 0, size = items_.size(); i < size; ++i) {
      const colorChange thisColorChange = items_[i];
      const triC oldColor = thisColorChange.oldColor();
      ::changeBlocks(&container->image_, thisColorChange.coordinates(),
                     oldColor.qrgb(), container->originalDimension_, true);
      const QSet<flossColor>::const_iterator it =
        rareColorTypes_.constFind(flossColor(oldColor));
      if (it != rareColorTypes_.constEnd()) {
        colorsToAdd.push_back(*it);
      }
      else {
        if (!rareColorTypes_.empty()) {
          // we're storing types but couldn't find one we should have
          qWarning() << "Lost pixel in rareHistoryEdit" << ::ctos(oldColor);
        }
        colorsToAdd.push_back(flossColor(oldColor));
      }
    }
    container->addColors(colorsToAdd);
    return dockListUpdate(colorsToAdd);
  }
  else { // forward
    QVector<triC> colorsToRemove;
    for (int i = 0, size = items_.size(); i < size; ++i) {
      const colorChange thisColorChange = items_[i];
      ::changeBlocks(&container->image_, thisColorChange.coordinates(),
                     thisColorChange.newColor(), container->originalDimension_,
                     true);
      colorsToRemove.push_back(thisColorChange.oldColor());
    }
    container->removeColors(colorsToRemove);
    return dockListUpdate(colorsToRemove);
  }
}
