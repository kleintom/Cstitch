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

#ifndef PATTERNIMAGECONTAINER_H
#define PATTERNIMAGECONTAINER_H

#include <QtCore/QObject>
#include <QtCore/QMetaType>

#include "triC.h"
#include "symbolChooser.h"

class patternWindow;
class QDomDocument;
class QDomElement;
class QMouseEvent;

// history information for a symbol change: the old symbol's index, the
// new symbol's index, and the symbol's color
class historyIndex {
 public:
  historyIndex(int oldIndex, int newIndex, const triC& color)
    : oldIndex_(oldIndex), newIndex_(newIndex), color_(color) { }
  explicit historyIndex(const QDomElement& xmlIndex);
  int oldIndex() const { return oldIndex_; }
  int newIndex() const { return newIndex_; }
  triC color() const { return color_; }
  QString toString() const;
 private:
  int oldIndex_;
  int newIndex_;
  triC color_;
};

//
// patternImageContainer maintains the status of a pattern/square image
// pair, such as color<->symbol associations
//
//// Implementation notes
// Initial color<->symbol associations are determined by symbolChooser_
// given the initial input colors_.  Users can change associations using
// changeSymbol (and mouseActivatedChangeSymbol).
// The only entire image stored here is the initial squared image; all
// other images are reconstituted by a color->pixmap hash and the original
// square image - this is _necessary_ and not merely convenient since
// entire patterned images large enough to be able to read the symbols
// can easily run into the hundreds of megs.  The default size of symbols
// and color squares is determined by symbolDimension_.  Each time
// symbols of a different size are requested symbolChooser_ must recreate
// them all at the new size (we don't like scaling things that are to
// be read).
//
class patternImageContainer : public QObject {

  Q_OBJECT

 public:
  mutable QAtomicInt ref; // for patternImagePtr
  // <squareImage> is the square image this pattern image is based on,
  // <squareDimension> is the square size of <squareImage>,
  // <baseSymbolDim> is the default initial symbol dimension,
  // <colors> are the colors of <squareImage>
  patternImageContainer(const QImage& squareImage,
                        const QString& imageName, int squareDimension,
                        int baseSymbolDim, const QVector<triC>& colors);
  // return the pattern image using the current symbol size setting
  QImage patternImageCurSymbolSize();
  const QImage& squareImage() const { return squareImage_; }
  // change the current symbol dimension and update colorSquares_ to have
  // the same dimension
  void setSymbolDimension(int dimension);
  int symbolDimension() const { return symbolDimension_; }
  // true if patternWindow is currently displaying the square version
  // of this square/pattern image combination
  bool viewingSquareImage() const { return viewingSquareImage_; }
  void setViewingSquareImage(bool b) { viewingSquareImage_ = b; }
  // return the width of the pattern image using the base symbol dimension
  int basePatternWidth() const {
    return (squareImage_.width()/squareDimension_)*baseSymbolDim_;
  }
  // return the height of the pattern image using the base symbol dimension
  int basePatternHeight() const {
    return (squareImage_.height()/squareDimension_)*baseSymbolDim_;
  }
  int squareDimension() const { return squareDimension_; }
  QString name() const { return imageName_; }
  // return the symbol for <color> with size <symbolDim> and no border
  QPixmap symbolNoBorder(const triC& color, int symbolDim);
  // return the symbols using the current symbol dimension and a default
  // border width
  QHash<QRgb, QPixmap> symbols();
  // return the symbols using <symbolDim> as width and no border
  QHash<QRgb, QPixmap> symbolsNoBorder(int symbolDim);
  QHash<QRgb, QPixmap> colorSquares() const { return colorSquares_; }
  const QVector<triC>& colors() { return colors_; }
  // pop up a symbol change dialog for the user to change the symbol for
  // <color>.
  // Return true if the symbol was actually changed.
  bool changeSymbol(const triC& color);
  // use <event width and height> and <event> to determine where the user
  // clicked on the image and pop up a symbol change dialog for the user
  // to change the symbol for the color at that location.
  // Return true if the symbol was actually changed.
  bool mouseActivatedChangeSymbol(int eventImageWidth,
                                  int eventImageHeight,
                                  QMouseEvent* event);
  // change the symbol list for <color> to use the symbol with index
  // <symbolIndex>, and emit symbolChanged
  bool updatePatternImage(const triC& color, int symbolIndex);
  bool backHistory() const {return !backHistory_.isEmpty(); }
  bool forwardHistory() const { return !forwardHistory_.isEmpty(); }
  // go forward one in the history list
  void moveHistoryForward();
  // go back one in the history list
  void moveHistoryBack();
  void writeSymbolHistory(QDomDocument* doc, QDomElement* appendee) const;
  void appendHistoryList(const QList<historyIndex>& list,
                         QDomDocument* doc, QDomElement* appendee) const;
  void updateHistory(const QDomElement& xmlHistory);

 private:
  void addToHistory(const historyIndex& historyRecord);
  // rewrite colorSquares_ using symbolDimension_ for the square size
  void generateColorSquares();

 signals:
  // let users know that the symbol for <color> is now <symbol>
  void symbolChanged(QRgb color, const QPixmap& symbol);

 private:
  QString imageName_;
  // the initial square image square dimension
  const int squareDimension_;
  // the initial symbol dimension
  const int baseSymbolDim_;
  // the current symbol dimension
  int symbolDimension_;
  const QImage squareImage_;
  const QVector<triC> colors_;
  // handles construction and choice of symbols
  symbolChooser symbolChooser_;
  QHash<QRgb, QPixmap> colorSquares_;
  bool viewingSquareImage_; // is the image on screen the square image?
  // the most recent edit sits on the back of backHistory_
  QList<historyIndex> backHistory_;
  QList<historyIndex> forwardHistory_;
};

// Copied from qshareddata.h - normally you can't put a QObject in a
// QExplicitlySharedDataPointer because a call to detach on the pointer
// would require a copy, which isn't allowed by QObejct.  But I'm not
// calling detach, so... remove detach and use it.
// Instead of having patternImageContainer derive from QSharedData
// (which it can't do since it's already deriving from QObject), which
// is (currently) just a public QAtomicInt, we just include a public
// QAtomicInt in patternImageContainer.
//template <class T> class QExplicitlySharedDataPointer
class patternImagePtr {
 public:
  inline patternImagePtr() { d = 0; }
  explicit patternImagePtr(patternImageContainer *data)
    : d(data) { if (d) d->ref.ref(); }
  inline patternImagePtr(const patternImagePtr& o)
    : d(o.d) { if (d) d->ref.ref(); }
  inline ~patternImagePtr() { if (d && !d->ref.deref()) delete d; }
  inline patternImageContainer& operator*() { return *d; }
  inline const patternImageContainer& operator*() const { return *d; }
  inline patternImageContainer* operator->() { return d; }
  inline patternImageContainer* operator->() const { return d; }
  inline patternImageContainer* data() const { return d; }
  inline const patternImageContainer* constData() const { return d; }

  inline operator bool() const { return d != 0; }
  inline bool operator!() const { return !d; }
  inline bool operator==(const patternImagePtr& other) const {
    return d == other.d;
  }
  inline bool operator!=(const patternImagePtr &other) const {
    return d != other.d;
  }
  inline bool operator==(const patternImageContainer *ptr) const {
    return d == ptr;
  }
  inline bool operator!=(const patternImageContainer *ptr) const {
    return d != ptr;
  }

  inline patternImagePtr& operator=(const patternImagePtr& o) {
    if (o.d != d) {
      if (o.d)
        o.d->ref.ref();
      if (d && !d->ref.deref())
        delete d;
      d = o.d;
    }
    return *this;
  }
  inline patternImagePtr& operator=(patternImageContainer *o) {
    if (o != d) {
      if (o)
        o->ref.ref();
      if (d && !d->ref.deref())
        delete d;
      d = o;
    }
    return *this;
  }

 private:
  patternImageContainer *d;
};
// make patternImagePtr known to QVariant
Q_DECLARE_METATYPE(patternImagePtr);

#endif

