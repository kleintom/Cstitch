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

#include "symbolDialog.h"

#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

#include "buttonGrid.h"

symbolDialog::symbolDialog(const QVector<patternSymbolIndex>& availableSymbols,
                           const QPixmap& originalSymbol,
                           QWidget* parent)
  : cancelAcceptDialogBase(parent), symbols_(availableSymbols) {

  QScrollArea* scrollArea = new QScrollArea(this);
  scrollArea->setAlignment(Qt::AlignCenter);
  QVector<QPixmap> symbolPixmaps;
  symbolPixmaps.reserve(symbols_.size());
  for (int i = 0, size = symbols_.size(); i < size; ++i) {
    symbolPixmaps.push_back(symbols_[i].symbol());
  }
  buttonGrid* grid = new buttonGrid(symbolPixmaps, "Choose a symbol", this);
  connect(grid, SIGNAL(buttonSelected(int)),
          this, SLOT(setSymbolSelected(int)));
  scrollArea->setWidget(grid);
  enableAcceptButton(false);

  QHBoxLayout* labelsLayout = new QHBoxLayout;
  QLabel* textLabel = new QLabel;
  //: note space at the end
  textLabel->setText(tr("Change symbol "));
  QLabel* symbolLabel = new QLabel;
  symbolLabel->setPixmap(originalSymbol);
  labelsLayout->addWidget(textLabel, 0, Qt::AlignRight);
  labelsLayout->addWidget(symbolLabel);

  QVBoxLayout* widgetLayout = new QVBoxLayout;
  widgetLayout->addWidget(scrollArea);
  widgetLayout->addLayout(labelsLayout);
  widgetLayout->addWidget(cancelAcceptWidget());
  setLayout(widgetLayout);
  setWindowTitle(tr("Change symbol"));
}

void symbolDialog::setSymbolSelected(int index) {

  enableAcceptButton(true);
  selectedSymbol_ = symbols_[index];
}

