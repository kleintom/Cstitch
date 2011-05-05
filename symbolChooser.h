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

#ifndef SYMBOLCHOOSER_H
#define SYMBOLCHOOSER_H

#include "triC.h"
#include "stepIndex.h"
#include "imageUtility.h"

extern const int MAX_NUM_SYMBOL_TYPES;

//
// symbolChooser makes the initial choice of the color->symbol
// correspondence for a user-supplied list of colors.
//
// Each symbol is made up of a symbol pixmap, a border (always
// specified by the width of the border, not of the symbol plus its
// border), and a numerical index that can be used to identify a
// particular image (cf. patternSymbolIndex, which is used to pass
// around symbols).  symbolChooser will initially generate all symbols
// at a base dimension and border; after the initial construction the
// user can either request symbols with a new overall icon size with
// the default border width or symbols with no border and a specified
// size.
//
//// Implementation notes
//
// Each time the user requests a new symbol or symbols, the new symbol(s)
// get inserted into the object's symbolMap_ if they weren't already there
// (at the specified overall size and border size).
//
// Image creation: Symbols come from a predefined list of unicode
// characters; the actual number of symbols available will depend on
// which of those symbols is actually available for the given font on
// the given computer in use (but should always include at least a
// subset of the standard ascii characters).  Each unicode symbol can
// be paired with up to four different distinct outlines.  The
// outlines used depend on the number of symbols required: the actual
// number of symbols available will be at least the number required
// for the given colors, but not necessarily all symbols that could be
// created.
//
// Each time the user requests a symbol that doesn't already exist for
// the requested color with the given dimension and border, a new
// symbol is added to the symbol map.  In other words, only one
// version of a symbol is ever stored at a time, and there may be
// multiple combinations of size/border stored in the symbol map at
// any given time.
//
class symbolChooser {

 public:
  // <symbolDimension> is the overall size of the final symbol,
  // <borderDim> is the width of the border, <colors> are the colors to
  // create symbols for
  symbolChooser(int symbolDimension, QVector<triC> colors,
                int borderDim = 4);
  // run through a predefined list of unicode characters to determine
  // which are defined in the current system font and save the list to
  // unicodeCharacters_
  static void initializeSymbolList();
  // set the current symbolDimension_, used as the default for all
  // subsequent requests for symbols
  void setSymbolDimension(int dimension);
  // return the symbol for <color> with total size <symbolDim> and no
  // border
  patternSymbolIndex getSymbolNoBorder(const triC& color, int symbolDim) {
    int savedBorderDim = borderDimension_;
    borderDimension_ = 0;
    patternSymbolIndex returnIndex(getSymbol(color, symbolDim));
    borderDimension_ = savedBorderDim;
    return returnIndex;
  }
  // return the index for <color> if it exists, else return -1
  int colorIndex(const triC& color) const {
    const QHash<QRgb, patternSymbolIndex>::const_iterator it =
      symbolMap_.find(color.qrgb());
    if (it != symbolMap_.end()) {
      return it.value().index();
    }
    else {
      return -1;
    }
  }
  // return the symbol for <color> with total size <symbolDim> and the
  // current borderDimension_
  patternSymbolIndex getSymbol(const triC& color, int symbolDim);
  // return the symbol for <color> using the current symbolSize_ and
  // borderDimension_
  patternSymbolIndex getSymbolCurDim(const triC& color) {
    return getSymbol(color, symbolDimension_);
  }
  // return a hash of symbols for <colors> with total size <symbolDim> and
  // no border
  QHash<QRgb, QPixmap> getSymbolsNoBorder(const QVector<triC>& colors,
                                          int symbolDim) {
    int savedBorderDim = borderDimension_;
    borderDimension_ = 0;
    QHash<QRgb, QPixmap> returnHash(getSymbols(colors, symbolDim));
    borderDimension_ = savedBorderDim;
    return returnHash;
  }
  // return a hash of symbols for <colors> with total size <symbolDim> and
  // the current borderDimension_
  QHash<QRgb, QPixmap> getSymbols(const QVector<triC>& colors,
                                  int symbolDim);
  // return a hash of symbols for <colors> using the current
  // symbolDimension_ and borderDimension_
  QHash<QRgb, QPixmap> getSymbolsCurDim(const QVector<triC>& colors) {
    return getSymbols(colors, symbolDimension_);
  }
  // remove the old symbol map entry for <color> and insert a new symbol
  // for <color> with symbol <newIndex>
  // returns true if the switch was sucessful
  bool changeSymbol(const triC& color, int newIndex);
  // return a list of all in play symbols that aren't currently assigned
  // to a color
  QVector<patternSymbolIndex>
    symbolsAvailable(const triC& symbolBackgroundColor, int symbolDim);
  // return the total number of possible symbols at our disposal
  static int maxNumberOfSymbols() {
    initializeSymbolList();
    return MAX_NUM_SYMBOL_TYPES * unicodeCharacters_.size();
  }
  // return a sample symbol of size <symbolSize> using the symbol font
  static QPixmap getSampleSymbol(int symbolSize);

 private:
  // return the next available index;
  // returns numberOfSymbols() if all indices have been used
  int getNewIndex() { return colorIndex_.next(); }
  // create and return the symbol for index <index> using the current
  // symbolDim and borderDim and <color> for the background if there's a
  // border; doesn't update symbolMap_
  QPixmap createSymbol(int index,
                       const triC& color = triC(255, 255, 255)) const;
  // draw the background pixmap for the different types of symbols
  // (type1 is "plain")
  void createSymbolType2(QPainter* painter, int drawDim) const;
  void createSymbolType3(QPainter* painter, int drawDim) const;
  void createSymbolType4(QPainter* painter, int drawDim) const;
  // return the total number of possible symbols
  int numberOfSymbols() const {
    return unicodeCharacters_.size() * numSymbolTypes_;
  }

 private:
  // the unicode characters to be used as symbols
  static QVector<QChar> unicodeCharacters_;
  // the font we're using for symbols (chosen for max # of symbols)
  static QFont unicodeFont_;
  // the total number of symbol types we're using (with light and dark
  // types counted separately)
  int numSymbolTypes_;
  // the most recently created symbols for the hash's color keys
  QHash<QRgb, patternSymbolIndex> symbolMap_;
  stepIndex colorIndex_; // available symbol indices
  int symbolDimension_; // size of the symbols (with border)
  // size of a border around the symbol (0 means no border)
  int borderDimension_;
};

#endif
