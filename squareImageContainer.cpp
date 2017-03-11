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

#include "colorLists.h"
#include "imageProcessing.h"
#include "grid.h"
#include "xmlUtility.h"
#include "rareColorsDialog.h"
#include "symbolChooser.h"
#include "versionProcessing.h"

mutableSquareImageContainer::mutableSquareImageContainer(const QString& name,
                                           const QVector<triC>& colors,
                                           const QImage& image,
                                           int dimension, flossType type)
  : squareImageContainer(name, image.size(), type), image_(image),
    toolFlossType_(flossVariable), originalDimension_(dimension),
    widthSquareCount_(image.width()/dimension),
    heightSquareCount_(image.height()/dimension),
    colorListCheckNeeded_(false) {

  if ((!colors.empty()) &&
     colors.size() <= symbolChooser::maxNumberOfSymbols()) {
    valid_ = true;
    invalidColorCount_ = 0;
    for (int i = 0, size = colors.size(); i < size; ++i) {
      flossColors_.push_back(flossColor(colors[i], type));
    }
  }
  else {
    valid_ = false;
    invalidColorCount_ = colors.size();
    flossColors_ = QVector<flossColor>();
  }
  squareImageContainer::setScaledSize(QSize(0, 0));
}

QVector<triC> mutableSquareImageContainer::checkColorList() {

  if (colorListCheckNeeded_) {
    const grid gridImage(image_);
    const QVector<triC> colorsToRemove = ::findColors(gridImage, colors());
    removeColors(colorsToRemove);
    colorListCheckNeeded_ = false;
    return colorsToRemove;
  }
  else {
    return QVector<triC>();
  }
}

flossColor mutableSquareImageContainer::removeColor(const triC& color) {

  const int removeIndex = flossColors_.indexOf(flossColor(color));
  if (removeIndex != -1) {
    const flossColor returnColor = flossColors_[removeIndex];
    flossColors_.remove(removeIndex);
    return returnColor;
  }
  else {
    return flossColor(color);
  }
}

dockListUpdate mutableSquareImageContainer::
performDetailing(const QImage& originalImage,
                 const QList<pixel>& detailSquares,
                 int numColors, flossType type) {

  if (detailSquares.empty()) {
    qWarning() << "Detail squares empty" << numColors;
    return dockListUpdate(QVector<flossColor>());
  }
  QVector<historyPixel> history;
  //// start the history by collecting the old pixels
  for (QList<pixel>::const_iterator it = detailSquares.constBegin(),
         end = detailSquares.constEnd(); it != end; ++it) {
    // we'll have to go back later and insert the new color for this pixel
    history.push_back(historyPixel(*it));
  }

  //// do the processing
  colorTransformerPtr transformer =
    colorTransformer::createColorTransformer(type);
  const QVector<triC> colors =
    ::chooseColors(originalImage, detailSquares, originalDimension_,
                   numColors, transformer);
  // this paints over our squareDetail marks (that's good)
  ::segment(originalImage, &image_, detailSquares, originalDimension_,
            colors);
  // median returns colors in the same order as detailSquares lists squares
  const QVector<triC> newColors = ::median(&image_, originalImage,
                                           detailSquares, history,
                                           originalDimension_);

  //// complete the history
  QVector<flossColor> colorsToAdd; // just return the new ones
  for (int i = 0, size = history.size(); i < size; ++i) {
    const triC thisColor(newColors[i]);
    history[i].setNewColor(thisColor.qrgb());
    const flossColor thisFlossColor(thisColor, type);
    if (!flossColors_.contains(thisFlossColor)) {
      history[i].setNewColorIsNew(true);
      colorsToAdd.push_back(thisFlossColor);
      flossColors_.push_back(thisFlossColor);
    }
  }
  colorListCheckNeeded_ = true;
  addToHistory(historyItemPtr(new detailHistoryItem(history, type)));
  return dockListUpdate(colorsToAdd);
}

bool mutableSquareImageContainer::addColor(const flossColor& color) {

  if (!flossColors_.contains(color)) {
    const addSquareColorVersionPtr addVersion =
      versionProcessor::processor()->addSquareColor();
    const flossColor versionColor = addVersion->transform(color);
    flossColors_.push_back(versionColor);
    return true;
  }
  else {
    return false;
  }
}

void mutableSquareImageContainer::
addColors(const QVector<flossColor>& colors) {

  for (int i = 0, size = colors.size(); i < size; ++i) {
    addColor(colors[i]);
  }
}

void mutableSquareImageContainer::removeColors(const QVector<triC>& colors) {

  for (int i = 0, size = colors.size(); i < size; ++i) {
    removeColor(colors[i]);
  }
}

dockListUpdate
mutableSquareImageContainer::changeColor(QRgb oldColor,
                                         flossColor newFlossColor) {

  const QRgb newColor = newFlossColor.color().qrgb();
  if (oldColor == newColor) {
    return dockListUpdate();
  }
  const QVector<pairOfInts> changedSquares =
    ::changeColor(&image_, oldColor, newColor, originalDimension_);
  if (!changedSquares.empty()) {
    const bool colorAdded = addColor(newFlossColor);
    const flossColor oldFlossColor = removeColor(oldColor);
    addToHistory(historyItemPtr(new changeAllHistoryItem(oldFlossColor,
                                                         newFlossColor,
                                                         colorAdded,
                                                         changedSquares)));
    return dockListUpdate(newFlossColor, colorAdded, oldColor);
  }
  else {
    return dockListUpdate();
  }
}

dockListUpdate mutableSquareImageContainer::fillRegion(int x, int y,
                                                       flossColor newColor) {

  const triC oldColor = image_.pixel(x, y);
  if (newColor == oldColor) {
    return dockListUpdate();
  }
  const QVector<pairOfInts> coordinates =
    ::fillRegion(&image_, x, y, newColor.qrgb(), originalDimension_);

  const bool colorAdded = addColor(newColor);
  const flossColor oldFlossColor = getFlossColorFromColor(oldColor);
  addToHistory(historyItemPtr(new fillRegionHistoryItem(oldFlossColor,
                                                        newColor,
                                                        colorAdded,
                                                        coordinates)));
  colorListCheckNeeded_ = true;
  return dockListUpdate(newColor, colorAdded);
}

dockListUpdate mutableSquareImageContainer::
commitChangeOneDrag(const QSet<pairOfInts>& squares, flossColor newColor) {

  const QRgb newRgbColor = newColor.qrgb();
  const bool colorAdded = addColor(newColor);
  QVector<pixel> historyPixels;
  QVector<triC> pixelColors;
  for (QSet<pairOfInts>::const_iterator it = squares.begin(),
          end = squares.end(); it != end; ++it) {
    const int x = it->x() * originalDimension_;
    const int y = it->y() * originalDimension_;
    const QRgb thisColor = image_.pixel(x, y);
    pixelColors.push_back(thisColor);
    historyPixels.push_back(pixel(thisColor, pairOfInts(x, y)));
  }
  ::changeBlocks(&image_, historyPixels, newRgbColor, originalDimension_);
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

  ::appendTextElement(doc, "tool_floss_type", toolFlossType_.prefix(),
                      appendee);

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

  const QString toolFlossTypePrefix =
    ::getElementText(element, "tool_floss_type");
  if (!toolFlossTypePrefix.isNull()) {
    toolFlossType_ = flossType(toolFlossTypePrefix);
  }

  const QDomElement& historyElement = element.firstChildElement("history");
  // we put everything on forwardHistory_ and then moveHistoryForward()
  // the number of back_history items
  QDomElement backElement =
    historyElement.firstChildElement("backward_history");
  QDomNodeList backList(backElement.elementsByTagName("history_item"));
  for (int i = 0, size = backList.size(); i < size; ++i) {
    forwardHistory_.
      push_back(historyItem::xmlToHistoryItem(backList.item(i).toElement()));
  }

  QDomElement forwardElement =
    historyElement.firstChildElement("forward_history");
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
    QSet<flossColor> oldFloss;
    QVector<triC> oldColors;
    for (int i = 0, size = pairs.size(); i < size; ++i) {
      const QRgb oldColor = pairs[i].first;
      const triC oldTriColor(oldColor);
      oldColors.push_back(oldTriColor);
      oldFloss.insert(getFlossColorFromColor(oldTriColor));
      const QRgb newColor = pairs[i].second;
      const QVector<pairOfInts> changedSquares =
        ::changeColor(&image_, oldColor, newColor, originalDimension_);
      if (!changedSquares.empty()) {
        removeColor(oldColor);
        changeHistories.push_back(colorChange(oldColor, newColor,
                                              changedSquares));
      }
    }
    addToHistory(historyItemPtr(new rareColorsHistoryItem(changeHistories,
                                                          oldFloss)));
    return dockListUpdate(oldColors);
  }
  else {
    return dockListUpdate();
  }
}

QSize mutableSquareImageContainer::zoom(bool zoomIn) {

  const int curSquareSize = scaledWidth()/widthSquareCount_;
  const int delta = zoomIn ? 1 : -1;
  const int newSquareSize = qMax(curSquareSize + delta, 1);
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
    const int newSquareSize = qMax(widthHint/widthSquareCount_, 1);
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
    const int newSquareSize = qMax(heightHint/heightSquareCount_, 1);
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
  QImage squareImage(curDimension, curDimension, QImage::Format_RGB32);
  for (int i = 0, size = squareColors.size(); i < size; ++i) {
    const triC& thisImageColor = squareColors[i];
    squareImage.fill(thisImageColor.qrgb());
    colorSquares[thisImageColor] = squareImage;
  }

  const grid baseImage(image(), originalDimension_);
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

QVector<triC> mutableSquareImageContainer::colors() const {

  QVector<triC> returnColors;
  returnColors.reserve(flossColors_.size());
  for (int i = 0, size = flossColors_.size(); i < size; ++i) {
    returnColors.push_back(flossColors_[i].color());
  }
  return returnColors;
}

flossColor 
mutableSquareImageContainer::getFlossColorFromColor(const triC& color) const {

  const int foundIndex = flossColors_.indexOf(flossColor(color));
  if (foundIndex != -1) {
    return flossColors_[foundIndex];
  }
  else {
    return flossColor(color);
  }
}
