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

#include "xmlUtility.h"

#include <QtCore/QDebug>

#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include "imageUtility.h"

void appendTextElement(QDomDocument* doc, const QString& elementName,
                       const QString& text, QDomElement* appendee,
                       const QString& attributeName,
                       const QString& attributeValue) {

  QDomElement element(doc->createElement(elementName));
  element.appendChild(doc->createTextNode(text));
  appendee->appendChild(element);

  if (attributeName != "") {
    element.setAttribute(attributeName, attributeValue);
  }
}

QString getElementText(const QDomDocument& doc,
                       const QString& elementName) {

  return getElementText(doc.documentElement(), elementName);
}

QString getElementText(const QDomElement& element,
                       const QString& elementName) {

  return element.firstChildElement(elementName).text();
}

QString getElementAttribute(const QDomDocument& doc,
                            const QString& elementName,
                            const QString& attributeName) {

  return doc.firstChildElement(elementName).attribute(attributeName);
}

void appendColorList(QDomDocument* doc, const QVector<triC>& colors,
                     QDomElement* appendee,
                     const QString& attributeName,
                     const QString& attributeValue) {

  QString colorListString;
  for (int i = 0, size = colors.size(); i < size; ++i) {
    colorListString += rgbToString(colors[i]) + ";";
  }
  if (colorListString != "") { // remove trailing ;
    colorListString.remove(colorListString.size() - 1, 1);
  }
  QDomElement element(doc->createElement("color_list"));
  element.appendChild(doc->createTextNode(colorListString));
  appendee->appendChild(element);
  if (attributeName != "") {
    element.setAttribute(attributeName, attributeValue);
  }
  element.setAttribute("count", QString::number(colors.size()));
}

QString coordinateListToString(const QVector<pairOfInts>& coordinates) {

  QString coordinateListString;
  for (int i = 0, size = coordinates.size(); i < size; ++i) {
    coordinateListString += ::coordinatesToString(coordinates[i]) + ";";
  }
  if (coordinateListString != "") { // remove trailing ;
    coordinateListString.remove(coordinateListString.size() - 1, 1);
  }
  return coordinateListString;
}

void appendCoordinatesList(QDomDocument* doc,
                           const QVector<pairOfInts>& coordinates,
                           QDomElement* appendee) {

  const QString coordinateListString =
    ::coordinateListToString(coordinates);
  ::appendTextElement(doc, "coordinate_list", coordinateListString,
                      appendee, "count",
                      QString::number(coordinates.size()));
}

QString coordinatesToString(const pairOfInts& p) {

  return "(" + QString::number(p.x()) + "," + QString::number(p.y()) + ")";
}

QVector<pairOfInts> xmlToCoordinatesList(const QString& list) {

  QVector<pairOfInts> returnList;
  if (list.isEmpty()) {
    return returnList;
  }
  QStringList coordinates = list.split(";");
  QRegExp
    regex("^\\((\\d+),(\\d+)\\)$");
  for (int i = 0, size = coordinates.size(); i < size; ++i) {
    if (regex.indexIn(coordinates[i]) != -1) {
      QStringList matches = regex.capturedTexts();
      if (matches.size() == 3) {
        QList<int> matchInts;
        // convert strings to ints!
        // NOTE that this switches the indices down by one from matches
        for (int j = 1; j <= 2; ++j) {
          matchInts.push_back(matches[j].toInt());
        }
        pairOfInts p(matchInts[0], matchInts[1]);
        returnList.push_back(p);
      }
    }
    else if (size > 1) {
      qWarning() << "Bad match in xmlToCoordinatesList:" << coordinates[i];
    }
  }
  return returnList;
}

void appendPixelList(QDomDocument* doc, const QVector<pixel>& pixels,
                     QDomElement* appendee) {

  QString pixelListString;
  for (int i = 0, size = pixels.size(); i < size; ++i) {
    pixelListString += ::pixelToString(pixels[i]) + ";";
  }
  if (pixelListString != "") { // remove trailing ;
    pixelListString.chop(1);
  }
  ::appendTextElement(doc, "pixel_list", pixelListString, appendee,
                      "count", QString::number(pixels.size()));
}

QString pixelToString(const pixel& p) {

  return "[" + ::coordinatesToString(pairOfInts(p.x(), p.y())) + "." +
    ::rgbToString(p.color()) + "]";
}

QVector<pixel> xmlToPixelList(const QString& list) {

  QVector<pixel> returnList;
  if (list.isEmpty()) {
    return returnList;
  }
  const QStringList pixels = list.split(";");
  QRegExp
    regex("^\\[\\((\\d+),(\\d+)\\)\\.\\((\\d+),(\\d+),(\\d+)\\)\\]$");
  for (int i = 0, size = pixels.size(); i < size; ++i) {
    if (regex.indexIn(pixels[i]) != -1) {
      QStringList matches = regex.capturedTexts();
      if (matches.size() == 6) {
        QList<int> matchInts;
        // convert strings to ints!
        // NOTE that this switches the indices down by one from matches
        for (int j = 1; j <= 5; ++j) {
          matchInts.push_back(matches[j].toInt());
        }
        pixel p(qRgb(matchInts[2], matchInts[3], matchInts[4]),
                pairOfInts(matchInts[0], matchInts[1]));
        returnList.push_back(p);
      }
    }
    else if (size > 1) {
      qWarning() << "Bad match in xmlToPixelList:" << pixels[i];
    }
  }
  return returnList;
}

void appendHistoryPixelList(QDomDocument* doc,
                            const QVector<historyPixel>& pixels,
                            QDomElement* appendee) {

  QString pixelListString;
  for (int i = 0, size = pixels.size(); i < size; ++i) {
    pixelListString += ::historyPixelToString(pixels[i]) + ";";
  }
  if (pixelListString != "") { // remove trailing ;
    pixelListString.remove(pixelListString.size() - 1, 1);
  }
  ::appendTextElement(doc, "history_pixel_list", pixelListString, appendee,
                      "count", QString::number(pixels.size()));
}

void appendColorChangeHistoryList(QDomDocument* doc,
                                  const QList<colorChange>& colorChanges,
                                  QDomElement* appendee) {

  QString colorChangeString;
  for (int i = 0, size = colorChanges.size(); i < size; ++i) {
    colorChangeString += "{" + ::rgbToString(colorChanges[i].oldColor()) +
      ":" + ::rgbToString(colorChanges[i].newColor()) + ":" +
      ::coordinateListToString(colorChanges[i].coordinates()) + "}+";
  }
  if (colorChangeString != "") { // remove trailing "+"
    colorChangeString.remove(colorChangeString.size() - 1, 1);
  }
  ::appendTextElement(doc, "color_change_list", colorChangeString,
                      appendee, "count",
                      QString::number(colorChanges.size()));
}

QList<colorChange> xmlToColorChangeList(const QString& list) {

  QList<colorChange> returnList;
  if (list.isEmpty()) {
    return returnList;
  }

  const QStringList colorChanges = list.split("+");
  for (int i = 0, size = colorChanges.size(); i < size; ++i) {
    QString thisColorChangeString = colorChanges[i];
    // remove {}
    thisColorChangeString.remove(thisColorChangeString.size() - 1, 1);
    thisColorChangeString.remove(0, 1);
    const QStringList colorChangeParts = thisColorChangeString.split(":");
    if (colorChangeParts.size() != 3) {
      qWarning() << "Bad string in xmlToColorChangeList:" <<
        thisColorChangeString;
      return returnList;
    }
    const triC oldColor = ::xmlStringToRgb(colorChangeParts[0]);
    const triC newColor = ::xmlStringToRgb(colorChangeParts[1]);
    const QVector<pairOfInts> coordinates =
      ::xmlToCoordinatesList(colorChangeParts[2]);
    returnList.push_back(colorChange(oldColor.qrgb(), newColor.qrgb(),
                                     coordinates));
  }
  return returnList;
}

QVector<triC> loadColorListFromText(const QString& list) {

  QVector<triC> colorList;
  if (list.isEmpty()) {
    return colorList;
  }
  QStringList stringList(list.split(";"));
  for (int i = 0, size = stringList.size(); i < size; ++i) {
    colorList.push_back(::xmlStringToRgb(stringList[i]));
  }
  return colorList;
}

QString rgbToString(const triC& color) {

  return  "(" + QString::number(color.r()) + "," +
    QString::number(color.g()) + "," + QString::number(color.b()) + ")";
}

QRgb xmlStringToRgb(const QString& string) {

  QRegExp regex("^\\((\\d+),(\\d+),(\\d+)\\)$");
  if (regex.indexIn(string) != -1) {
    QStringList matches = regex.capturedTexts();
    // remember that matches[0] is the entire match
    return qRgb(matches[1].toInt(), matches[2].toInt(), matches[3].toInt());
  }
  else {
    return qRgb(0, 0, 0);
  }
}

QString flossColorToString(const flossColor& color) {

  return color.prefix() + ::rgbToString(color.color());
}

flossColor xmlStringToFlossColor(const QString& string) {

  const QChar flossPrefix = string[0];
  if (flossPrefix != '(') { // floss
    const QString rgbString = string.mid(1); // all but the first
    const triC color = ::xmlStringToRgb(rgbString);
    return flossColor(color, flossPrefix);
  }
  else { // just rgb, no floss type
    return flossColor(::xmlStringToRgb(string));
  }    
}

QString flossSetToString(const QSet<flossColor>& colors) {

  QString returnString;
  for (QSet<flossColor>::const_iterator it = colors.constBegin(),
         end = colors.constEnd(); it != end; ++it) {
    returnString += ::flossColorToString(*it) + ";";
  }
  if (colors.size() > 0) {
    returnString.chop(1); // remove trailing comma
  }
  return returnString;
}

void appendFlossList(QDomDocument* doc, const QSet<flossColor>& colors,
                     QDomElement* appendee) {

  const QString xmlString = ::flossSetToString(colors);
  ::appendTextElement(doc, "floss_list", xmlString, appendee, "count",
                      QString::number(colors.size()));
}

QSet<flossColor> xmlStringToFlossSet(const QString& string) {

  QSet<flossColor> returnSet;
  if (string.isEmpty()) {
    return returnSet;
  }
  const QStringList stringList(string.split(';'));
  for (int i = 0, size = stringList.size(); i < size; ++i) {
    returnSet.insert(::xmlStringToFlossColor(stringList[i]));
  }
  return returnSet;
}

QString historyPixelToString(const historyPixel& p) {

  return "{" +
    ::pixelToString(pixel(p.oldColor().qrgb(), pairOfInts(p.x(), p.y()))) +
    ":" + ::rgbToString(p.newColor()) +
    ":" + ::boolToString(p.newColorIsNew()) + "}";
}

QVector<historyPixel> xmlToHistoryPixelList(const QString& list) {

  QVector<historyPixel> returnList;
  if (list.isEmpty()) {
    return returnList;
  }
  const QStringList stringList(list.split(";"));
  QRegExp
    regex("^\\{\\[\\((\\d+),(\\d+)\\)\\.\\((\\d+),(\\d+),(\\d+)\\)\\]:\\((\\d+),(\\d+),(\\d+)\\):(true|false)\\}$");
  for (int i = 0, size = stringList.size(); i < size; ++i) {
    if (regex.indexIn(stringList[i]) != -1) {
      QStringList matches = regex.capturedTexts();
      if (matches.size() == 10) {
        QList<int> matchInts;
        // convert strings to ints!
        // NOTE that this switches the indices down by one from matches
        for (int j = 1; j <= 8; ++j) {
          matchInts.push_back(matches[j].toInt());
        }
        historyPixel hp(pairOfInts(matchInts[0], matchInts[1]),
                        qRgb(matchInts[2], matchInts[3], matchInts[4]),
                        qRgb(matchInts[5], matchInts[6], matchInts[7]),
                        ::stringToBool(matches[9]));
        returnList.push_back(hp);
      }
    }
    else {
      qWarning() << "Bad match in xmlToHistoryPixelList:" << stringList[i];
    }
  }
  return returnList;
}
