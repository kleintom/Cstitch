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

#ifndef DIMENSIONCOMPUTER_H
#define DIMENSIONCOMPUTER_H

#include "cancelAcceptDialogBase.h"

class QSpinBox;
class QDoubleSpinBox;
class QLabel;
class QComboBox;

// a dialog for the user to be able to compute the final pattern size
// (in inches and squares) based on the current image size and a
// user-selected square size and fabric type.
class dimensionComputer : public cancelAcceptDialogBase {

  Q_OBJECT

 public:
  // <imageSize> is the current image size (in pixels), 
  // <startDimension> is the initial dimension for the spinbox
  explicit dimensionComputer(const QSize& imageSize, 
                             int startDimension, QWidget* parent);
  int getDimension() const;

 private:
  static QString highlightText(const QString& text);
  // Create the end text for a label describing the fabric size corresponding to
  // some squares per <unit> fabric, where the size of the fabric is
  // <width>x<height>.
  QString createFabricDescriptionEnd(qreal width, qreal height,
                                     QString unit, QString unitPlural);

 protected slots:
  virtual void processAcceptClick();

 private slots:
  // update the computed dimensions based on the current input values
  void updateDims();

 private:
  enum {INCHES, CENTIMETERS};
  // image width and height in pixels
  const int width_;
  const int height_;
  const QString tab_;
  // let the user select the square size for their pattern
  QSpinBox* squareSizeBox_;
  // imperial or metric?
  QComboBox* unitsBox_;
  // the end of the square size text label (the beginning is fixed)
  QLabel* squareSizeLabelEnd_;
  QLabel* outputLabel_; // the output text
  // Let the user specify a fabric square count per unit.
  QString fabricSizeTextStart_;
  QString fabricSizeTextMid_;
  QString fabricSizeTextEnd_;
  QDoubleSpinBox* fabricCountBox_;
  QLabel* fabricChoiceLabelEnd_;
};

#endif
