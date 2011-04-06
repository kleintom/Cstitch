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

#ifndef XMLUTILITY_H
#define XMLUTILITY_H

#include <QtCore/QString>

#include "triC.h"

class pairOfInts;
class pixel;
class historyPixel;
class colorChange;
class QDomDocument;
class QDomElement;

void appendTextElement(QDomDocument* doc, const QString& elementName,
                       const QString& text, QDomElement* appendee,
                       const QString& attributeName = QString(),
                       const QString& attributeValue = QString());

QString getElementText(const QDomDocument& doc,
                       const QString& elementName);
QString getElementText(const QDomElement& element,
                       const QString& elementName);

void appendColorList(QDomDocument* doc, const QVector<triC>& colors,
                     QDomElement* appendee,
                     const QString& attributeName = QString(),
                     const QString& attributeValue = QString());
void appendCoordinatesList(QDomDocument* doc,
                           const QVector<pairOfInts>& coordinates,
                           QDomElement* appendee);
QVector<pairOfInts> xmlToCoordinatesList(const QString& list);

void appendPixelList(QDomDocument* doc, const QVector<pixel>& pixels,
                     QDomElement* appendee);
QVector<pixel> xmlToPixelList(const QString& list);

void appendHistoryPixelList(QDomDocument* doc,
                            const QVector<historyPixel>& pixels,
                            QDomElement* appendee);
QVector<historyPixel> xmlToHistoryPixelList(const QString& list);

void appendColorChangeHistoryList(QDomDocument* doc,
                                  const QList<colorChange>& colorChanges,
                                  QDomElement* appendee);
QList<colorChange> xmlToColorChangeList(const QString& list);

QString rgbToString(const triC& color);
inline QString rgbToString(QRgb color) { return rgbToString(triC(color)); }
QRgb xmlStringToRgb(QString string);

inline QString boolToString(bool b) { return b ? "true" : "false"; }
inline bool stringToBool(const QString& s) {
  return s == "true" ? true : false;
}

QString coordinateListToString(const QVector<pairOfInts>& coordinates);
QString coordinatesToString(const pairOfInts& p);

QString pixelToString(const pixel& p);

QString historyPixelToString(const historyPixel& p);

QVector<triC> loadColorListFromText(const QString& list);

#endif
