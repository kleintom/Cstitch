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

#include <QtGui/QSpinBox>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QVBoxLayout>

#include "utility.h"

extern const int SQUARE_SIZE_MIN;
extern const int SQUARE_SIZE_MAX;

dimensionComputer::dimensionComputer(const QSize& imageSize,
                                     int startDimension,
                                     QWidget* parent)
  : cancelAcceptDialogBase(parent),
    width_(imageSize.width()), height_(imageSize.height()) {

  QGroupBox* groupBox = 
    new QGroupBox(tr("Click OK to set the main window square size"),
                  this);

  // the start of the square size line
  QLabel* squareSizeLabel = 
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

  QHBoxLayout* squareSizeLayout = new QHBoxLayout;
  squareSizeLayout->addWidget(squareSizeLabel);
  squareSizeLayout->addWidget(squareSizeBox_);
  squareSizeLayout->addWidget(squareSizeLabelEnd_);
  squareSizeLayout->addStretch();
  
  QVBoxLayout* groupBoxLayout = new QVBoxLayout;
  groupBoxLayout->addLayout(squareSizeLayout);
  groupBoxLayout->addWidget(outputLabel_);
  groupBox->setLayout(groupBoxLayout);
  QVBoxLayout* vLayout = new QVBoxLayout;
  vLayout->addWidget(groupBox);
  vLayout->addWidget(cancelAcceptWidget());

  setLayout(vLayout);
  setWindowTitle(tr("Compute dimensions"));
  // (call updateDims() instead of just doing the setValue here since 
  // if the setValue doesn't change the default value then updateDims()
  // is never called)
  updateDims();
}

void dimensionComputer::updateDims() {

  const int newValue = squareSizeBox_->value();
  const int xBoxes = width_/newValue;
  const int yBoxes = height_/newValue;
  squareSizeLabelEnd_->setText("your final pattern will be " +
                               itoqs(xBoxes) + "x" + itoqs(yBoxes) +
                               " squares.");
  const int aidasSize = 8;
  const int aidas[aidasSize] = {7, 10, 11, 12, 14, 18, 25, 28};
  QString newText;
  const QString tab = "   ";
  for (int i = 0; i < aidasSize; ++i) {
    const qreal width = static_cast<qreal>(xBoxes)/aidas[i];
    const qreal height = static_cast<qreal>(yBoxes)/aidas[i];
    const QString count = QString("%1").arg(aidas[i], 2);
    newText += tab + "Final fabric dimensions for " + 
      "<span style='color: #474858'>" +
      count +
      "</span> squares per inch fabric: <span style='color: #474858'>" +
      ::rtoqs(width) + "x" + ::rtoqs(height) + "</span> inches<br /><br />";
  }
  newText.chop(12); // remove the last 2 <br />
  outputLabel_->setText(newText);
  update();
}

int dimensionComputer::getDimension() const {

  return squareSizeBox_->value();
}
