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

#include "windowSavers.h"

#include "xmlUtility.h"

colorCompareSaver::colorCompareSaver(const QDomElement& xmlElement)
  : modeSaver(::getElementText(xmlElement, "index").toInt(), 0),
    creationMode_(::getElementText(xmlElement, "creation_mode")),
    colors_(::loadColorListFromText(::getElementText(xmlElement,
                                                     "color_list"))) {

  const bool hidden = ::stringToBool(::getElementText(xmlElement, "hidden"));
  setHidden(hidden);
}

QDomElement colorCompareSaver::toXml(QDomDocument* doc) const {

  QDomElement element = doc->createElement("color_compare_image");
  ::appendTextElement(doc, "index", QString::number(index()), &element);
  const QString hiddenString = ::boolToString(hidden());
  ::appendTextElement(doc, "hidden", hiddenString, &element);
  ::appendTextElement(doc, "creation_mode", creationMode_, &element);
  ::appendColorList(doc, colors_, &element);

  return element;
}

squareWindowSaver::squareWindowSaver(const QDomElement& xmlElement)
  : modeSaver(::getElementText(xmlElement, "index").toInt(),
              ::getElementText(xmlElement, "parent_index").toInt()),
    creationMode_(::getElementText(xmlElement, "creation_mode")),
    squareDimension_(::getElementText(xmlElement,
                                      "square_dimension").toInt()) {

  bool hidden = ::stringToBool(::getElementText(xmlElement, "hidden"));
  setHidden(hidden);
}

QDomElement squareWindowSaver::toXml(QDomDocument* doc) const {

  QDomElement element = doc->createElement("square_window_image");
  ::appendTextElement(doc, "index", QString::number(index()), &element);
  const QString hiddenString = ::boolToString(hidden());
  ::appendTextElement(doc, "hidden", hiddenString, &element);
  ::appendTextElement(doc, "parent_index", QString::number(parentIndex()),
                      &element);
  ::appendTextElement(doc, "creation_mode", creationMode_, &element);
  ::appendTextElement(doc, "square_dimension",
                      QString::number(squareDimension_), &element);

  return element;
}

patternWindowSaver::patternWindowSaver(const QDomElement& xmlElement)
  : modeSaver(::getElementText(xmlElement, "index").toInt(),
              ::getElementText(xmlElement, "parent_index").toInt()),
    squareDimension_(::getElementText(xmlElement, "square_dimension").toInt()) {

  QDomNode historyNode =
    squareHistory_.importNode(xmlElement.
                              firstChildElement("square_history"),
                              true);
  squareHistory_.appendChild(historyNode);
}

QDomElement patternWindowSaver::toXml(QDomDocument* doc) const {

  QDomElement element = doc->createElement("pattern_window_image");
  ::appendTextElement(doc, "index", QString::number(index()), &element);
  const QString hiddenString = ::boolToString(hidden());
  ::appendTextElement(doc, "hidden", hiddenString, &element);
  ::appendTextElement(doc, "parent_index", QString::number(parentIndex()),
                      &element);
  ::appendTextElement(doc, "square_dimension",
                      QString::number(squareDimension_), &element);
  QDomNode historyNode =
    doc->importNode(squareHistory_.firstChildElement("square_history"),
                    true);
  element.appendChild(historyNode);

  return element;
}
