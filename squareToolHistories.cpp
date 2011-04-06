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
  : toolColor_(::xmlStringToRgb(::getElementText(xmlHistory,
                                                 "tool_color"))),
    toolColorIsNew_(::stringToBool(::getElementText(xmlHistory,
                                                    "color_is_new"))),
    priorColor_(::xmlStringToRgb(::getElementText(xmlHistory, "old_color"))),
    coordinates_(::xmlToCoordinatesList(::getElementText(xmlHistory,
                                                         "coordinate_list")))
{ }

void changeAllHistoryItem::toXml(QDomDocument* doc, QDomElement* appendee)
  const {

  QDomElement itemElement(doc->createElement("history_item"));
  appendee->appendChild(itemElement);
  ::appendTextElement(doc, "tool",  "change all", &itemElement);
  ::appendTextElement(doc, "tool_color",  ::rgbToString(toolColor_),
                      &itemElement);
  ::appendTextElement(doc, "color_is_new",
                      ::boolToString(toolColorIsNew_),
                      &itemElement);
  ::appendTextElement(doc, "old_color", ::rgbToString(priorColor_),
                        &itemElement);
  ::appendCoordinatesList(doc, coordinates_, &itemElement);
}

dockListUpdate changeAllHistoryItem::
performHistoryEdit(mutableSquareImageContainer* container,
                   historyDirection direction) const {

  QRgb addedColor, removedColor;
  if (direction == H_BACK) {
    addedColor = priorColor_;
    removedColor = toolColor_;
  }
  else {
    addedColor = toolColor_;
    removedColor = priorColor_;
  }

  ::changeBlocks(&container->image_, coordinates_,
                 addedColor, container->originalDimension_, true);

  if (toolColorIsNew_) {
    container->addColor(addedColor);
    container->removeColor(removedColor);
    return dockListUpdate(triC(addedColor), true, triC(removedColor));
  }
  else {
    if (direction == H_BACK) {
      container->addColor(addedColor);
      return dockListUpdate(addedColor, true);
    }
    else {
      container->removeColor(removedColor);
      return dockListUpdate(triC(addedColor), false, triC(removedColor));
    }
  }
}

changeOneHistoryItem::changeOneHistoryItem(const QDomElement& xmlHistory)
  : toolColor_(::xmlStringToRgb(::getElementText(xmlHistory,
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
  ::appendTextElement(doc, "tool_color",  ::rgbToString(toolColor_),
                      &itemElement);
  ::appendTextElement(doc, "color_is_new",
                      ::boolToString(toolColorIsNew_),
                      &itemElement);
  ::appendPixelList(doc, pixels_, &itemElement);
}

dockListUpdate changeOneHistoryItem::
performHistoryEdit(mutableSquareImageContainer* container,
                   historyDirection direction) const {

  const QRgb newColor = toolColor_;
  if (direction == H_BACK) {
    ::changeBlocks(&container->image_, pixels_, container->originalDimension_);
  }
  else { // forward
    ::changeBlocks(&container->image_, pixels_, newColor,
                   container->originalDimension_);
  }
  if (toolColorIsNew_) {
    if (direction == H_BACK) {
      container->removeColor(newColor);
      return dockListUpdate(triC(), false, triC(newColor));
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
  : toolColor_(::xmlStringToRgb(::getElementText(xmlHistory,
                                                 "tool_color"))),
    toolColorIsNew_(::stringToBool(::getElementText(xmlHistory,
                                                    "color_is_new"))),
    priorColor_(::xmlStringToRgb(::getElementText(xmlHistory, "old_color"))),
    coordinates_(::xmlToCoordinatesList(::getElementText(xmlHistory,
                                                         "coordinate_list")))
{ }

void fillRegionHistoryItem::toXml(QDomDocument* doc, QDomElement* appendee)
  const {

  QDomElement itemElement(doc->createElement("history_item"));
  appendee->appendChild(itemElement);
  ::appendTextElement(doc, "tool", "fill region", &itemElement);
  ::appendTextElement(doc, "tool_color",  ::rgbToString(toolColor_),
                      &itemElement);
  ::appendTextElement(doc, "color_is_new",
                      ::boolToString(toolColorIsNew_),
                      &itemElement);
  ::appendTextElement(doc, "old_color", ::rgbToString(priorColor_),
                        &itemElement);
  ::appendCoordinatesList(doc, coordinates_, &itemElement);
}

dockListUpdate fillRegionHistoryItem::
performHistoryEdit(mutableSquareImageContainer* container,
                   historyDirection direction) const {

  const QRgb newColor = toolColor_;
  if (direction == H_BACK) {
    ::changeBlocks(&container->image_, coordinates_, priorColor_,
                   container->originalDimension_, true);
  }
  else { // forward
    ::changeBlocks(&container->image_, coordinates_, newColor,
                   container->originalDimension_, true);
  }
  if (toolColorIsNew_) {
    if (direction == H_BACK) {
      container->removeColor(newColor);
      return dockListUpdate(triC(), false, triC(newColor));
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

detailHistoryItem::detailHistoryItem(const QDomElement& xmlHistory)
  : detailPixels_(::xmlToHistoryPixelList(::getElementText(xmlHistory,
                                                           "history_pixel_list")))
{}

void detailHistoryItem::toXml(QDomDocument* doc, QDomElement* appendee)
  const {

  QDomElement itemElement(doc->createElement("history_item"));
  appendee->appendChild(itemElement);
  ::appendTextElement(doc, "tool", "detail", &itemElement);
  ::appendHistoryPixelList(doc, detailPixels_, &itemElement);
}

dockListUpdate detailHistoryItem::
performHistoryEdit(mutableSquareImageContainer* container,
                   historyDirection direction) const {

  if (direction == H_BACK) {
    QVector<triC> colorsToRemove;
    for (QVector<historyPixel>::const_iterator it = detailPixels_.begin(),
           end = detailPixels_.end(); it != end; ++it) {
      ::changeOneBlock(&container->image_, (*it).x(), (*it).y(),
                       (*it).oldColor().qrgb(), container->originalDimension_,
                       true);
      if ((*it).newColorIsNew()) {
        colorsToRemove.push_back((*it).newColor());
      }
    }
    container->removeColors(colorsToRemove);
    return dockListUpdate(triC(), false, colorsToRemove);
  }
  else { // forward
    QVector<triC> colorsToAdd;
    for (QVector<historyPixel>::const_iterator it = detailPixels_.begin(),
           end = detailPixels_.end(); it != end; ++it) {
      ::changeOneBlock(&container->image_, (*it).x(), (*it).y(),
                       (*it).newColor().qrgb(), container->originalDimension_,
                       true);
      if ((*it).newColorIsNew()) {
        colorsToAdd.push_back((*it).newColor());
      }
    }
    container->addColors(colorsToAdd);
    return dockListUpdate(colorsToAdd);
  }
}

rareColorsHistoryItem::rareColorsHistoryItem(const QDomElement& xmlHistory)
  : items_(::xmlToColorChangeList(::getElementText(xmlHistory,
                                                   "color_change_list"))) {}

void rareColorsHistoryItem::toXml(QDomDocument* doc, QDomElement* appendee)
  const {

  QDomElement itemElement(doc->createElement("history_item"));
  appendee->appendChild(itemElement);
  ::appendTextElement(doc, "tool", "rare colors", &itemElement);
  ::appendColorChangeHistoryList(doc, items_, &itemElement);
}

dockListUpdate rareColorsHistoryItem::
performHistoryEdit(mutableSquareImageContainer* container,
                   historyDirection direction) const {

  if (direction == H_BACK) {
    QVector<triC> colorsToAdd;
    for (int i = 0, size = items_.size(); i < size; ++i) {
      const colorChange thisColorChange = items_[i];
      ::changeBlocks(&container->image_, thisColorChange.coordinates(),
                     thisColorChange.oldColor(), container->originalDimension_,
                     true);
      colorsToAdd.push_back(thisColorChange.oldColor());
    }
    container->addColors(colorsToAdd);
    return dockListUpdate(colorsToAdd);
  }
  else {
    QVector<triC> colorsToRemove;
    for (int i = 0, size = items_.size(); i < size; ++i) {
      const colorChange thisColorChange = items_[i];
      ::changeBlocks(&container->image_, thisColorChange.coordinates(),
                     thisColorChange.newColor(), container->originalDimension_,
                     true);
      colorsToRemove.push_back(thisColorChange.oldColor());
    }
    container->removeColors(colorsToRemove);
    return dockListUpdate(colorsToRemove, true);
  }
}
