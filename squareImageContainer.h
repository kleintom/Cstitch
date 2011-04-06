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

#ifndef SQUAREIMAGECONTAINER_H
#define SQUAREIMAGECONTAINER_H

#include <QtXml/QDomDocument>

#include "imageContainer.h"
#include "squareDockTools.h"
#include "squareToolHistories.h"

class squareImageContainer;
typedef QExplicitlySharedDataPointer<squareImageContainer> squareImagePtr;

// squareImageContainer extends imageContainer by providing a pure abstract
// interface for functionality required by a squared image
class squareImageContainer : public imageContainer {

 public:
  squareImageContainer(const QString& name, const QSize initialImageSize,
                       bool dmc)
   : imageContainer(name, initialImageSize, dmc) {}
  // overrides imageContainer; provided there as an alternative to
  // dynamic_casting, which was being used as a workaround for QT's lack
  // of template support
  const squareImageContainer* squareContainer() const { return this; }
  squareImageContainer* squareContainer() { return this; }
  // return true if the current color list is dmc colors only
  virtual bool currentlyDMC() const = 0;
  // return the original dimension of the image's squares
  virtual int originalDimension() const = 0;
  // return the current (scaled) dimension of the image's squares
  virtual int scaledDimension() const = 0;
  // return the number of squares in the horizontal direction
  virtual int xSquareCount() const = 0;
  virtual int ySquareCount() const = 0;
  // return true if the image is valid (where valid is defined by derived
  // classes)
  virtual bool isValid() const = 0;
  // return the number of colors on the image's color list
  virtual int numColors() const = 0;
  // remove the given <colors> from the image's color list and update
  // currentlyDMC() accordingly
  virtual void removeColors(const QVector<triC>& colors) = 0;
  // return true if the back history is non-empty
  virtual bool backHistory() const = 0;
  // return true if the forward history is non-empty
  virtual bool forwardHistory() const = 0;
  // move forward one on the history list
  // return a dockListUpdate for the history item performed
  virtual dockListUpdate moveHistoryForward() = 0;
  // move back one on the history list
  // return a dockListUpdate for the history item performed
  virtual dockListUpdate moveHistoryBack() = 0;
  // return true if the image and its color list should be checked to make
  // sure they're in sync
  virtual bool colorListCheckNeeded() const = 0;
  // compare the image and the color list to make sure they give the same
  // list of colors
  // return the colors on the list that aren't on the image
  virtual QVector<triC> checkColorList() = 0;
  // change every occurrence of <oldColor> in the image to <newColor>
  // return a dockListUpdate for the change and update currentlyDMC()
  virtual dockListUpdate changeColor(QRgb oldColor, QRgb newColor) = 0;
  // change <squares> to have <newColor> in the image
  virtual dockListUpdate commitChangeOneDrag(const QSet<pairOfInts>& squares,
                                             QRgb newColor) = 0;
  // fill the region containing (<x>, <y>) with <newColor>
  virtual dockListUpdate fillRegion(int x, int y, QRgb newColor) = 0;
  // perform detailing on <detailSquares> from the container's image,
  // using <originalImage> for reference colors and
  // choosing at most <numColors> colors for the detail squares,
  // making those colors all dmc if <dmcOnly>
  virtual dockListUpdate
    performDetailing(const QImage& originalImage,
                     const QList<pixel>& detailSquares,
                     int numColors, bool dmcOnly) = 0;
  // return the current backward history as xml
  virtual QDomDocument backImageHistoryXml() const = 0;
  // append the entire edit history as xml to <appendee>
  virtual void writeImageHistory(QDomDocument* doc,
                                 QDomElement* appendee) const = 0;
  // restore this image's history from <element>, running back history
  // if any
  virtual void updateImageHistory(const QDomElement& element) = 0;
  // undo back history and then clear both histories
  virtual void rewindAndClearHistory() = 0;
  // setScaledSize for (mutable) square images is a set once affair; this
  // allows scaled size to be set again
  virtual void resetZoom() = 0;
  virtual QSize setScaledWidth(int widthHint) = 0;
  virtual QSize setScaledHeight(int heightHint) = 0;
  virtual dockListUpdate replaceRareColors() = 0;
};

// a mutableSquareImageContainer copies in its image so that it can be
// altered by the container
class mutableSquareImageContainer : public squareImageContainer {

  // historyItem classes perform history updates on this class's data
  friend dockListUpdate
    changeAllHistoryItem::performHistoryEdit(mutableSquareImageContainer* ,
                                             historyDirection ) const;
  friend dockListUpdate
    changeOneHistoryItem::performHistoryEdit(mutableSquareImageContainer* ,
                                             historyDirection ) const;
  friend dockListUpdate
    fillRegionHistoryItem::performHistoryEdit(mutableSquareImageContainer* ,
                                              historyDirection ) const;
  friend dockListUpdate
    detailHistoryItem::performHistoryEdit(mutableSquareImageContainer* ,
                                          historyDirection ) const;
  friend dockListUpdate
    rareColorsHistoryItem::performHistoryEdit(mutableSquareImageContainer* ,
                                              historyDirection ) const;

 public:
  mutableSquareImageContainer(const QString& name,
                              const QVector<triC>& colors,
                              const QImage& image, int dimension, bool dmc);
  const QImage& image() const { return image_; }
  const QVector<triC>& colors() const { return colors_; }
  bool currentlyDMC() const { return currentlyDMC_; }
  int originalDimension() const { return originalDimension_; }
  int scaledDimension() const { return scaledWidth()/widthSquareCount_; }
  int xSquareCount() const { return widthSquareCount_; }
  int ySquareCount() const { return heightSquareCount_; }
  bool isValid() const { return valid_; }
  int numColors() const {
    return valid_ ? colors_.size() : invalidColorCount_;
  }
  void removeColors(const QVector<triC>& colors);
  bool backHistory() const { return !backHistory_.isEmpty(); }
  bool forwardHistory() const { return !forwardHistory_.isEmpty(); }
  dockListUpdate moveHistoryForward();
  dockListUpdate moveHistoryBack();
  bool colorListCheckNeeded() const { return colorListCheckNeeded_; }
  QVector<triC> checkColorList();
  dockListUpdate changeColor(QRgb oldColor, QRgb newColor);
  dockListUpdate commitChangeOneDrag(const QSet<pairOfInts>& squares,
                                     QRgb newColor);
  dockListUpdate fillRegion(int x, int y, QRgb newColor);
  dockListUpdate performDetailing(const QImage& originalImage,
                                  const QList<pixel>& detailSquares,
                                  int numColors, bool dmcOnly);
  QDomDocument backImageHistoryXml() const;
  void writeImageHistory(QDomDocument* doc, QDomElement* appendee) const;
  void updateImageHistory(const QDomElement& element);
  void rewindAndClearHistory();
  // increases or decreases the scaled square size by one
  QSize zoom(bool zoomIn);
  // setScaledSize for (mutable) square images is a set once affair; this
  // allows scaled size to be set again
  void resetZoom() { imageContainer::setScaledSize(QSize(0, 0)); }
  // set scaled size to the largest true square size that is <= <sizeHint>
  // return the new size
  QSize setScaledSize(const QSize& sizeHint);
  // set scaled size to the largest true square width that
  // is <= <widthHint>; return the new size
  QSize setScaledWidth(int widthHint);
  // set scaled size to the largest true square height that is
  // <= heightHint; return the new size
  QSize setScaledHeight(int heightHint);
  dockListUpdate replaceRareColors();
  bool isOriginal() const { return false; }
  QImage scaledImage() const;

 private:
  void setColorList(const QVector<triC>& colorList) {
    colors_ = colorList;
  }
  void drawDetail(int xStart, int yStart, const QColor& color);
  void addColors(const QVector<triC>& colors);
  // add <color> to colors_
  // caller should adjust currentlyDMC_ accordingly
  void addColorNoDmcUpdate(const triC& color);
  // add <color> to colors_ and update currentlyDMC_
  bool addColor(const triC& color);
  // remove <color> from colors_
  // caller should adjust currentlyDMC_ accordingly
  void removeColorNoDmcUpdate(const triC& color);
  // remove <color> from colors_ and adjust currentlyDMC_ accordingly
  void removeColor(const triC& color);
  void addToHistory(const historyItemPtr& ptr) {
    backHistory_.push_back(ptr);
    forwardHistory_.clear();
  }

 private:
  QImage image_; // the square image (at its original size)
  QVector<triC> colors_;
  bool currentlyDMC_; // true if colors_ contains only dmc colors
  const int originalDimension_; // square dimension
  // for convenience: the number of horizontal and vertical squares
  const int widthSquareCount_;
  const int heightSquareCount_;
  // the most recently performed tool action sits on the back of
  // backHistory
  QList<historyItemPtr> backHistory_;
  QList<historyItemPtr> forwardHistory_;
  // valid_ if colors_.size() <= numSymbols && > 0
  bool valid_;
  // set only if valid = false, in which case colors should be set to 0
  int invalidColorCount_;
  // true if the image and the color list should be compared to make sure
  // there aren't colors on the color list but not on the image (which can
  // happen, for example, if the user paints over all of one color with a
  // color that already existed)
  bool colorListCheckNeeded_;
};

// immutableSquareImageContainer holds a _const_ reference to an image
// (whence most of its squareImageContainer interface consists of noops)
class immutableSquareImageContainer : public squareImageContainer {

 public:
  immutableSquareImageContainer(const QString& name, const QImage& image)
    : squareImageContainer(name, image.size(), false),
    image_(image), colors_(QVector<triC>(0)) { }
  const QImage& image() const { return image_; }
  const QVector<triC>& colors() const { return colors_; }
  bool currentlyDMC() const { return false; }
  int originalDimension() const { return 1; }
  int scaledDimension() const { return 1; }
  int xSquareCount() const { return originalWidth(); }
  int ySquareCount() const { return originalHeight(); }
  bool isValid() const { return false; }
  int numColors() const { return 0; }
  void removeColors(const QVector<triC>& ) { return; }
  bool backHistory() const { return false; }
  bool forwardHistory() const { return false; }
  dockListUpdate moveHistoryForward() { return dockListUpdate(); }
  dockListUpdate moveHistoryBack() { return dockListUpdate(); }
  bool colorListCheckNeeded() const { return false; }
  QVector<triC> checkColorList() { return QVector<triC>(); }
  dockListUpdate changeColor(QRgb , QRgb ) {
    return dockListUpdate();
  }
  dockListUpdate commitChangeOneDrag(const QSet<pairOfInts>& ,
                                     QRgb ) {
    return dockListUpdate();
  }
  dockListUpdate fillRegion(int , int , QRgb ) {
    return dockListUpdate();
  }
  dockListUpdate performDetailing(const QImage& , const QList<pixel>& ,
                                  int , bool ) {
    return dockListUpdate();
  }
  QDomDocument backImageHistoryXml() const { return QDomDocument(); }
  void writeImageHistory(QDomDocument* , QDomElement* ) const { return; }
  void updateImageHistory(const QDomElement& ) { return; }
  void rewindAndClearHistory() { return; }
  QSize setScaledWidth(int widthHint);
  QSize setScaledHeight(int heightHint);
  void resetZoom() { }
  dockListUpdate replaceRareColors() { return dockListUpdate(); }
  bool isOriginal() const { return true; }

 private:
  const QImage& image_;
  const QVector<triC> colors_;
};

#endif
