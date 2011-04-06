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

#include "rareColorsDialog.h"

#include <QtCore/QDebug>

#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QScrollArea>
#include <QtGui/QPainter>

#include "dmcList.h"
#include "imageUtility.h"
#include "triC.h"
#include "utility.h"

extern const int D_MAX; // max distance between two colors
static const int ICON_SIZE = 32;

rareColorsDialog::rareColorsDialog(const QHash<QRgb, int>& colorCounts)
  : cancelAcceptDialogBase(NULL), colorCounts_(colorCounts) {

  minCountLeftLabel_ = new QLabel(tr("Replace checked colors that occur "));
  minCountRightLabel_ = new QLabel(tr(" or fewer times."));

  minCountBox_ = new QSpinBox;
  const int rangeMax = 100;
  minCountBox_->setRange(1, rangeMax);
  // make the box a fixed width; otherwise it grows with the width of the
  // dialog (and I don't trust sizePolicy()s that I've never been able to
  // get to do what I expect)
  const QFontMetrics fontMetrics(font());
  minCountBox_->setFixedWidth(fontMetrics.width(::itoqs(rangeMax)) + 30);
  connect(minCountBox_, SIGNAL(valueChanged(int )),
          this, SLOT(processNewMin(int )));

  minCountLayout_ = new QHBoxLayout;
  minCountLayout_->addWidget(minCountLeftLabel_);
  minCountLayout_->addWidget(minCountBox_);
  minCountLayout_->addWidget(minCountRightLabel_);

  scroll_ = new QScrollArea;
  // Qt tells me to use stylesheets to style widgets (instead of palettes),
  // but if you change one style property of a complex widget, you have to
  // change _every_ property of the widget.  Huh?
  // http://doc.trolltech.com/4.7-snapshot/stylesheet-customizing.html
  //Note: With complex widgets such as QComboBox and QScrollBar, if one
  //property or sub-control is customized, all the other properties or
  //sub-controls must be customized as well.
//  scroll_->setStyleSheet("QWidget { background-color: white; }");
  scroll_->viewport()->setStyleSheet("background-color: white;");
  // make the scroll as wide as the widest possible row
  const QString testString(::itoqs(rangeMax) +
                           " squares (255, 255, 255) --> (255, 255, 255)");
  // PM_IndicatorWidth is the width of a checkbox
  const int maxWidth = fontMetrics.width(testString) +
    scroll_->style()->pixelMetric(QStyle::PM_IndicatorWidth) +
    ::scrollbarWidth() + ICON_SIZE + 35;
  scroll_->setMinimumWidth(maxWidth);
  scroll_->setMinimumHeight(200);
  scrollWidget_ = NULL;
  colorsLayout_ = NULL;
  noReplacementColorsLabel_ = NULL;
  noRareColorsLabel_ = NULL;

  selectAllLabel_ = new QLabel(tr("<a href=\"check\">Check all boxes</a>"),
                               this);
  selectAllLabel_->setTextFormat(Qt::RichText);
  connect(selectAllLabel_, SIGNAL(linkActivated(const QString& )),
          this, SLOT(checkUncheckCheckboxes(const QString& )));
  selectNoneLabel_ = new QLabel(tr("<a href=\"uncheck\">Uncheck all boxes</a>"),
                                this);
  selectNoneLabel_->setTextFormat(Qt::RichText);
  connect(selectNoneLabel_, SIGNAL(linkActivated(const QString& )),
          this, SLOT(checkUncheckCheckboxes(const QString & )));

  mainLayout_ = new QVBoxLayout;
  mainLayout_->addLayout(minCountLayout_);
  mainLayout_->addWidget(scroll_);
  mainLayout_->addWidget(selectAllLabel_);
  mainLayout_->addWidget(selectNoneLabel_);
  mainLayout_->addWidget(cancelAcceptWidget());
  setLayout(mainLayout_);

  minCountBox_->setValue(5);
  setWindowTitle(tr("Replace rare colors"));
}

void rareColorsDialog::processNewMin(int min) {

  // clean out the old rows
  for (int i = checkboxes_.size() - 1; i >= 0; --i) {
    delete checkboxes_.takeAt(i);
  }
  // see sarcastic note about this krap at the bottom of this bleepin
  // function
  delete scrollWidget_; // deletes any children
  scrollWidget_ = new QWidget(scroll_);
  colorsLayout_ = new QVBoxLayout;
  scrollWidget_->setLayout(colorsLayout_);

  QList<QRgb> rareColors;
  QList<QRgb> commonColors;
  for (QHash<QRgb, int>::const_iterator it = colorCounts_.begin();
       it != colorCounts_.end(); ++it) {
    if (it.value() <= min) {
      rareColors.push_back(it.key());
    }
    else {
      commonColors.push_back(it.key());
    }
  }
  // sort the rare colors by intensity
  qSort(rareColors.begin(), rareColors.end(), qRgbIntensity());
  if (!commonColors.isEmpty() && !rareColors.isEmpty()) {
    // each row consists of a checkbox saying whether or not to include
    // the row's color, followed by the rare color and its count and then
    // the replacement color for the rare color
    for (int i = 0, size = rareColors.size(); i < size; ++i) {

      QRgb thisOldColor = rareColors[i];

      // find the new color
      QRgb thisNewColor = ::closestMatch(thisOldColor, commonColors);
      const int thisColorCount = colorCounts_[thisOldColor];
      const QString squareString = (thisColorCount == 1) ?
        " square " : " squares ";
      const QString iconString(::itoqs(thisColorCount) + squareString +
                               ::colorToPrettyString(thisOldColor) + " --> " +
                               ::colorToPrettyString(thisNewColor));
      QCheckBox* checkBox = new QCheckBox(iconString);
      const QPixmap iconPixmap = createIconPixmap(thisOldColor, thisNewColor);
      checkBox->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
      checkBox->setIcon(QIcon(iconPixmap));
      checkBox->setChecked(true);
      checkboxes_.push_back(checkBox);
      colorsLayout_->addWidget(checkBox);
    }
  }
  else if (rareColors.isEmpty()) {
    noRareColorsLabel_ =
      new QLabel(tr("There aren't any colors that occur ") +
                 ::itoqs(minCountBox_->value()) + " or fewer times!");
    colorsLayout_->addWidget(noRareColorsLabel_);
  }
  else if (commonColors.isEmpty()) {
    noReplacementColorsLabel_ =
      new QLabel(tr("There are no replacement colors available!"));
    colorsLayout_->addWidget(noReplacementColorsLabel_);
  }

  scrollWidget_->resize(colorsLayout_->sizeHint());
  // if you put this up where the other stuff gets recreated (or you don't
  // recreate the other stuff) then on the second and all later runs
  // through this function colorsLayout_->sizeHint() will return (18, 18).
  // Obviously.
  scroll_->setWidget(scrollWidget_);
}

QPixmap rareColorsDialog::createIconPixmap(QRgb oldColor, QRgb newColor)
  const {

  const int iconDim = ICON_SIZE;
  const int halfDim = iconDim/2;
  QPixmap pixmap(iconDim, iconDim);
  QPainter painter(&pixmap);
  painter.fillRect(0, 0, halfDim, iconDim, QColor(oldColor));
  painter.fillRect(halfDim, 0, halfDim, iconDim, QColor(newColor));
  painter.drawRect(0, 0, halfDim - 1, iconDim - 1);
  painter.drawRect(halfDim, 0, halfDim - 1, iconDim - 1);

  return pixmap;
}

void rareColorsDialog::checkUncheckCheckboxes(const QString& checkOrUncheck) {

  bool checkAll = (checkOrUncheck == "check") ? true : false;
  for (int i = 0, size = checkboxes_.size(); i < size; ++i) {
    checkboxes_[i]->setChecked(checkAll);
  }
}

QList<QRgbPair> rareColorsDialog::colorsToChange() const {

  QList<QRgbPair> returnPairs;
  QRegExp regex("\\((\\d+), (\\d+), (\\d+)\\).*\\((\\d+), (\\d+), (\\d+)\\)");
  for (int i = 0, size = checkboxes_.size(); i < size; ++i) {
    if (checkboxes_[i]->isChecked() &&
        regex.indexIn(checkboxes_[i]->text()) != -1) {
      QStringList matches = regex.capturedTexts();
      // remember that matches[0] is the entire match
      returnPairs.push_back(QRgbPair(qRgb(matches[1].toInt(),
                                          matches[2].toInt(),
                                          matches[3].toInt()),
                                     qRgb(matches[4].toInt(),
                                          matches[5].toInt(),
                                          matches[6].toInt())));
    }
  }
  qDebug() << "size" << returnPairs.size();
  return returnPairs;
}


