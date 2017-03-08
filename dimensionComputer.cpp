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

#include <QtCore/QSettings>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>
#include <QDebug>
#include <QStringBuilder>

#include "utility.h"

extern const int SQUARE_SIZE_MIN;
extern const int SQUARE_SIZE_MAX;

dimensionComputer::dimensionComputer(const QSize& imageSize,
                                     int startDimension,
                                     QWidget* parent)
  : cancelAcceptDialogBase(parent),
    width_(imageSize.width()), height_(imageSize.height()),
    tab_("&nbsp;&nbsp;&nbsp;&nbsp;") {

  //: note space at the end
  QLabel* unitsLabel = new QLabel(tr("Units: "), this);
  unitsBox_ = new QComboBox(this);
  unitsBox_->addItem(tr("Inches"), QVariant(INCHES));
  unitsBox_->addItem(tr("Centimeters"), QVariant(CENTIMETERS));
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

  // A line for computing dimensions from a user-settable fabric count.
  QHBoxLayout* fabricChooserLayout = new QHBoxLayout;
  //: These three strings eventually get glued together to read, for example:
  //: "[Final fabric dimensions for] 8 [squares per] cm [fabric]: 5x10"
  fabricSizeTextStart_ = tr("Final fabric dimensions for");
  fabricSizeTextMid_ = tr("squares per");
  fabricSizeTextEnd_ = tr("fabric");
  QLabel* fabricChoiceLabelStart =
    new QLabel(tab_ % fabricSizeTextStart_ % " ", this);
  // (This is required as soon as you add some non-default stretch to the layout
  // containing the label. Shrug.)
  fabricChoiceLabelStart->setTextFormat(Qt::RichText);
  // Text will be set in updateDims().
  fabricChoiceLabelEnd_ = new QLabel(this);
  fabricChoiceLabelEnd_->setTextFormat(Qt::RichText);
  fabricCountBox_ = new QDoubleSpinBox(this);
  fabricCountBox_->setDecimals(1);
  fabricCountBox_->setRange(0, 99.5);
  connect(fabricCountBox_, SIGNAL(valueChanged(const QString& )),
          this, SLOT(updateDims()));
  fabricChooserLayout->addWidget(fabricChoiceLabelStart);
  fabricChooserLayout->addWidget(fabricCountBox_);
  fabricChooserLayout->addWidget(fabricChoiceLabelEnd_);
  fabricChooserLayout->addStretch();
  
  QVBoxLayout* groupBoxLayout = new QVBoxLayout;
  groupBoxLayout->addLayout(squareSizeLayout);
  groupBoxLayout->addWidget(outputLabel_);
  groupBoxLayout->addSpacing(4);
  groupBoxLayout->addLayout(fabricChooserLayout);
  groupBoxLayout->setSizeConstraint(QLayout::SetFixedSize);

  groupBox->setLayout(groupBoxLayout);

  QVBoxLayout* vLayout = new QVBoxLayout;
  vLayout->addLayout(unitsLayout);
  vLayout->addWidget(groupBox);
  vLayout->addWidget(cancelAcceptWidget());
  vLayout->setSizeConstraint(QLayout::SetFixedSize);

  setLayout(vLayout);
  setWindowTitle(tr("Compute dimensions"));

  const QSettings settings("cstitch", "cstitch");
  if (settings.contains("dimension_computer_units")) {
    const QVariant units = settings.value("dimension_computer_units");
    unitsBox_->setCurrentIndex(unitsBox_->findData(units));
  }
  else {
    // Set units based on the locale.
    const QLocale locale;
    if (locale.measurementSystem() == QLocale::MetricSystem) {
      unitsBox_->setCurrentIndex(unitsBox_->findData(QVariant(CENTIMETERS)));
    }
    else {
      unitsBox_->setCurrentIndex(unitsBox_->findData(QVariant(INCHES)));
    }
  }
  if (settings.contains("dimension_computer_fabric_count")) {
    const double fabricCount =
      settings.value("dimension_computer_fabric_count").toDouble();
    fabricCountBox_->setValue(fabricCount);
  }
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
    aidas << 7 << 10 << 11 << 12 << 14 << 16 << 18 << 20 << 22 << 24 << 25 << 28;
  }
  else {
    //: singular
    newUnit = tr("cm");
    //: plural
    newUnitPlural = tr("cm");
    aidas << 2.4 << 4.4 << 5.5 << 7;
  }

  const int newValue = squareSizeBox_->value();
  const int xBoxes = width_ / newValue;
  const int yBoxes = height_ / newValue;
  const QString endText = tr("your final pattern will be %1x%2 squares.")
                            .arg(::itoqs(xBoxes)).arg(::itoqs(yBoxes));
  squareSizeLabelEnd_->setText(endText);

  //// Update the labels for the fixed fabric sizes.
  const QString newlines("<br /><br />");
  QString newText;
  for (int i = 0, size = aidas.size(); i < size; ++i) {
    const qreal width = xBoxes / aidas[i];
    const qreal height = yBoxes / aidas[i];
    const QString count = QString("%1").arg(aidas[i], 2);
    // (The "%"s are StringBuilder concatenation.)
    newText = newText %
      tab_ %
      fabricSizeTextStart_ % " " %
      highlightText(count) % " " %
      createFabricDescriptionEnd(width, height, newUnit, newUnitPlural) %
      newlines;
  }
  newText.chop(newlines.size()); // Remove the newlines on the last entry.
  outputLabel_->setText(newText);

  //// Update the label for the user-specified fabric size.
  const double userSquaresPerUnit = fabricCountBox_->value();
  qreal userSetWidth, userSetHeight;
  if (userSquaresPerUnit == 0) {
    userSetWidth = 0;
    userSetHeight = 0;
  }
  else {
    userSetWidth = xBoxes / userSquaresPerUnit;
    userSetHeight = yBoxes / userSquaresPerUnit;
  }
  const QString userSetEndText = createFabricDescriptionEnd(userSetWidth,
                                                            userSetHeight,
                                                            newUnit,
                                                            newUnitPlural);
  fabricChoiceLabelEnd_->setText("&nbsp;" % userSetEndText);

  update();
}

QString dimensionComputer::createFabricDescriptionEnd(qreal width, qreal height,
                                                      QString unit,
                                                      QString unitPlural) {

  // "[squares per] cm [fabric]:"
  const QString descriptionStart =
    fabricSizeTextMid_ % " " %
    unit % " " %
    fabricSizeTextEnd_ % ":";

  if (width == 0 || height == 0) {
    return descriptionStart;
  }
  else {
    return
      descriptionStart % " " %
      highlightText(::rtoqs(width) + "x" % ::rtoqs(height)) % " " %
      unitPlural;
  }
}

QString dimensionComputer::highlightText(const QString& text) {

  return "<b style='color: #298220'>" % text % "</b>";
}

int dimensionComputer::getDimension() const {

  return squareSizeBox_->value();
}

void dimensionComputer::processAcceptClick() {

  QSettings settings("cstitch", "cstitch");
  settings.setValue("dimension_computer_units", unitsBox_->currentData().toInt());
  const double fabricCount = fabricCountBox_->value();
  if (fabricCount > 0) {
    settings.setValue("dimension_computer_fabric_count", fabricCount);
  }

  cancelAcceptDialogBase::processAcceptClick();
}
