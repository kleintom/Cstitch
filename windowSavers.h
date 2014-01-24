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

#ifndef WINDOWSAVERS_H
#define WINDOWSAVERS_H

#include <QtCore/QVector>

#include <QtXml/QDomDocument>

#include "triC.h"

// hold a record of an index, its parent index, and any children indices
// hidden means this index has been deleted in some way, but we still need
// to keep a record of it for its children
class parentChildren {

 public:
  parentChildren() : thisIndex_(-1), thisHidden_(false), parentIndex_(-1),
    childrenIndices_() {}
  parentChildren(int thisIndex, int parentIndex)
    : thisIndex_(thisIndex), thisHidden_(false), parentIndex_(parentIndex)
    {}
  int parentIndex() const { return parentIndex_; }
  int thisIndex() const { return thisIndex_; }
  void setThisIndex(int index) { thisIndex_ = index; }
  void setHidden(bool b) { thisHidden_ = b; }
  bool hidden() const { return thisHidden_; }
  void addChild(int childIndex) { childrenIndices_.push_back(childIndex); }
  void removeChild(int childIndex) {
    childrenIndices_.removeOne(childIndex);
  }
  bool hasChildren() const { return !childrenIndices_.isEmpty(); }
  QList<int> children() const { return childrenIndices_; }

 private:
  int thisIndex_;
  // thisIndex_ has been deleted, but there are still children
  bool thisHidden_;
  int parentIndex_;
  QList<int> childrenIndices_;
};

// interface for the specific mode saver classes; the basic function is
// to take in data via a constructor and write it out via toXml
class modeSaver : public parentChildren {

 public:
  modeSaver() {}
  modeSaver(int thisIndex, int parentIndex)
    : parentChildren(thisIndex, parentIndex) {}
  virtual ~modeSaver() {}
  virtual QDomElement
    toXml(QDomDocument* doc) const = 0;
  int index() const { return thisIndex(); }
  int parent() const { return parentIndex(); }
};

class colorCompareSaver : public modeSaver {

 public:
  colorCompareSaver() : creationMode_(""), colors_() {}
  colorCompareSaver(int thisIndex, int parentIndex,
                    const QString& creationMode,
                    const QVector<triC>& colors)
    : modeSaver(thisIndex, parentIndex), creationMode_(creationMode),
    colors_(colors) {}
  explicit colorCompareSaver(const QDomElement& xmlElement);
  QString creationMode() const { return creationMode_; }
  const QVector<triC>& colors() const { return colors_; }
  QDomElement toXml(QDomDocument* doc) const;

 private:
  QString creationMode_;
  QVector<triC> colors_;
};

class squareWindowSaver : public modeSaver {

 public:
  squareWindowSaver() : creationMode_(""), squareDimension_(0) {}
  squareWindowSaver(int thisIndex, int parentIndex,
                    const QString& creationMode, int squareDimension)
    : modeSaver(thisIndex, parentIndex), creationMode_(creationMode),
    squareDimension_(squareDimension) {}
  explicit squareWindowSaver(const QDomElement& xmlElement);
  QString creationMode() const { return creationMode_; }
  int squareDimension() const { return squareDimension_; }
  QDomElement toXml(QDomDocument* doc) const;

 private:
  QString creationMode_;
  int squareDimension_;
};

class patternWindowSaver : public modeSaver {

 public:
  patternWindowSaver() : modeSaver() {}
  patternWindowSaver(int thisIndex, int parentIndex, int squareDimension,
                    const QDomDocument& xmlSquareHistory)
    : modeSaver(thisIndex, parentIndex), squareDimension_(squareDimension),
    squareHistory_(xmlSquareHistory) {}
  explicit patternWindowSaver(const QDomElement& xmlElement);
  QDomElement toXml(QDomDocument* doc) const;
  QDomDocument squareHistory() const { return squareHistory_; }

 private:
  int squareDimension_;
  QDomDocument squareHistory_;
};

#endif
