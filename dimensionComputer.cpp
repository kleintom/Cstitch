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

#include "dimensionComputer.h"

#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>
#include <QDebug>

#include "utility.h"

extern const int SQUARE_SIZE_MIN;
extern const int SQUARE_SIZE_MAX;

dimensionComputer::dimensionComputer(const QSize& imageSize,
                                     int startDimension,
                                     QWidget* parent)
  : cancelAcceptDialogBase(parent),
    width_(imageSize.width()), height_(imageSize.height()) {

  //: note space at the end
  QLabel* unitsLabel = new QLabel(tr("Units: "), this);
  unitsBox_ = new QComboBox(this);
  unitsBox_->addItem(tr("Inches"));
  unitsBox_->addItem(tr("Centimeters"));
  connect(unitsBox_, SIGNAL(currentTextChanged(const QString& )),
          this, SLOT(updateDims()));

  QGroupBox* groupBox = 
    new QGroupBox(tr("Click OK to set the main window square size"), this);

  // the start of the square size line
  QLabel* squareSizeLabel =
    //: The full text will be:
    //: "If you choose a square size of [box] your final pattern will be
    //: [width by height] squares."
    //: with box a box letting the user choose a square size.  The phrase
    //: "If you choose a square size of" goes before the box.
    new QLabel(tr("If you choose a square size of"), this);
  // the end of the square size line (set in updateDims())
  squareSizeLabelEnd_ = new QLabel(this);
  squareSizeBox_ = new QSpinBox(this);
  squareSizeBox_->setMinimum(SQUARE_SIZE_MIN);
  squareSizeBox_->setMaximum(SQUARE_SIZE_MAX);
  squareSizeBox_->setValue(startDimension);
  connect(squareSizeBox_, SIGNAL(valueChanged(const QString& )),
          this, SLOT(updateDims()));
  // label for the fabric dimensions
  outputLabel_ = new QLabel(this);

  QHBoxLayout* unitsLayout = new QHBoxLayout;
  unitsLayout->addWidget(unitsLabel, 0, Qt::AlignRight);
  unitsLayout->addWidget(unitsBox_, 0, Qt::AlignRight);
  unitsLayout->insertStretch(0, 1);  

  QHBoxLayout* squareSizeLayout = new QHBoxLayout;
  squareSizeLayout->addWidget(squareSizeLabel);
  squareSizeLayout->addWidget(squareSizeBox_);
  squareSizeLayout->addWidget(squareSizeLabelEnd_);
  squareSizeLayout->addStretch();
  
  QVBoxLayout* groupBoxLayout = new QVBoxLayout;
  groupBoxLayout->addLayout(squareSizeLayout);
  groupBoxLayout->addWidget(outputLabel_);
groupBoxLayout->setSizeConstraint(QLayout::SetFixedSize);
  groupBox->setLayout(groupBoxLayout);

  QVBoxLayout* vLayout = new QVBoxLayout;
  vLayout->addLayout(unitsLayout);
  vLayout->addWidget(groupBox);
  vLayout->addWidget(cancelAcceptWidget());
vLayout->setSizeConstraint(QLayout::SetFixedSize);

  setLayout(vLayout);
  setWindowTitle(tr("Compute dimensions"));
  // (call updateDims() instead of just doing the setValue here since 
  // if the setValue doesn't change the default value then updateDims()
  // is never called)
  updateDims();
}

void dimensionComputer::updateDims() {

  QString newUnit, newUnitPlural;
  QList<qreal> aidas;
  if (unitsBox_->currentText() == tr("Inches")) {
    //: singular
    newUnit = tr("inch");
    //: plural
    newUnitPlural = tr("inches");
    aidas << 7 << 10 << 11 << 12 << 14 << 16 << 18 << 22 << 25 << 28;
  }
  else {
    //: singular
    newUnit = tr("cm");
    //: plural
    newUnitPlural = tr("cm");
    aidas << 2.4 << 4.4 << 5.5 << 7;
  }

  const int newValue = squareSizeBox_->value();
  const int xBoxes = width_/newValue;
  const int yBoxes = height_/newValue;
  const QString endText = tr("your final pattern will be %1x%2 squares.")
                            .arg(::itoqs(xBoxes)).arg(::itoqs(yBoxes));
  squareSizeLabelEnd_->setText(endText);

  QString newText;
  const QString tab = "   ";
  for (int i = 0, size = aidas.size(); i < size; ++i) {
    const qreal width = xBoxes/aidas[i];
    const qreal height = yBoxes/aidas[i];
    const QString count = QString("%1").arg(aidas[i], 2);
    newText += tab + tr("Final fabric dimensions for "
                        "<span style='color: #474858'>%1</span> squares per "
                        "%2 fabric: "
                        "<span style='color: #474858'>%3x%4</span> "
                        "%5<br /><br />")
                       .arg(count).arg(newUnit).arg(::rtoqs(width))
                       .arg(::rtoqs(height)).arg(newUnitPlural);
    
  }
  newText.chop(12); // remove the last 2 <br />
  outputLabel_->setText(newText);
  update();
}

int dimensionComputer::getDimension() const {

  return squareSizeBox_->value();
}
