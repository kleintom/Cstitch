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

#include "symbolChooser.h"

#include <QtCore/QDebug>

#include <QtGui/QFontDatabase>
#include <QtGui/QPainter>

#include "utility.h"
#include "imageProcessing.h"

extern const int MAX_NUM_SYMBOL_TYPES = 8;
QVector<QChar> symbolChooser::unicodeCharacters_ = QVector<QChar>();
QFont symbolChooser::unicodeFont_ = QFont();

symbolChooser::symbolChooser(int symbolDimension, QVector<triC> colors,
                             int borderDimension)
  : symbolDimension_(symbolDimension), borderDimension_(borderDimension) {

  initializeSymbolList();
  const int colorCount = colors.size();
  const int characterCount = unicodeCharacters_.size();
  // each createSymbolType* creates two types of symbols (light and dark)
  // from a given character, so numSymbolTypes is always even
  for (int i = 2; i <= MAX_NUM_SYMBOL_TYPES; i += 2) {
    numSymbolTypes_ = i;
    if (numSymbolTypes_ * characterCount >= colorCount) {
      break;
    }
  }
  darkIndex_ = stepIndex(0, numberOfSymbols(), 2);
  lightIndex_ = stepIndex(1, numberOfSymbols(), 2);

  // find the median intensity of the input colors
  const int size = colors.size();
  int middle;
  if (size%2 == 0) {
    middle = size/2;
  }
  else {
    middle = (size == 1) ? 0 : (size + 1)/2;
  }
  std::nth_element(colors.begin(), colors.begin() + middle, colors.end(),
                   triCIntensity());
  medianIntensity_ = colors[middle].intensity();
}

void symbolChooser::setSymbolDimension(int dimension) {

  if (dimension > 0) {
    symbolDimension_ = dimension;
  }
}

int symbolChooser::getNewIndex(const triC& color) {

  int newIndex = numberOfSymbols()/2;
  if (color.intensity() < medianIntensity_) {
    newIndex = darkIndex_.next();
    if (newIndex == numberOfSymbols()/2) {
      newIndex = lightIndex_.next();
    }
  }
  else {
    newIndex = lightIndex_.next();
    if (newIndex == numberOfSymbols()/2) {
      newIndex = darkIndex_.next();
    }
  }
  return newIndex;
}

patternSymbolIndex symbolChooser::getSymbol(const triC& color,
                                            int symbolDim) {

  const QRgb rgbColor = color.qrgb();
  symbolDimension_ = symbolDim;
  if (symbolMap_.contains(rgbColor)) {
    if (symbolMap_[rgbColor].borderWidth() == borderDimension_ &&
        symbolMap_[rgbColor].symbolDimension() == symbolDimension_) {
      return symbolMap_[rgbColor];
    }
    else { // right symbol, wrong size
      const int index = symbolMap_[rgbColor].index();
      patternSymbolIndex symbolIndex(createSymbol(index, rgbColor),
                                     index, borderDimension_,
                                     symbolDimension_);
      symbolMap_.insert(rgbColor, symbolIndex);
      return symbolIndex;
    }
  }
  else {
    const int newIndex = getNewIndex(color);
    patternSymbolIndex symbolIndex(createSymbol(newIndex, rgbColor),
                                   newIndex, borderDimension_,
                                   symbolDimension_);
    symbolMap_.insert(rgbColor, symbolIndex);
    return symbolIndex;
  }
}

QHash<QRgb, QPixmap> symbolChooser::getSymbols(const QVector<triC>& colors,
                                               int symbolDim) {

  symbolDimension_ = symbolDim;
  QHash<QRgb, QPixmap> returnMap;
  for (QVector<triC>::const_iterator it = colors.begin(), end = colors.end();
       it != end; ++it) {
    const QRgb thisColor = (*it).qrgb();
    returnMap.insert(thisColor, getSymbolCurDim(thisColor).symbol());
  }
  return returnMap;
}

QPixmap symbolChooser::createSymbol(int index, const triC& color) const {

  const int drawDimension = symbolDimension_ - 2*borderDimension_;
  QPixmap drawSymbol(drawDimension, drawDimension);
  drawSymbol.fill(Qt::white);
  if (index < numberOfSymbols()) {
    QPainter painter(&drawSymbol);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const int interval = 2*unicodeCharacters_.size();
    if (index < interval) {
      createSymbolType1(&painter, drawDimension, index);
    }
    else if (index >= interval && index < 2*interval) {
      index -= interval;
      createSymbolType2(&painter, drawDimension, index);
    }
    else if (index >= 2*interval && index < 3*interval) {
      index -= 2*interval;
      createSymbolType3(&painter, drawDimension, index);
    }
    else if (index >= 3*interval && index < 4*interval) {
      index -= 3*interval;
      createSymbolType4(&painter, drawDimension, index);
    }

    const QChar symbolChar = unicodeCharacters_[index/2];
    const QString symbolString(symbolChar);
    // find the right font size for our box size
    QFont font(unicodeFont_);
    ::setFontHeight(&font, drawDimension + 2);
    painter.setFont(font);
    painter.drawText(0, 0, drawDimension, drawDimension,
                     Qt::AlignCenter, symbolString);
  }
  else { // no symbols left, so just make it the original color
    QPainter painter(&drawSymbol);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0, 0, drawDimension, drawDimension, color.qc());
  }

  if (borderDimension_) {
    QPixmap newSymbol(symbolDimension_, symbolDimension_);
    newSymbol.fill(color.qrgb());
    QPainter painter(&newSymbol);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawPixmap(QPoint(borderDimension_, borderDimension_),
                       drawSymbol);
    return newSymbol;
  }
  else {
    return drawSymbol;
  }
}

void symbolChooser::createSymbolType1(QPainter* painter,
                                      int drawDimension, int index)  const {
  if (index % 2 == 0) {
    painter->fillRect(0, 0, drawDimension, drawDimension, Qt::black);
    const int innerDim = 2;
    painter->fillRect(QRect(innerDim, innerDim,
                            drawDimension - 2*innerDim,
                            drawDimension - 2*innerDim), Qt::white);
  }
}

void symbolChooser::createSymbolType2(QPainter* painter,
                                      int drawDimension, int index) const {
  painter->save();
  if (index % 2 == 0) {
    painter->fillRect(0, 0, drawDimension, drawDimension, Qt::black);
    painter->setBrush(Qt::white);
  }
  painter->drawEllipse(0, 0, drawDimension, drawDimension);
  painter->restore();
}

void symbolChooser::createSymbolType3(QPainter* painter,
                                      int drawDimension, int index) const {
  const qreal d1 = static_cast<qreal>(drawDimension)/3;
  const qreal d2 = drawDimension - d1;
  QPolygonF poly1;
  poly1 << QPointF(0, 0) << QPointF(d1, 0) <<
    QPointF(0, d1) << QPointF(0, 0);
  QPolygonF poly2;
  poly2 << QPointF(drawDimension, drawDimension) <<
    QPointF(drawDimension, d2) << QPointF(d2, drawDimension) <<
    QPointF(drawDimension, drawDimension);
  if (index%2 == 1) {
    painter->save();
    painter->setBrush(Qt::black);
    painter->drawConvexPolygon(poly1);
    painter->drawConvexPolygon(poly2);
    painter->restore();
  }
  else {
    painter->drawConvexPolygon(poly1);
    painter->drawConvexPolygon(poly2);
  }
}

void symbolChooser::createSymbolType4(QPainter* painter,
                                      int drawDimension, int index) const {
  const qreal d1 = static_cast<qreal>(drawDimension)/3;
  const qreal d2 = drawDimension - d1;
  QPolygonF poly1;
  poly1 << QPointF(0, drawDimension) << QPointF(0, d2) <<
    QPointF(d1, drawDimension) << QPointF(0, drawDimension);
  QPolygonF poly2;
  poly2 << QPointF(drawDimension, 0) << QPointF(d2, 0) <<
    QPointF(drawDimension, d1) << QPointF(drawDimension, 0);
  if (index%2 == 1) {
    painter->save();
    painter->setBrush(Qt::black);
    painter->drawConvexPolygon(poly1);
    painter->drawConvexPolygon(poly2);
    painter->restore();
  }
  else {
    painter->drawConvexPolygon(poly1);
    painter->drawConvexPolygon(poly2);
  }
}

QVector<patternSymbolIndex> symbolChooser::
symbolsAvailable(const triC& color, int symbolDim) {

  symbolDimension_ = symbolDim;
  QVector<patternSymbolIndex> availableSymbols;
  const QRgb rgbColor = color.qrgb();
  if (color.intensity() <= medianIntensity_) {
    loadIndices(&availableSymbols, darkIndex_.availableIndices(), rgbColor);
    loadIndices(&availableSymbols, lightIndex_.availableIndices(), rgbColor);
  }
  else {
    loadIndices(&availableSymbols, lightIndex_.availableIndices(), rgbColor);
    loadIndices(&availableSymbols, darkIndex_.availableIndices(), rgbColor);
  }
  return availableSymbols;
}

void symbolChooser::loadIndices(QVector<patternSymbolIndex>* vector,
                                const QSet<int>& indices, QRgb color) const {

  for (QSet<int>::const_iterator it = indices.begin(),
          end = indices.end(); it != end; ++it) {
    vector->push_back(patternSymbolIndex(createSymbol(*it, color),
                                         *it, borderDimension_,
                                         symbolDimension_));
  }
}

bool symbolChooser::changeSymbol(const triC& color, int newIndex) {

  const QRgb rgbColor = color.qrgb();
  const QHash<QRgb, patternSymbolIndex>::const_iterator it =
    symbolMap_.find(rgbColor);
  if (it != symbolMap_.end()) {
    const int oldIndex = it.value().index();
    if (oldIndex == newIndex) {
      return true;
    }
    // make sure the new index is available
    if ((newIndex%2 == 0 && !darkIndex_.indexIsAvailable(newIndex)) ||
        (newIndex%2 == 1 && !lightIndex_.indexIsAvailable(newIndex))) {
      return false;
    }
    // free the old index
    if (oldIndex%2 == 0) {
      darkIndex_.free(oldIndex);
    }
    else {
      lightIndex_.free(oldIndex);
    }
    symbolMap_.remove(rgbColor);
    // add the new index
    if (newIndex%2 == 0) {
      darkIndex_.reserve(newIndex);
    }
    else {
      lightIndex_.reserve(newIndex);
    }
    symbolMap_.insert(rgbColor,
                      patternSymbolIndex(createSymbol(newIndex, rgbColor),
                                         newIndex, borderDimension_,
                                         symbolDimension_));
    return true;
  }
  else {
    qWarning() << "Unreserved color in symbolChooser::changeSymbol" <<
      ::ctos(color) << newIndex;
    return false;
  }
}

void symbolChooser::initializeSymbolList() {

  if (!unicodeCharacters_.empty()) { // we're already initialized
    return;
  }
  // symbols we'd like to use if they're available
  QVector<QChar> maybeChars;
  maybeChars.push_back(QChar(0x20)); // space
  // geometric shapes
  maybeChars.push_back(QChar(0x25a0));
  maybeChars.push_back(QChar(0x25a1));
  for (int i = 0x25ac; i <= 0x25b3; ++i) {
    maybeChars.push_back(QChar(i));
  }
  maybeChars.push_back(QChar(0x25b6));
  maybeChars.push_back(QChar(0x25b7));
  maybeChars.push_back(QChar(0x25bc));
  maybeChars.push_back(QChar(0x25bd));
  maybeChars.push_back(QChar(0x25c0));
  maybeChars.push_back(QChar(0x25c1));
  maybeChars.push_back(QChar(0x25c6));
  maybeChars.push_back(QChar(0x25c7));
  for (int i = 0x25cf; i <= 0x25d7; ++i) {
    maybeChars.push_back(QChar(i));
  }
  for (int i = 0x25dc; i <= 0x25e5; ++i) {
    maybeChars.push_back(QChar(i));
  }
  for (int i = 0x25e7; i <= 0x25ea; ++i) {
    maybeChars.push_back(QChar(i));
  }
  for (int i = 0x25ed; i <= 0x25fa; ++i) {
    maybeChars.push_back(QChar(i));
  }
  // miscellaneous symbols
  maybeChars.push_back(QChar(0x2605));
  maybeChars.push_back(QChar(0x2606));
  maybeChars.push_back(QChar(0x260a));
  maybeChars.push_back(QChar(0x260b));
  maybeChars.push_back(QChar(0x260d));
  for (int i = 0x2648; i <= 0x264b; ++i) {
    maybeChars.push_back(QChar(i));
  }
  maybeChars.push_back(QChar(0x264e));
  // dingbats
  for (int i = 0x2780; i <= 0x2788; ++i) {
    maybeChars.push_back(QChar(i));
  }
  for (int i = 0x278a; i <= 0x2792; ++i) {
    maybeChars.push_back(QChar(i));
  }
  // mathematical operators
  maybeChars.push_back(QChar(0x2200));
  maybeChars.push_back(QChar(0x2202));
  maybeChars.push_back(QChar(0x2203));
  maybeChars.push_back(QChar(0x2204));
  maybeChars.push_back(QChar(0x2208));
  maybeChars.push_back(QChar(0x2209));
  maybeChars.push_back(QChar(0x220f));
  maybeChars.push_back(QChar(0x2210));
  maybeChars.push_back(QChar(0x2213));
  maybeChars.push_back(QChar(0x221e));
  maybeChars.push_back(QChar(0x221f));
  maybeChars.push_back(QChar(0x2220));
  maybeChars.push_back(QChar(0x2222));
  maybeChars.push_back(QChar(0x2225));
  maybeChars.push_back(QChar(0x2226));
  maybeChars.push_back(QChar(0x222b));
  maybeChars.push_back(QChar(0x222c));
  maybeChars.push_back(QChar(0x223f));
  maybeChars.push_back(QChar(0x2240));
  maybeChars.push_back(QChar(0x224e));
  maybeChars.push_back(QChar(0x2259));
  maybeChars.push_back(QChar(0x225a));
  maybeChars.push_back(QChar(0x2263));
  maybeChars.push_back(QChar(0x2266));
  maybeChars.push_back(QChar(0x2267));
  maybeChars.push_back(QChar(0x226a));
  maybeChars.push_back(QChar(0x226b));
  maybeChars.push_back(QChar(0x2276));
  for (int i = 0x2282; i <= 0x2285; ++i) {
    maybeChars.push_back(QChar(i));
  }
  maybeChars.push_back(QChar(0x228f));
  maybeChars.push_back(QChar(0x2290));
  maybeChars.push_back(QChar(0x2293));
  maybeChars.push_back(QChar(0x2294));
  maybeChars.push_back(QChar(0x2296));
  maybeChars.push_back(QChar(0x229f));
  for (int i = 0x22a2; i <= 0x22a5; ++i) {
    maybeChars.push_back(QChar(i));
  }
  maybeChars.push_back(QChar(0x22bb));
  maybeChars.push_back(QChar(0x22bc));
  maybeChars.push_back(QChar(0x22bd));
  maybeChars.push_back(QChar(0x22bf));
  maybeChars.push_back(QChar(0x22c0));
  maybeChars.push_back(QChar(0x22c2));
  for (int i = 0x22c8; i <= 0x22cc; ++i) {
    maybeChars.push_back(QChar(i));
  }
  // miscellaneous technical
  maybeChars.push_back(QChar(0x2318));
  // arrows
  maybeChars.push_back(QChar(0x21af));
  for (int i = 0x21cd; i <= 0x21d9; ++i) {
    maybeChars.push_back(QChar(i));
  }
  // miscellaneous mathematical symbols-b
  maybeChars.push_back(QChar(0x29ce));
  for (int i = 0x29d1; i <= 0x29d5; ++i) {
    maybeChars.push_back(QChar(i));
  }
  // miscellaneous symbols and arrows
  for (int i = 0x2b12; i <= 0x2b19; ++i) {
    maybeChars.push_back(QChar(i));
  }
  // number forms
  maybeChars.push_back(QChar(0x2183));
  // general punctuation
  maybeChars.push_back(QChar(0x203c));
  maybeChars.push_back(QChar(0x2047));
  maybeChars.push_back(QChar(0x2048));
  maybeChars.push_back(QChar(0x2049));
  maybeChars.push_back(QChar(0x204b));
  // phonetic extensions
  maybeChars.push_back(QChar(0x1d1f));
  // basic latin
  maybeChars.push_back(QChar(0x0021));
  for (int i = 0x0023; i <= 0x0026; ++i) {
    maybeChars.push_back(QChar(i));
  }
  for (int i = 0x0028; i <= 0x002b; ++i) {
    maybeChars.push_back(QChar(i));
  }
  for (int i = 0x002f; i <= 0x0039; ++i) {
    maybeChars.push_back(QChar(i));
  }
  maybeChars.push_back(QChar(0x003d));
  for (int i = 0x003f; i <= 0x005e; ++i) {
    maybeChars.push_back(QChar(i));
  }
  for (int i = 0x0061; i <= 0x007d; ++i) {
    maybeChars.push_back(QChar(i));
  }

  QFontDatabase database;
  QStringList fontList(database.families());
  const QFont appFont(QApplication::font());
  // we'll use a font on this list if it exists on the system
  QStringList sufficientFontList;
#ifdef Q_OS_LINUX
  if (fontList.contains("Sans Serif")) {
    sufficientFontList << "Sans Serif";
  }
#endif
#ifdef Q_OS_WIN
  if (QSysInfo::windowsVersion() == QSysInfo::WV_XP) {
    // the XP install I have is rather pauce on unicode fonts with the
    // symbols I use, Lucinda Sans Unicode being the best with 199/255
    // symbols.  On the other hand I tested some unicode fonts I
    // installed that I wouldn't want to use, even though they provide
    // all 255 symbols, so here's the compromise: use Lucinda Sans
    // Unicode if it exists and has more symbols than the default
    // application font; if Lucinda Sans Unicode doesn't exist then
    // we'll take our chances with whatever font provided by the
    // system provides the most symbols
    if (fontList.contains("Lucida Sans Unicode")) {
      sufficientFontList << "Lucida Sans Unicode";
    }
  }
#endif
  ////#ifdef MAC?

  if (!sufficientFontList.isEmpty()) {
    fontList = sufficientFontList;
    // prefer the app font if there's a tie
    fontList.push_front(appFont.family());
  }
  else {
    // use the system font list, but
    // move the app font to the front of the list (prefer it)
    fontList.move(fontList.indexOf(appFont.family()), 0);
  }

  // choose the font on fontList with the most symbols, prefering
  // fonts at the front of the list in the event of a tie
  int maxCount = 0;
  QFont chosenFont = appFont;
  foreach (const QString thisFontString, fontList) {
    const QFont thisFont(thisFontString);
    const QFontMetrics thisFontMetric(thisFont);
    int thisCount = 0;
    foreach (QChar thisChar, maybeChars) {
      if (thisFontMetric.inFont(thisChar)) {
        ++thisCount;
      }
    }
    if (thisCount > maxCount) {
      maxCount = thisCount;
      chosenFont = thisFont;
      if (maxCount == maybeChars.size()) {
        break; // we can't do any better, so stop looking
      }
    }
  }
  const QFontMetrics chosenFontMetrics(chosenFont);
  foreach (QChar thisChar, maybeChars) {
    if (chosenFontMetrics.inFont(thisChar)) {
      unicodeCharacters_.push_back(thisChar);
    }
//   else {
//      qDebug() << "Symbol not in font: "<< QStringthisChar.unicode();
//    }
  }
  unicodeFont_ = chosenFont;
  //qDebug() << "Font selection time: " << double(t.elapsed())/1000. <<
  //chosenFont.family();
}

QPixmap symbolChooser::getSampleSymbol(int symbolSize) {

  QPixmap symbol(symbolSize, symbolSize);
  symbol.fill(Qt::white);
  QPainter painter(&symbol);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.fillRect(0, 0, symbolSize, symbolSize, Qt::black);
  const int innerDim = 2;
  painter.fillRect(QRect(innerDim, innerDim,
                         symbolSize - 2*innerDim,
                         symbolSize - 2*innerDim), Qt::white);

  const QChar symbolChar = 'f';
  const QString symbolString(symbolChar);
  // find the right font size for our box size
  QFont font(unicodeFont_);
  ::setFontHeight(&font, symbolSize + 2);
  painter.setFont(font);
  painter.drawText(0, 0, symbolSize, symbolSize,
                   Qt::AlignCenter, symbolString);
  return symbol;
}
