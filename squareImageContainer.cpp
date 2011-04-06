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

#include "squareImageContainer.h"

#include <QtGui/QPainter>

#include "dmcList.h"
#include "imageProcessing.h"
#include "grid.h"
#include "xmlUtility.h"
#include "rareColorsDialog.h"
#include "symbolChooser.h"

mutableSquareImageContainer::mutableSquareImageContainer(const QString& name,
                                           const QVector<triC>& colors,
                                           const QImage& image,
                                           int dimension, bool dmc)
  : squareImageContainer(name, image.size(), dmc), image_(image),
    colors_(colors), currentlyDMC_(dmc), originalDimension_(dimension),
    widthSquareCount_(image.width()/dimension),
    heightSquareCount_(image.height()/dimension),
    colorListCheckNeeded_(false) {

  if ((!colors.empty()) &&
     colors.size() <= symbolChooser::maxNumberOfSymbols()) {
    valid_ = true;
    invalidColorCount_ = 0;
  }
  else {
    valid_ = false;
    invalidColorCount_ = colors.size();
    setColorList(QVector<triC>());
  }
  squareImageContainer::setScaledSize(QSize(0, 0));
}

QVector<triC> mutableSquareImageContainer::checkColorList() {

  if (colorListCheckNeeded_) {
    const grid gridImage(image_);
    const QVector<triC> colorsToRemove = ::findColors(gridImage, colors_);
    removeColors(colorsToRemove);
    colorListCheckNeeded_ = false;
    return colorsToRemove;
  }
  else {
    return QVector<triC>();
  }
}

void mutableSquareImageContainer::removeColor(const triC& color) {

  removeColorNoDmcUpdate(color);
  currentlyDMC_ = currentlyDMC_ ? true : ::colorsAreDmc(colors_);
}

void mutableSquareImageContainer::removeColorNoDmcUpdate(const triC& color) {

  const int foundIndex = colors_.indexOf(color);
  if (foundIndex) {
    colors_.remove(foundIndex);
  }
}

dockListUpdate mutableSquareImageContainer::
performDetailing(const QImage& originalImage,
                 const QList<pixel>& detailSquares,
                 int numColors, bool dmcOnly) {

  if (detailSquares.empty()) {
    qWarning() << "Detail squares empty" << numColors;
    return dockListUpdate(QVector<triC>());
  }
  QVector<historyPixel> history;
  //// start the history by collecting the old pixels
  for (QList<pixel>::const_iterator it = detailSquares.constBegin(),
         end = detailSquares.constEnd(); it != end; ++it) {
    // we'll have to go back later and insert the new color for this pixel
    history.push_back(historyPixel(*it));
  }

  //// do the processing
  const QVector<triC> colors =
    ::chooseColors(originalImage, detailSquares, originalDimension_,
                   numColors, dmcOnly);
  // this paints over our squareDetail marks (that's good)
  ::segment(originalImage, &image_, detailSquares, originalDimension_,
            colors);
  // median returns colors in the same order as detailSquares lists squares
  const QVector<triC> newColors = ::median(&image_, originalImage,
                                           detailSquares, history,
                                           originalDimension_);

  //// complete the history
  QVector<triC> colorsToAdd; // just return the new ones
  // iterating over both history and newColors
  QVector<historyPixel>::iterator historyI = history.begin();
  QVector<triC>::const_iterator colorI = newColors.begin();
  for (; historyI != history.end(); ++historyI, ++colorI) {
    (*historyI).setNewColor((*colorI).qrgb());
    if (!colors_.contains(*colorI)) {
      (*historyI).setNewColorIsNew(true);
      colorsToAdd.push_back(*colorI);
      colors_.push_back(*colorI);
    }
  }
  colorListCheckNeeded_ = true;
  addToHistory(historyItemPtr(new detailHistoryItem(history)));
  return dockListUpdate(colorsToAdd);
}

void mutableSquareImageContainer::addColorNoDmcUpdate(const triC& color) {

  if (!colors_.contains(color)) {
    colors_.push_back(color);
  }
}

bool mutableSquareImageContainer::addColor(const triC& color) {

  if (!colors_.contains(color)) {
    colors_.push_back(color);
    currentlyDMC_ = currentlyDMC_ ? ::colorIsDmc(color) : false;
    return true;
  }
  else {
    return false;
  }
}

void mutableSquareImageContainer::removeColors(const QVector<triC>& colors) {

  for (QVector<triC>::const_iterator it = colors.begin(), end = colors.end();
       it != end; ++it) {
    removeColorNoDmcUpdate(*it);
  }
  currentlyDMC_ = currentlyDMC_ ? true : ::colorsAreDmc(colors_);
}

void mutableSquareImageContainer::addColors(const QVector<triC>& colors) {

  for (int i = 0, size = colors.size(); i < size; ++i) {
    addColorNoDmcUpdate(colors[i]);
  }
  currentlyDMC_ = !currentlyDMC_ ? false : ::colorsAreDmc(colors);
}

dockListUpdate mutableSquareImageContainer::changeColor(QRgb oldColor,
                                                        QRgb newColor) {

  if (oldColor == newColor) {
    return dockListUpdate();
  }
  const QVector<pairOfInts> changedSquares =
    ::changeColor(&image_, oldColor, newColor, originalDimension_);
  if (!changedSquares.empty()) {
    const bool colorAdded = addColor(newColor);
    removeColor(oldColor);
    addToHistory(historyItemPtr(new changeAllHistoryItem(oldColor, newColor,
                                                         colorAdded,
                                                         changedSquares)));
    return dockListUpdate(newColor, colorAdded, oldColor);
  }
  else {
    return dockListUpdate();
  }
}

dockListUpdate mutableSquareImageContainer::fillRegion(int x, int y,
                                                       QRgb newColor) {

  const triC oldColor = image_.pixel(x, y);
  if (oldColor == newColor) {
    return dockListUpdate();
  }
  const QVector<pairOfInts> coordinates =
    ::fillRegion(&image_, x, y, newColor, originalDimension_);
  //qDebug() << "fill time:" << double(t.elapsed())/1000.;

  const bool colorAdded = addColor(newColor);
  addToHistory(historyItemPtr(new fillRegionHistoryItem(oldColor.qrgb(),
                                                        newColor,
                                                        colorAdded,
                                                        coordinates)));
  colorListCheckNeeded_ = true;
  return dockListUpdate(newColor, colorAdded);
}

dockListUpdate mutableSquareImageContainer::
commitChangeOneDrag(const QSet<pairOfInts>& squares, QRgb newColor) {

  const bool colorAdded = addColor(newColor);
  QVector<pixel> historyPixels;
  for (QSet<pairOfInts>::const_iterator it = squares.begin(),
          end = squares.end(); it != end; ++it) {
    const int x = (*it).x() * originalDimension_;
    const int y = (*it).y() * originalDimension_;
    const QRgb thisColor = image_.pixel(x, y);
    historyPixels.push_back(pixel(thisColor, pairOfInts(x, y)));
    //    ::changeOneBlock(&image_, x, y, newColor, originalDimension_);
  }
  ::changeBlocks(&image_, historyPixels, newColor, originalDimension_);
  QExplicitlySharedDataPointer<historyItem>(new changeOneHistoryItem(newColor, colorAdded, historyPixels));
  addToHistory(historyItemPtr(new changeOneHistoryItem(newColor, colorAdded,
                                                       historyPixels)));
  colorListCheckNeeded_ = true;
  return dockListUpdate(newColor, colorAdded);
}

dockListUpdate mutableSquareImageContainer::moveHistoryForward() {

  if (!forwardHistory_.empty()) {
    backHistory_.push_back(forwardHistory_.front());
    forwardHistory_.pop_front();
    return backHistory_.back()->performHistoryEdit(this, H_FORWARD);
  }
  else {
    return dockListUpdate();
  }
}

dockListUpdate mutableSquareImageContainer::moveHistoryBack() {

  if (!backHistory_.empty()) {
    const dockListUpdate update =
      backHistory_.back()->performHistoryEdit(this, H_BACK);
    forwardHistory_.push_front(backHistory_.back());
    backHistory_.pop_back();
    return update;
  }
  else {
    return dockListUpdate();
  }
}

QDomDocument mutableSquareImageContainer::backImageHistoryXml() const {

  QDomDocument doc;
  QDomElement root(doc.createElement("square_history"));
  doc.appendChild(root);
  QDomElement history(doc.createElement("backward_history"));
  root.appendChild(history);
  history.setAttribute("count", backHistory_.size());
  for (int i = 0, size = backHistory_.size(); i < size; ++i) {
    backHistory_[i]->toXml(&doc, &history);
  }
  return doc;
}

void mutableSquareImageContainer::
writeImageHistory(QDomDocument* doc, QDomElement* appendee) const {

  if (backHistory_.empty() && forwardHistory_.empty()) {
    return;
  }

  QDomElement history(doc->createElement("history"));
  appendee->appendChild(history);

  if (!backHistory_.empty()) {
    QDomElement back(doc->createElement("backward_history"));
    history.appendChild(back);
    back.setAttribute("count", backHistory_.size());
    for (int i = 0, size = backHistory_.size(); i < size; ++i) {
      backHistory_[i]->toXml(doc, &back);
    }
  }

  if (!forwardHistory_.empty()) {
    QDomElement forward(doc->createElement("forward_history"));
    history.appendChild(forward);
    forward.setAttribute("count", forwardHistory_.size());
    for (int i = 0, size = forwardHistory_.size(); i < size; ++i) {
      forwardHistory_[i]->toXml(doc, &forward);
    }
  }
}

void mutableSquareImageContainer::
updateImageHistory(const QDomElement& element) {

  // we put everything on forwardHistory_ and then moveHistoryForward()
  // the number of back_history items
  QDomElement backElement(element.firstChildElement("backward_history"));
  QDomNodeList backList(backElement.elementsByTagName("history_item"));
  for (int i = 0, size = backList.size(); i < size; ++i) {
    forwardHistory_.
      push_back(historyItem::xmlToHistoryItem(backList.item(i).toElement()));
  }

  QDomElement forwardElement(element.firstChildElement("forward_history"));
  QDomNodeList forwardList(forwardElement.elementsByTagName("history_item"));
  for (int i = 0, size = forwardList.size(); i < size; ++i) {
    forwardHistory_.
      push_back(historyItem::xmlToHistoryItem(forwardList.
                                              item(i).toElement()));
  }

  // move forward over the back history items
  for (int i = 0, size = backList.size(); i < size; ++i) {
    moveHistoryForward();
  }
}

void mutableSquareImageContainer::rewindAndClearHistory() {

  for (int i = 0, size = backHistory_.size(); i < size; ++i) {
    moveHistoryBack();
  }
  forwardHistory_.clear();
}

dockListUpdate mutableSquareImageContainer::replaceRareColors() {

  QHash<QRgb, int> countHash;
  ::colorCounts(image_, originalDimension_, &countHash);

  rareColorsDialog countDialog(countHash);
  const int dialogReturnCode = countDialog.exec();
  QList<QRgbPair> pairs = countDialog.colorsToChange();
  if (dialogReturnCode == QDialog::Accepted && pairs.size() > 0) {
    QList<colorChange> changeHistories;
    QVector<triC> oldColors;
    for (int i = 0, size = pairs.size(); i < size; ++i) {
      const QRgb oldColor = pairs[i].first;
      oldColors.push_back(triC(oldColor));
      const QRgb newColor = pairs[i].second;
      const QVector<pairOfInts> changedSquares =
        ::changeColor(&image_, oldColor, newColor, originalDimension_);
      if (!changedSquares.empty()) {
        removeColor(oldColor);
        changeHistories.push_back(colorChange(oldColor, newColor,
                                              changedSquares));
      }
    }
    addToHistory(historyItemPtr(new rareColorsHistoryItem(changeHistories)));
    return dockListUpdate(oldColors, true);
  }
  else {
    return dockListUpdate();
  }
}

QSize mutableSquareImageContainer::zoom(bool zoomIn) {

  const int curSquareSize = scaledWidth()/widthSquareCount_;
  const int delta = zoomIn ? 1 : -1;
  const int newSquareSize = qMax(curSquareSize + delta, 2);
  const int newWidth = newSquareSize * widthSquareCount_;
  const int newHeight = newSquareSize * heightSquareCount_;
  const QSize newSize(newWidth, newHeight);
  imageContainer::setScaledSize(newSize);
  return newSize;
}

QSize mutableSquareImageContainer::setScaledSize(const QSize& sizeHint) {

  if (sizeHint.isEmpty()) {
    imageContainer::setScaledSize(sizeHint);
    return sizeHint;
  }

  // adjust sizeHint (smaller) to make the dimensions multiples of a
  // square size
  // yup - we ignore the sizeHint height
  if (scaledSize().isEmpty()) {
    return setScaledWidth(sizeHint.width());
  }
  else {
    return scaledSize();
  }
}

QSize mutableSquareImageContainer::setScaledWidth(int widthHint) {

  if (scaledSize().isEmpty()) {
    const int newSquareSize = qMax(widthHint/widthSquareCount_, 2);
    const int newWidth = newSquareSize * widthSquareCount_;
    const int newHeight = newSquareSize * heightSquareCount_;
    const QSize newSize(newWidth, newHeight);
    imageContainer::setScaledSize(newSize);
    return newSize;
  }
  else {
    return scaledSize();
  }
}

QSize mutableSquareImageContainer::setScaledHeight(int heightHint) {

  if (scaledSize().isEmpty()) {
    const int newSquareSize = qMax(heightHint/heightSquareCount_, 2);
    const int newWidth = newSquareSize * widthSquareCount_;
    const int newHeight = newSquareSize * heightSquareCount_;
    const QSize newSize(newWidth, newHeight);
    imageContainer::setScaledSize(newSize);
    return newSize;
  }
  else {
    return scaledSize();
  }
}

QImage mutableSquareImageContainer::scaledImage() const {

  QHash<triC, QImage> colorSquares;
  const QVector<triC>& squareColors = colors();
  const int curDimension = scaledDimension();
  const grid baseImage(image(), originalDimension_);
  QImage squareImage(curDimension, curDimension, QImage::Format_RGB32);
  for (int i = 0, size = squareColors.size(); i < size; ++i) {
    const triC& thisImageColor = squareColors[i];
    squareImage.fill(thisImageColor.qrgb());
    colorSquares[thisImageColor] = squareImage;
  }

  QImage returnImage(scaledSize(), QImage::Format_RGB32);
  QPainter painter(&returnImage);
  for (int yBox = 0; yBox < heightSquareCount_; ++yBox) {
    for (int xBox = 0; xBox < widthSquareCount_; ++xBox) {
      const triC& thisSquareColor =
        baseImage(xBox*originalDimension_, yBox*originalDimension_);
      painter.drawImage(QPoint(xBox*curDimension, yBox*curDimension),
                        colorSquares[thisSquareColor]);

    }
  }
  return returnImage;
}

QSize immutableSquareImageContainer::setScaledWidth(int widthHint) {

  const int newHeight =
    qRound(originalHeight() * static_cast<qreal>(widthHint)/originalWidth());
  const QSize newSize(widthHint, newHeight);
  imageContainer::setScaledSize(newSize);
  return newSize;
}

QSize immutableSquareImageContainer::setScaledHeight(int heightHint) {

  const int newWidth =
    qRound(originalWidth() * static_cast<qreal>(heightHint)/originalHeight());
  const QSize newSize(newWidth, heightHint);
  imageContainer::setScaledSize(newSize);
  return newSize;
}
