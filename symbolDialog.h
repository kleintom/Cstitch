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

#ifndef SYMBOLDIALOG_H
#define SYMBOLDIALOG_H

#include "imageUtility.h"
#include "cancelAcceptDialogBase.h"

///
// symbolDialog is a popup dialog that presents a grid of symbols for the
// user to select from; this dialog execed returns accept or cancel; the
// last symbol clicked before the accept/cancel is returned by
// selectedSymbol()
//
class symbolDialog : public cancelAcceptDialogBase {

  Q_OBJECT

 public:
  symbolDialog(const QVector<patternSymbolIndex>& availableSymbols,
               const QPixmap& originalSymbol,
               QWidget* parent = NULL);
  // return the last selected symbol
  patternSymbolIndex selectedSymbol() const {return selectedSymbol_;}

 private slots:
  // record the symbol corresponding to <index> (from symbols_)
  void setSymbolSelected(int index);

 private:
  QVector<patternSymbolIndex> symbols_;
  patternSymbolIndex selectedSymbol_;
};

#endif
