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

#ifndef SQUARETOOLHISTORIES_H
#define SQUARETOOLHISTORIES_H

#include <QtCore/QSharedData>

#include "triC.h"
#include "squareDockTools.h"

class pairOfInts;
class pixel;
class historyPixel;
class colorChange;
class mutableSquareImageContainer;
class historyItem;
class QDomDocument;
class QDomElement;
template<class T> class QExplicitlySharedDataPointer;

typedef QExplicitlySharedDataPointer<historyItem> historyItemPtr;

enum historyDirection {H_BACK, H_FORWARD};

// dockListUpdate holds the record of how the color list dock needs to be
// updated after completion of a tool use
class dockListUpdate {
 public:
  dockListUpdate() : singleColor_(true), colorsToAdd_(1, triC()),
    colorIsNew_(false) {}
  dockListUpdate(const triC& color, bool colorIsNew)
    : singleColor_(true), colorsToAdd_(1, color), colorIsNew_(colorIsNew)
  { }
  dockListUpdate(const triC& color, bool colorIsNew, triC removeColor)
    : singleColor_(true), colorsToAdd_(1, color), colorIsNew_(colorIsNew),
    colorsToRemove_(1, removeColor) { }
  dockListUpdate(const triC& color, bool colorIsNew,
                 const QVector<triC>& colorsToRemove)
    : singleColor_(true), colorsToAdd_(1, color), colorIsNew_(colorIsNew),
    colorsToRemove_(colorsToRemove) {}
  explicit dockListUpdate(const QVector<triC>& colorsToAdd)
    : singleColor_(false), colorsToAdd_(colorsToAdd), colorIsNew_(false) {}
  // (<remove> is a dummy to distinguish this from the colorsToAdd version)
  dockListUpdate(const QVector<triC>& colorsToRemove, bool )
    : singleColor_(false), colorIsNew_(false),
    colorsToRemove_(colorsToRemove) {}
  // return true if the update is for a single color
  bool singleColor() const { return singleColor_; }
  // return true if the tool color for this update is a color that wasn't
  // on the color list before
  bool colorIsNew() const { return colorIsNew_; }
  // return the tool color for the tool use that was the source for this
  // update
  triC color() const { return colorsToAdd_[0]; }
  QVector<triC> colors() const { return colorsToAdd_; }
  bool removeColors() const { return !colorsToRemove_.empty(); }
  QVector<triC> colorsToRemove() const { return colorsToRemove_; }
 private:
  bool singleColor_; // true if this update is for a single color
  // colors that need to be added to the color list dock (if they don't
  // already exist there)
  QVector<triC> colorsToAdd_;
  // true if the color for the tool use that prompted this update wasn't
  // on the color list before
  bool colorIsNew_;
  // colors that need to be removed from the color list dock
  QVector<triC> colorsToRemove_;
};

class historyItem : public QSharedData {

 public :
  // append the xml version of this history item to <appendee>
  virtual void toXml(QDomDocument* doc, QDomElement* appendee) const = 0;
  // perform a history edit on <container> given the data of this history
  // item and the <direction> of the edit (forward or backward)
  virtual dockListUpdate
    performHistoryEdit(mutableSquareImageContainer* container,
                       historyDirection direction) const = 0;
  // a "factory" that returns a historyItem pointer to a derived history
  // item whose type and data are determined by the <xml> content
  static historyItemPtr xmlToHistoryItem(const QDomElement& xml);
};

class changeAllHistoryItem : public historyItem {

 public:
 changeAllHistoryItem(QRgb oldColor, QRgb toolColor, bool toolColorIsNew,
                      const QVector<pairOfInts>& coordinates)
    : toolColor_(toolColor), toolColorIsNew_(toolColorIsNew),
    priorColor_(oldColor), coordinates_(coordinates) {}
  explicit changeAllHistoryItem(const QDomElement& xmlHistory);
  void toXml(QDomDocument* doc, QDomElement* appendee) const;
  dockListUpdate performHistoryEdit(mutableSquareImageContainer* container,
                                    historyDirection direction) const;
  QRgb toolColor() const { return toolColor_; }
  QRgb oldColor() const { return priorColor_; }
  QVector<pairOfInts> coordinates() const { return coordinates_; }

 private:
  const QRgb toolColor_; // the color associated with the tool used
  const bool toolColorIsNew_; // true if the tool color didn't exist before
  const QRgb priorColor_; // the color being painted over
  const QVector<pairOfInts> coordinates_; // the coordinates painted over
};

class changeOneHistoryItem : public historyItem {

 public:
  changeOneHistoryItem(QRgb toolColor, bool toolColorIsNew,
                       const QVector<pixel>& pixels)
    :  toolColor_(toolColor), toolColorIsNew_(toolColorIsNew),
    pixels_(pixels) {}
  explicit changeOneHistoryItem(const QDomElement& xmlHistory);
  void toXml(QDomDocument* doc, QDomElement* appendee) const;
  dockListUpdate performHistoryEdit(mutableSquareImageContainer* container,
                                    historyDirection direction) const;

 private:
  const QRgb toolColor_; // the color associated with the tool used
  const bool toolColorIsNew_; // true if the tool color didn't exist before
  const QVector<pixel> pixels_; // the old pixels we've changed
};

class fillRegionHistoryItem : public historyItem {

 public:
  fillRegionHistoryItem(QRgb oldColor, QRgb toolColor, bool toolColorIsNew,
                        const QVector<pairOfInts>& coordinates)
    :  toolColor_(toolColor), toolColorIsNew_(toolColorIsNew),
    priorColor_(oldColor), coordinates_(coordinates) {}
  explicit fillRegionHistoryItem(const QDomElement& xmlHistory);
  void toXml(QDomDocument* doc, QDomElement* appendee) const;
  dockListUpdate performHistoryEdit(mutableSquareImageContainer* container,
                                    historyDirection direction) const;

 private:
  const QRgb toolColor_; // the color associated with the tool used
  const bool toolColorIsNew_; // true if the tool color didn't exist before
  const QRgb priorColor_; // the color being painted over
  // square coordinates of the squares painted over
  const QVector<pairOfInts> coordinates_;
};

class detailHistoryItem : public historyItem {

 public:
  explicit detailHistoryItem(const QVector<historyPixel>& detailPixels)
    : detailPixels_(detailPixels) {}
  explicit detailHistoryItem(const QDomElement& xmlHistory);
  void toXml(QDomDocument* doc, QDomElement* appendee) const;
  dockListUpdate performHistoryEdit(mutableSquareImageContainer* container,
                                    historyDirection direction) const;

 private:
  const QVector<historyPixel> detailPixels_;
};

class rareColorsHistoryItem : public historyItem {

 public:
  explicit rareColorsHistoryItem(const QList<colorChange>& items)
    : items_(items) {}
  explicit rareColorsHistoryItem(const QDomElement& xmlHistory);
  void toXml(QDomDocument* doc, QDomElement* appendee) const;
  dockListUpdate performHistoryEdit(mutableSquareImageContainer* container,
                                    historyDirection direction) const;

 private:
  // a changeAllHistoryItem for each rare color that is replaced
  const QList<colorChange> items_;
};

#endif
