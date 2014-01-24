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

#include <algorithm>

#include <QtCore/QDebug>

#include <QFontDatabase>
#include <QPainter>
#include <QtWidgets/QApplication>

#include "utility.h"
#include "imageProcessing.h"

extern const int MAX_NUM_SYMBOL_TYPES = 4;
QVector<QChar> symbolChooser::unicodeCharacters_ = QVector<QChar>();
symbolChooser::cheapFont symbolChooser::unicodeFont_ = symbolChooser::cheapFont();

// functor for comparing two triCs by intensity
class triCIntensityDefinite {
 public:
  bool operator()(const triC& c1, const triC& c2) const {
    return ::definiteIntensityCompare(c1, c2);
  }
};

symbolChooser::symbolChooser(int symbolDimension, const QVector<triC>& colors,
                             int borderDimension)
  : colors_(colors), symbolDimension_(symbolDimension),
    borderDimension_(borderDimension) {

  std::sort(colors_.begin(), colors_.end(), triCIntensityDefinite());
  initializeSymbolList();
  const int colorCount = colors_.size();
  const int characterCount = unicodeCharacters_.size();
  // use "just enough" symbols to cover the number of colors we need
  for (int i = 1; i <= MAX_NUM_SYMBOL_TYPES; ++i) {
    numSymbolTypes_ = i;
    if (numSymbolTypes_ * characterCount >= colorCount + 30) {
      break;
    }
  }
  colorIndex_ = stepIndex(0, numberOfSymbols(), 1);
  // it's possible these symbols will never get used with the default values, but
  // we still need to establish the color-->symbol correspondence here
  for (int i = 0, size = colors_.size(); i < size; ++i) {
    createNewSymbolCurDims(colors_[i].qrgb());
  }
}

void symbolChooser::setSymbolDimension(int dimension) {

  if (dimension > 0) {
    symbolDimension_ = dimension;
  }
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
    return createNewSymbolCurDims(rgbColor);
  }
}

patternSymbolIndex symbolChooser::createNewSymbolCurDims(QRgb color) {

  const int newIndex = getNewIndex();
  const patternSymbolIndex symbolIndex(createSymbol(newIndex, color),
                                       newIndex, borderDimension_,
                                       symbolDimension_);
  symbolMap_.insert(color, symbolIndex);
  return symbolIndex;
}

QHash<QRgb, QPixmap> symbolChooser::getSymbols(int symbolDim) {

  symbolDimension_ = symbolDim;
  QHash<QRgb, QPixmap> returnMap;
  for (int i = 0, size = colors_.size(); i < size; ++i) {
    const QRgb thisColor = colors_[i].qrgb();
    returnMap.insert(thisColor, getSymbolCurDim(thisColor).symbol());
  }
  return returnMap;
}

QPixmap symbolChooser::createSymbol(int index, const triC& color) const {

  const int drawDimension = symbolDimension_ - 2 * borderDimension_;
  QPixmap drawSymbol(drawDimension, drawDimension);
  drawSymbol.fill(Qt::white);
  if (index < numberOfSymbols()) {
    QPainter painter(&drawSymbol);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const int interval = unicodeCharacters_.size();
    // type1 symbol doesn't need further editing
    if (index >= interval && index < 2*interval) {
      index -= interval;
      createSymbolType2(&painter, drawDimension);
    }
    else if (index >= 2*interval && index < 3*interval) {
      index -= 2*interval;
      createSymbolType3(&painter, drawDimension);
    }
    else if (index >= 3*interval && index < 4*interval) {
      index -= 3*interval;
      createSymbolType4(&painter, drawDimension);
    }
    const QChar symbolChar = unicodeCharacters_[index];
    const QString symbolString(symbolChar);
    painter.setFont(unicodeFont_.qFont());
    const int heightBuffer = 2;
    ::setFontHeight(&painter, drawDimension - heightBuffer);
    const QRect textRect(0, heightBuffer/2, drawDimension,
                         drawDimension - heightBuffer/2);
    painter.drawText(textRect, Qt::AlignCenter, symbolString);
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

// circle
void symbolChooser::createSymbolType2(QPainter* painter,
                                      int drawDimension) const {

  painter->save();
  painter->drawEllipse(0, 0, drawDimension, drawDimension);
  painter->restore();
}

// NW and SE diagonals
void symbolChooser::createSymbolType3(QPainter* painter,
                                      int drawDimension) const {

  const qreal d1 = static_cast<qreal>(drawDimension)/3;
  const qreal d2 = drawDimension - d1;
  painter->setBrush(Qt::black);
  painter->drawLine(0, d1, d1, 0);
  painter->drawLine(d2, drawDimension, drawDimension, d2);
}

// NE and SW diagonals
void symbolChooser::createSymbolType4(QPainter* painter,
                                      int drawDimension) const {

  const qreal d1 = static_cast<qreal>(drawDimension)/3;
  const qreal d2 = drawDimension - d1;
  painter->setBrush(Qt::black);
  painter->drawLine(d2, 0, drawDimension, d1);
  painter->drawLine(0, d2, d1, drawDimension);
}

QVector<patternSymbolIndex> symbolChooser::
symbolsAvailable(const triC& color, int symbolDim) {

  symbolDimension_ = symbolDim;
  QVector<patternSymbolIndex> availableSymbols;
  const QRgb rgbColor = color.qrgb();
  const QList<int> availableIndices = colorIndex_.availableIndices();
  for (QList<int>::const_iterator it = availableIndices.begin(),
         end = availableIndices.end(); it != end; ++it) {
    availableSymbols.push_back(patternSymbolIndex(createSymbol(*it,
                                                               rgbColor),
                                                  *it, borderDimension_,
                                                  symbolDimension_));
  }
  return availableSymbols;
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
    if (!colorIndex_.indexIsAvailable(newIndex)) {
      return false;
    }
    // free the old index
    colorIndex_.free(oldIndex);
    symbolMap_.remove(rgbColor);
    colorIndex_.reserve(newIndex);
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

  const QString symbolString("f");
  // find the right font size for our box size
  const QFont unicodeFont = unicodeFont_.qFont();
  QFont font(unicodeFont);
  painter.setFont(unicodeFont);
  ::setFontHeight(&painter, symbolSize - 2);
  painter.drawText(0, 1, symbolSize, symbolSize,
                   Qt::AlignCenter, symbolString);
  return symbol;
}

void symbolChooser::initializeSymbolList() {

  if (!unicodeCharacters_.empty()) { // we're already initialized
    return;
  }
  // symbols we'd like to use if they're available
  QVector<QChar> maybeChars;
  maybeChars.push_back(QChar(0x20)); // space
  // geometric shapes
  maybeChars.push_back(QChar(0x25a1));
  maybeChars.push_back(QChar(0x25b2));
  maybeChars.push_back(QChar(0x25b3));
  maybeChars.push_back(QChar(0x25b6));
  maybeChars.push_back(QChar(0x25b7));
  maybeChars.push_back(QChar(0x25bc));
  maybeChars.push_back(QChar(0x25bd));
  maybeChars.push_back(QChar(0x25c0));
  maybeChars.push_back(QChar(0x25c1));
  for (int i = 0x25e0; i <= 0x25e5; ++i) {
    maybeChars.push_back(QChar(i));
  }
  for (int i = 0x25e7; i <= 0x25eb; ++i) {
    maybeChars.push_back(QChar(i));
  }
  //[ miscellaneous symbols and arrows
  for (int i = 0x2b12; i <= 0x2b15; ++i) {
    maybeChars.push_back(QChar(i));
  }
  //]
  for (int i = 0x25f0; i <= 0x25f3; ++i) {
    maybeChars.push_back(QChar(i));
  }
  maybeChars.push_back(QChar(0x25f8));
  maybeChars.push_back(QChar(0x25f9));
  maybeChars.push_back(QChar(0x25fa));
  maybeChars.push_back(QChar(0x25ff));
  // box drawings
  maybeChars.push_back(QChar(0x254b));
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
  maybeChars.push_back(QChar(0x26af));
  // Miscellaneous Mathematical Symbols-A
  maybeChars.push_back(QChar(0x27c2));
  maybeChars.push_back(QChar(0x27c8));
  maybeChars.push_back(QChar(0x27c9));
  maybeChars.push_back(QChar(0x27d2));
  maybeChars.push_back(QChar(0x27da));
  // mathematical operators
  maybeChars.push_back(QChar(0x2200));
  maybeChars.push_back(QChar(0x2202));
  maybeChars.push_back(QChar(0x2203));
  maybeChars.push_back(QChar(0x2204));
  maybeChars.push_back(QChar(0x2208));
  maybeChars.push_back(QChar(0x2209));
  maybeChars.push_back(QChar(0x220b));
  maybeChars.push_back(QChar(0x220c));
  maybeChars.push_back(QChar(0x220f));
  maybeChars.push_back(QChar(0x2211));
  maybeChars.push_back(QChar(0x2213));
  maybeChars.push_back(QChar(0x2220));
  maybeChars.push_back(QChar(0x2222));
  maybeChars.push_back(QChar(0x2225));
  maybeChars.push_back(QChar(0x222b));
  maybeChars.push_back(QChar(0x222c));
  maybeChars.push_back(QChar(0x223f));
  maybeChars.push_back(QChar(0x224e));
  maybeChars.push_back(QChar(0x225a));
  maybeChars.push_back(QChar(0x2263));
  maybeChars.push_back(QChar(0x2266));
  maybeChars.push_back(QChar(0x2267));
  maybeChars.push_back(QChar(0x226a));
  maybeChars.push_back(QChar(0x226b));
  maybeChars.push_back(QChar(0x2276));
  maybeChars.push_back(QChar(0x2284));
  maybeChars.push_back(QChar(0x2285));
  maybeChars.push_back(QChar(0x2290));
  maybeChars.push_back(QChar(0x2296));
  maybeChars.push_back(QChar(0x22a2));
  maybeChars.push_back(QChar(0x22a3));
  maybeChars.push_back(QChar(0x22a5));
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
  for (int i = 0x21d0; i <= 0x21d9; ++i) {
    maybeChars.push_back(QChar(i));
  }
  // Supplemental Mathematical Operators
  maybeChars.push_back(QChar(0x2a01));
  maybeChars.push_back(QChar(0x2a02));
  maybeChars.push_back(QChar(0x2a05));
  maybeChars.push_back(QChar(0x2a06));
  maybeChars.push_back(QChar(0x2a20));
  maybeChars.push_back(QChar(0x2a4c));
  maybeChars.push_back(QChar(0x2a4d));
  maybeChars.push_back(QChar(0x2a5e));
  maybeChars.push_back(QChar(0x2a60));
  maybeChars.push_back(QChar(0x2a62));
  maybeChars.push_back(QChar(0x2a63));
  maybeChars.push_back(QChar(0x2a71));
  maybeChars.push_back(QChar(0x2a72));
  maybeChars.push_back(QChar(0x2acf));
  maybeChars.push_back(QChar(0x2ad9));
  // miscellaneous mathematical symbols-b
  maybeChars.push_back(QChar(0x29c9));
  maybeChars.push_back(QChar(0x29cd));
  maybeChars.push_back(QChar(0x29ce));
  for (int i = 0x29d1; i <= 0x29d7; ++i) {
    maybeChars.push_back(QChar(i));
  }
  maybeChars.push_back(QChar(0x29f0));
  maybeChars.push_back(QChar(0x29f1));
  // number forms
  maybeChars.push_back(QChar(0x2180));
  // general punctuation
  maybeChars.push_back(QChar(0x203c));
  maybeChars.push_back(QChar(0x2047));
  maybeChars.push_back(QChar(0x2048));
  maybeChars.push_back(QChar(0x2049));
  maybeChars.push_back(QChar(0x204b));
  // letterlike symbols
  maybeChars.push_back(QChar(0x2142));
  maybeChars.push_back(QChar(0x2143));
  maybeChars.push_back(QChar(0x2144));
  // miscellaneous symbols and arrows
  for (int i = 0x2b16; i <= 0x2b19; ++i) {
    maybeChars.push_back(QChar(i));
  }
  // Latin extended B
  maybeChars.push_back(QChar(0x019f));
  maybeChars.push_back(QChar(0x01a9));
  // basic latin
  maybeChars.push_back(QChar(0x0021));
  maybeChars.push_back(QChar(0x0023));
  maybeChars.push_back(QChar(0x0025));
  maybeChars.push_back(QChar(0x0026));
  for (int i = 0x0028; i <= 0x002a; ++i) {
    maybeChars.push_back(QChar(i));
  }
  for (int i = 0x0030; i <= 0x0039; ++i) {
    maybeChars.push_back(QChar(i));
  }
  maybeChars.push_back(QChar(0x003d));
  for (int i = 0x003f; i <= 0x005a; ++i) {
    maybeChars.push_back(QChar(i));
  }
  for (int i = 0x0061; i <= 0x007d; ++i) {
    maybeChars.push_back(QChar(i));
  }
  // dingbats
  for (int i = 0x2780; i <= 0x2788; ++i) {
    maybeChars.push_back(QChar(i));
  }
  for (int i = 0x278a; i <= 0x2792; ++i) {
    maybeChars.push_back(QChar(i));
  }
  // geometric shapes (cont.)
  maybeChars.push_back(QChar(0x25cf));
  maybeChars.push_back(QChar(0x25a0));

  QFontDatabase database;
  QStringList fontList(database.families());
  const QFont appFont(QApplication::font());
  const QString appFontFamily = appFont.family();
//  qDebug() << "Application font: " << appFont.family();
//  qDebug() << "Available fonts: " << fontList;
  // who knows what we'll get if we just blindly choose a font
  // family from the system list, so prefer this font
  // (to be loaded (or not) below)
  QString preferredFontFamily;
#ifdef Q_OS_LINUX
  if (fontList.contains("DejaVu Sans")) {
    preferredFontFamily = "DejaVu Sans";
  }
  else if (fontList.contains("Sans Serif")) {
    preferredFontFamily = "Sans Serif";
  }
#endif
#ifdef Q_OS_WIN
  const QSysInfo::WinVersion winVersion = QSysInfo::windowsVersion();
  if ((winVersion & QSysInfo::WV_NT_based && winVersion >= QSysInfo::WV_XP)) {
    // supposedly xp, vista, win7, win8[.1] all come with
    // Lucida Sans Unicode and Tahoma, which both
    // provide good symbols; prefer Lucida since historically
    // that was what was preferred on XP, and there's less chance
    // of breaking projects if we stick with that
    if (fontList.contains("Lucida Sans Unicode")) {
      preferredFontFamily = "Lucida Sans Unicode";
    }
    else if (fontList.contains("Tahoma")) {
      preferredFontFamily = "Tahoma";
    }
    // supposedly dlg 2 always maps to Tahoma ...
    else if (fontList.contains("MS Shell Dlg 2")) {
      preferredFontFamily = "MS Shell Dlg 2";
    }
    // Segoe UI symbols look pretty small and crappy (win8)
  }
#endif
  ////#ifdef MAC?

  if (!preferredFontFamily.isEmpty()) {
    fontList.clear();
    fontList << preferredFontFamily;
  }
  else {
    // use the system font list, but
    // move the app font to the front of the list (prefer it)
    //// NOTE that appFont's family may not appear in fontList...
    if (fontList.contains(appFontFamily)) {
      fontList.move(fontList.indexOf(appFontFamily), 0);
    }
    else {
      fontList.push_front(appFontFamily);
    }
  }

  QFont chosenFont;
  if (fontList.length() == 1) {
    chosenFont = fontList[0];
  }
  else {
    // choose the font on fontList with the most symbols, prefering
    // fonts at the front of the list in the event of a tie
    int maxCount = 0;
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
      }
    }
  }
//  qDebug() << "Font chosen: " << chosenFont;
  const QFontMetrics chosenFontMetrics(chosenFont);
  foreach (QChar thisChar, maybeChars) {
    if (chosenFontMetrics.inFont(thisChar)) {
      unicodeCharacters_.push_back(thisChar);
    }
  }
//  qDebug() << "Number of font symbols available: " << unicodeCharacters_.length();
  unicodeFont_.setFont(chosenFont);
}
