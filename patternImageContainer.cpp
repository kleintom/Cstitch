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

#include "patternImageContainer.h"

#include <QtCore/QDebug>

#include <QtWidgets/QMessageBox>
#include <QMouseEvent>
#include <QPainter>

#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include "symbolDialog.h"
#include "patternWindow.h"
#include "imageUtility.h"
#include "patternDockWidget.h"
#include "xmlUtility.h"

extern const int MAX_SYMBOL_SIZE;
extern const int MIN_SYMBOL_SIZE;

QString historyIndex::toString() const {

  return "[" + ::rgbToString(color_) + "," +
    QString::number(oldIndex_) + "," + QString::number(newIndex_) + "]";
}

historyIndex::historyIndex(const QDomElement& xmlIndex) {

  QString item(xmlIndex.text());
  QRegExp regex("^\\[\\((\\d+),(\\d+),(\\d+)\\),(\\d+),(\\d+)\\]$");
  if (regex.indexIn(item) != -1) {
    QStringList matches = regex.capturedTexts();
    QList<int> matchInts;
    // convert strings to ints!
    // NOTE that this switches the indices down by one from matches
    for (int j = 1; j <= 5; ++j) {
      matchInts.push_back(matches[j].toInt());
    }
    color_ = triC(matchInts[0], matchInts[1], matchInts[2]);
    oldIndex_ = matchInts[3];
    newIndex_ = matchInts[4];
  }
}

patternImageContainer::patternImageContainer(const QImage& squareImage,
                                             const QString& imageName,
                                             int squareDimension,
                                             int baseSymbolDim,
                                             const QVector<flossColor>& colors)
  : ref(0), imageName_(imageName), squareDimension_(squareDimension),
    baseSymbolDim_(baseSymbolDim), symbolDimension_(baseSymbolDim),
    squareImage_(squareImage), flossColors_(colors),
    symbolChooser_(baseSymbolDim, patternImageContainer::colors()),
    viewingSquareImage_(false) {

  generateColorSquares();
}

void patternImageContainer::generateColorSquares() {

  QPixmap colorSquare(symbolDimension_, symbolDimension_);
  for (int i = 0, size = flossColors_.size(); i < size; ++i) {
    const triC thisColor = flossColors_[i].color();
    colorSquare.fill(thisColor.qc());
    colorSquares_[thisColor.qrgb()] = colorSquare;
  }
}

QImage patternImageContainer::patternImageCurSymbolSize() {

  const QHash<QRgb, QPixmap> symbols =
    symbolChooser_.getSymbols(symbolDimension_);
  const int xBoxes = squareImage_.width()/squareDimension_;
  const int yBoxes = squareImage_.height()/squareDimension_;
  QImage returnImage(xBoxes * symbolDimension_, yBoxes * symbolDimension_,
                     QImage::Format_RGB32);
  if (returnImage.isNull()) {
    qWarning() << "Empty Image in patternImageCurSymbolSize.";
    return QImage();
  }
  QPainter painter(&returnImage);
  painter.fillRect(0, 0, xBoxes * symbolDimension_,
                   yBoxes * symbolDimension_, QColor(255, 255, 255));
  QPixmap thisSymbol;
  for (int j = 0; j < yBoxes; ++j) {
    for (int i = 0; i < xBoxes; ++i) {
      thisSymbol =
        symbols[squareImage_.pixel(i * squareDimension_,
                                   j * squareDimension_)];
      if (!thisSymbol.isNull()) {
        painter.drawPixmap(i*symbolDimension_, j*symbolDimension_,
                           thisSymbol);
      }
      else {
        qWarning() << "Bad image in patternImage at" << i << j;
        // fill it with red to make it obvious
        QImage badSymbol(symbolDimension_, symbolDimension_,
                         QImage::Format_RGB32);
        badSymbol.fill(QColor(255, 0, 0).rgb());
        painter.drawImage(i*symbolDimension_, j*symbolDimension_,
                          badSymbol);
      }
    }
  }
  return returnImage;
}

bool patternImageContainer::changeSymbol(const triC& color) {

  const QVector<patternSymbolIndex> availableSymbols =
    symbolChooser_.symbolsAvailable(color, baseSymbolDim_);
  if (!availableSymbols.empty()) {
    const QPixmap originalSymbol = 
      symbolChooser_.getSymbol(color, baseSymbolDim_).symbol();
    symbolDialog dialog(symbolChooser_.symbolsAvailable(color,
                                                        baseSymbolDim_),
                        originalSymbol);
    const int returnCode = dialog.exec();
    if (returnCode == QDialog::Accepted) {
      const int oldIndex = symbolChooser_.colorIndex(color);
      const patternSymbolIndex newSymbolIndex = dialog.selectedSymbol();
      if (updatePatternImage(color, newSymbolIndex.index())) {
        addToHistory(historyIndex(oldIndex, newSymbolIndex.index(), color));
        return true;
      }
      else {
        return false;
      }
    }
    else {
      return false;
    }
  }
  else {
    QMessageBox::information(NULL, tr("No free symbols"),
     tr("All symbols are in use, so you cannot change any symbols."));
    return false;
  }
}

void patternImageContainer::setSymbolDimension(int dimension) {

  if (dimension > MAX_SYMBOL_SIZE) {
    dimension = MAX_SYMBOL_SIZE;
  }
  if (dimension < MIN_SYMBOL_SIZE) {
    dimension = MIN_SYMBOL_SIZE;
  }
  symbolDimension_ = dimension;
  symbolChooser_.setSymbolDimension(dimension);
  generateColorSquares();
}

QPixmap patternImageContainer::symbolNoBorder(const triC& color,
                                              int symbolDim) {

  return symbolChooser_.getSymbolNoBorder(color, symbolDim).symbol();
}

QHash<QRgb, QPixmap> patternImageContainer::symbols() {

  return symbolChooser_.getSymbols(symbolDimension_);
}

QHash<QRgb, QPixmap> patternImageContainer::symbolsWithBorder(int symbolDim, int colorBorderWidth) {

  return symbolChooser_.getSymbolsWithBorder(symbolDim, colorBorderWidth);
}

bool patternImageContainer::updatePatternImage(const triC& color,
                                               int symbolIndex) {

  // remove the old symbol and replace it with the symbol for symbolIndex
  const bool changedSymbol =
    symbolChooser_.changeSymbol(color, symbolIndex);
  emit symbolChanged(color.qrgb(),
                     symbolChooser_.getSymbol(color, baseSymbolDim_).
                     symbol());
  return changedSymbol;
}

bool patternImageContainer::mouseActivatedChangeSymbol(int eventImageWidth,
                                                       int eventImageHeight,
                                                       QMouseEvent* event) {
  const int x =
    event->x()*static_cast<qreal>(squareImage_.width())/eventImageWidth;
  const int y =
    event->y()*static_cast<qreal>(squareImage_.height())/eventImageHeight;
  const triC clickedColor = squareImage_.pixel(x, y);
  return changeSymbol(clickedColor);
}

void patternImageContainer::addToHistory(const historyIndex& historyRecord) {

  backHistory_.push_back(historyRecord);
  forwardHistory_.clear();
}

void patternImageContainer::moveHistoryForward() {

  if (!forwardHistory_.empty()) {
    backHistory_.push_back(forwardHistory_.front());
    forwardHistory_.pop_front();

    const historyIndex historyRecord = backHistory_.back();
    updatePatternImage(historyRecord.color(), historyRecord.newIndex());
  }
}

void patternImageContainer::moveHistoryBack() {

  if (!backHistory_.empty()) {
    const historyIndex historyRecord = backHistory_.back();
    updatePatternImage(historyRecord.color(), historyRecord.oldIndex());

    forwardHistory_.push_front(backHistory_.back());
    backHistory_.pop_back();
  }
}

void patternImageContainer::writeSymbolHistory(QDomDocument* doc,
                                               QDomElement* appendee) const {

  if (backHistory_.size() == 0 && forwardHistory_.size() == 0) {
    return;
  }
  QDomElement history(doc->createElement("symbol_history"));
  appendee->appendChild(history);
  if (!backHistory_.empty()) {
    QDomElement backHistory(doc->createElement("backward_history"));
    history.appendChild(backHistory);
    backHistory.setAttribute("count", backHistory_.size());
    appendHistoryList(backHistory_, doc, &backHistory);
  }
  if (!forwardHistory_.empty()) {
    QDomElement forwardHistory(doc->createElement("forward_history"));
    history.appendChild(forwardHistory);
    forwardHistory.setAttribute("count", forwardHistory_.size());
    appendHistoryList(forwardHistory_, doc, &forwardHistory);
  }
}

void patternImageContainer::appendHistoryList(const QList<historyIndex>& list,
                                              QDomDocument* doc,
                                              QDomElement* appendee) const {

  for (int i = 0, size = list.size(); i < size; ++i) {
    ::appendTextElement(doc, "history_item", list[i].toString(), appendee);
  }
}

void patternImageContainer::updateHistory(const QDomElement& xmlHistory) {

  QDomElement backHistory(xmlHistory.firstChildElement("backward_history"));
  QDomNodeList backList(backHistory.elementsByTagName("history_item"));
  for (int i = 0, size = backList.size(); i < size; ++i) {
    forwardHistory_.
      push_back(historyIndex(backList.item(i).toElement()));
  }

  QDomElement forwardElement(xmlHistory.firstChildElement("forward_history"));
  QDomNodeList forwardList(forwardElement.elementsByTagName("history_item"));
  for (int i = 0, size = forwardList.size(); i < size; ++i) {
    forwardHistory_.
      push_back(historyIndex(forwardList.item(i).toElement()));
  }

  // move forward over the back history items
  for (int i = 0, size = backList.size(); i < size; ++i) {
    moveHistoryForward();
  }
}

QVector<triC> patternImageContainer::colors() const {

  QVector<triC> returnColors;
  returnColors.reserve(flossColors_.size());
  for (int i = 0, size = flossColors_.size(); i < size; ++i) {
    returnColors.push_back(flossColors_[i].color());
  }
  return returnColors;
}
