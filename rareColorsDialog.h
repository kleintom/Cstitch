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

#ifndef RARECOLORSDIALOG_H
#define RARECOLORSDIALOG_H

#include <QtCore/QList>

#include "cancelAcceptDialogBase.h"

class QVBoxLayout;
class QHBoxLayout;
class QCheckBox;
class QLabel;
class QSpinBox;
class QScrollArea;
class QWidget;
template<class T1, class T2> struct QPair;

typedef QPair<QRgb, QRgb> QRgbPair;

//
// class rareColorsDialog
//
// rareColorsDialog presents the user with a list of "rare" colors in the
// current image and chooses and displays a replacement color for each
// rare color - currently the user takes the replacement or leaves it
// (via a checkbox).  A color is rare if it occurs in n or fewer boxes,
// where n is chosen by the user from a spinbox (the dialog updates its
// list as the spinbox changes its value)
//

class rareColorsDialog : public cancelAcceptDialogBase {

  Q_OBJECT

 public:
  // <colorCounts> provides a hash with keys the colors in the current
  // image and values the number of squares of that color in the image
  rareColorsDialog(const QHash<QRgb, int>& colorCounts);
  // return the list of colors that the user selected to change
  QList<QRgbPair> colorsToChange() const;

 private slots:
  // the user changed the min spinbox, so recalculate the rare colors and
  // update the dialog
  void processNewMin(int min);
  // returns a pixmap square that's half oldColor, half newColor (in that
  // order)
  QPixmap createIconPixmap(QRgb oldColor, QRgb newColor) const;
  // check or uncheck all color checkboxes (checkOrUncheck is "check" or
  // "uncheck")
  void checkUncheckCheckboxes(const QString& checkOrUncheck);

 private:
  QVBoxLayout* mainLayout_;

  QHBoxLayout* minCountLayout_;
  // the min count spinbox goes in between the text of leftLabel and
  // rightLabel
  QLabel* minCountLeftLabel_;
  QLabel* minCountRightLabel_;
  QSpinBox* minCountBox_;

  QList<QCheckBox*> checkboxes_; // each color gets a checkbox
  QScrollArea* scroll_; // for the color list
  QWidget* scrollWidget_;
  QVBoxLayout* colorsLayout_;
  // displayed if there are no non-rare colors to choose from
  QLabel* noReplacementColorsLabel_;
  // displayed if there are no rare colors
  QLabel* noRareColorsLabel_;

  // clickable text to select all checkboxes
  QLabel* selectAllLabel_;
  // clickable text to deselect all checkboxes
  QLabel* selectNoneLabel_;

  // keys are colors in the current image, values are the number of times
  // a color appears in that image
  const QHash<QRgb, int>& colorCounts_;
};

#endif
