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

#include "colorDialog.h"

#include <algorithm>

#include <QtCore/qmath.h>
#include <QtCore/QDebug>

#include <QtGui/QScrollArea>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QApplication>
#include <QtGui/QComboBox>
#include <QtGui/QPainter>
#include <QtGui/QCloseEvent>
#include <QtGui/QColorDialog>
#include <QtGui/QDesktopWidget>

#include "dmcList.h"
#include "colorButton.h"
#include "imageLabel.h"
#include "imageProcessing.h"
#include "buttonGrid.h"


// a less than functor used to sort colors by distance from the input color
class triCDistanceSort {
 public:
  explicit triCDistanceSort(const triC& c) : c_(c) {}
  bool operator()(const triC& c1, const triC& c2) const {
    return ::ds(c_, c1) < ::ds(c_, c2);
  }
 private:
  const triC c_;
};


void baseDialogMode::constructGrid(QVBoxLayout* dialogLayout, int gridWidth,
                                   const QVector<triC>& colors,
                                   QWidget* parent) {

  maxCoordinates_ = pairOfInts((colors.size()-1)%gridWidth,
                               (colors.size()-1)/gridWidth);
  scrollArea_ = new QScrollArea(parent);
  scrollArea_->setAlignment(Qt::AlignCenter);
  dialogLayout->insertWidget(0, scrollArea_);
  // second parameter is button icon size
  grid_ = new colorButtonGrid(colors, 22, QObject::tr("Choose a color"),
                              gridWidth, parent);
  QObject::connect(grid_, SIGNAL(buttonSelected(QRgb, int , int )),
                   parent, SLOT(setColorSelected(QRgb, int, int )));
  scrollArea_->setWidget(grid_);
  widget_ = grid_;
  isNull_ = false;
}

void baseDialogMode::enable() {

  if (scrollArea_) {
    scrollArea_->show();
  }
  if (widget_) {
    widget_->show();
  }
  if (grid_) {
    grid_->focusButton(currentGridSelection_.x(),
                       currentGridSelection_.y());
  }
}

void baseDialogMode::disable() {

  if (scrollArea_) {
    scrollArea_->hide();
  }
  if (widget_) {
    widget_->hide();
  }
}

QSize baseDialogMode::sizeHint() const {

  return (grid_ == NULL) ?
    QSize(0, 0) : QSize(grid_->width(), grid_->height());
}

QSize baseDialogMode::scrollFrameSize() const {

  return (scrollArea_ == NULL) ? QSize(0, 0) :
    QSize(scrollArea_->frameWidth(), scrollArea_->frameWidth());
}

void baseDialogMode::setGridFocus(const pairOfInts& coordinates) {

  if (grid_) {
    grid_->focusButton(coordinates.x(), coordinates.y());
  }
}

squareDialogMode::
squareDialogMode(QVBoxLayout* dialogLayout, QVector<triC> colors,
                 bool dmcOnly, const triC& inputColor, QWidget* parent) {

  const int gridWidth = 10;
  setGridWidth(gridWidth);
  if (dmcOnly) {
    colors = ::rgbToDmc(colors, true);
  }
  std::sort(colors.begin(), colors.end(), triCDistanceSort(inputColor));
  constructGrid(dialogLayout, gridWidth, colors, parent);
  disable();
}

listDialogMode::
listDialogMode(QVBoxLayout* dialogLayout, QVector<triC> colors,
               bool dmcOnly, const triC& inputColor, QWidget* parent) {

  const int gridWidth = qMax(static_cast<int>(ceil(sqrt(colors.size()))),
                             10);
  setGridWidth(gridWidth);
  if (dmcOnly) {
    colors = ::rgbToDmc(colors, true);
  }
  std::sort(colors.begin(), colors.end(), triCDistanceSort(inputColor));
  constructGrid(dialogLayout, gridWidth, colors, parent);
  disable();
}

dmcDialogMode::
dmcDialogMode(QVBoxLayout* dialogLayout, const triC& inputColor,
              QWidget* parent) {

  QVector<triC> dmcColors = ::loadDMC();
  std::sort(dmcColors.begin(), dmcColors.end(),
            triCDistanceSort(inputColor));
  int gridWidth = qMax(static_cast<int>(ceil(sqrt(dmcColors.size()))), 10);
  gridWidth += gridWidth/4;
  setGridWidth(gridWidth);
  constructGrid(dialogLayout, gridWidth, dmcColors, parent);
  disable();
}

imageDialogMode::
imageDialogMode(QVBoxLayout* dialogLayout, const triC& inputColor,
                bool dmcOnly, QWidget* parent) {

  const QColor inputColorQC(inputColor.qc());
  colorLabel_ = new QLabel(parent);
  setWidget(colorLabel_);

  QPixmap colorPatch(CFI_WIDTH, CFI_WIDTH);
  colorPatch.fill(QApplication::palette().color(QPalette::Window));
  QPainter painter;
  painter.begin(&colorPatch);
  // fill the whole thing with the color from the image
  painter.fillRect(CFI_BORDER, CFI_BORDER,
                   CFI_INNER_WIDTH, CFI_INNER_WIDTH, inputColorQC);
  const QColor baseColor =
    dmcOnly ? ::rgbToDmc(inputColor).qc() : inputColorQC;
  // fill "chosen" square with dmc version of input image color
  painter.fillRect(CFI_WIDTH/2, CFI_BORDER,
                   CFI_INNER_WIDTH/2, CFI_INNER_WIDTH/2, baseColor);
  // border around the whole rectangle
  painter.drawRect(CFI_BORDER, CFI_BORDER,
                   CFI_INNER_WIDTH, CFI_INNER_WIDTH);
  // vertical midline
  painter.drawLine(CFI_WIDTH/2, CFI_BORDER,
                   CFI_WIDTH/2, CFI_WIDTH - CFI_BORDER);
  // horizontal midline for the right side
  painter.drawLine(CFI_WIDTH/2, CFI_WIDTH/2,
                   CFI_WIDTH - CFI_BORDER, CFI_WIDTH/2);
  painter.end();
  colorLabel_->setPixmap(colorPatch);
  dialogLayout->insertWidget(0, colorLabel_, 0, Qt::AlignCenter);
  disable();
}

void imageDialogMode::updateImageColorLabel(QRgb color, bool mouseClick) {

  // mouse move if not mouseClick
  const int yStart = mouseClick ? CFI_BORDER : CFI_WIDTH/2;

  QPixmap colorPatch = *(colorLabel_->pixmap());
  QPainter painter;
  painter.begin(&colorPatch);
  painter.fillRect(CFI_WIDTH/2, yStart,
                   CFI_INNER_WIDTH/2, CFI_INNER_WIDTH/2, QColor(color));
  painter.drawRect(CFI_WIDTH/2, yStart,
                   CFI_INNER_WIDTH/2, CFI_INNER_WIDTH/2);
  painter.end();
  colorLabel_->setPixmap(colorPatch);
}

colorDialog::colorDialog(const QVector<triC>& listColors,
                         const triC& inputColor, bool dmcOnly,
                         int frameWidth, int frameHeight)
  : cancelAcceptDialogBase(NULL), dialogLayout_(new QVBoxLayout),
    inputColor_(inputColor), colorSelected_(inputColor), dmcOnly_(dmcOnly),
    listColors_(listColors), curMode_(NULL),
    frameWidth_(frameWidth), frameHeight_(frameHeight) {

  setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
  dialogLayout_->setSpacing(0);
  setLayout(dialogLayout_);

  constructorHelper(false, dmcOnly);
  setCurMode(CD_LIST);

  setWindowTitle(tr("Select color"));
  fitDialog();
}

colorDialog::colorDialog(const QVector<triC>& squareColors,
                         const QVector<triC>& listColors,
                         const triC& inputColor, bool dmcOnly,
                         int frameWidth, int frameHeight)
  : cancelAcceptDialogBase(NULL), dialogLayout_(new QVBoxLayout),
    inputColor_(inputColor), colorSelected_(inputColor), dmcOnly_(dmcOnly),
    listColors_(listColors),
    squareColors_(dmcOnly_ ? squareColors : ::rgbToDmc(squareColors, true)),
    curMode_(NULL), frameWidth_(frameWidth), frameHeight_(frameHeight) {

  setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
  dialogLayout_->setSpacing(0);
  setLayout(dialogLayout_);
  if (squareColors.size() > 1) {
    constructorHelper(true, dmcOnly);
    setCurMode(CD_SQUARE);
  }
  else {
    constructorHelper(false, dmcOnly);
    setCurMode(CD_LIST);
  }
  setWindowTitle(tr("Select color"));
  fitDialog();
}

void colorDialog::constructorHelper(bool useSquareColors, bool dmcOnly) {

  setAttribute(Qt::WA_DeleteOnClose);

  // left right buttons and label
  leftRightColorPatch_ = new QLabel;
  QPixmap colorPatch(2*LR_BOX + 2*LR_BORDER, LR_BOX + 2*LR_BORDER);
  colorPatch.fill(QColor(180, 180, 180));
  QPainter painter;
  painter.begin(&colorPatch);
  painter.fillRect(LR_BORDER, LR_BORDER, 2*LR_BOX, LR_BOX,
                   inputColor_.qc());
  painter.drawRect(LR_BORDER, LR_BORDER, 2*LR_BOX, LR_BOX);
  painter.drawLine(LR_BORDER + LR_BOX, LR_BORDER,
                   LR_BORDER + LR_BOX, LR_BORDER + LR_BOX);
  painter.end();
  leftRightColorPatch_->setPixmap(colorPatch);

  leftButton_ = new QPushButton(QIcon(":leftArrow.png"), "", this);
  leftButton_->setEnabled(false);
  connect(leftButton_, SIGNAL(clicked()),
          this, SLOT(processLeftClick()));

  rightButton_ = new QPushButton(QIcon(":rightArrow.png"), "", this);
  rightButton_->setEnabled(false);
  connect(rightButton_, SIGNAL(clicked()),
          this, SLOT(processRightClick()));

  leftRightLayout_ = new QHBoxLayout;
  leftRightHolder_ = new QWidget;
  leftRightHolder_->setLayout(leftRightLayout_);
  dialogLayout_->addWidget(leftRightHolder_);

  leftRightLayout_->setAlignment(Qt::AlignHCenter);
  leftRightLayout_->addWidget(leftButton_);
  leftRightLayout_->addWidget(leftRightColorPatch_);
  leftRightLayout_->addWidget(rightButton_);

  // choice box
  modeChoiceBox_ = new QComboBox;
  if (useSquareColors) {
    modeChoiceBox_->addItem("Choose a square color",
                            QVariant::fromValue(CD_SQUARE));
  }
  modeChoiceBox_->addItem("Choose a list color",
                          QVariant::fromValue(CD_LIST));
  modeChoiceBox_->addItem("Choose a DMC color",
                          QVariant::fromValue(CD_DMC));
  modeChoiceBox_->addItem("Choose from an image",
                          QVariant::fromValue(CD_IMAGE));
  if (!dmcOnly) {
    modeChoiceBox_->addItem("Choose a new color",
                            QVariant::fromValue(CD_NEW));
  }

  connect(modeChoiceBox_, SIGNAL(activated(int )),
          this, SLOT(processModeChange(int )));

  buttonsLayout_ = new QHBoxLayout;
  buttonsLayout_->setSpacing(9);
  dialogLayout_->addLayout(buttonsLayout_);
  buttonsLayout_->addWidget(modeChoiceBox_);
  buttonsLayout_->addWidget(cancelAcceptWidget());
  move(200, 50);
}

void colorDialog::closeEvent(QCloseEvent* event) {

  emit colorDialogClosing();
  event->accept();
}

void colorDialog::setColorSelected(QRgb color, int x, int y) {

  curMode_->setCurrentGridSelection(pairOfInts(x, y));
  colorSelected_ = color;
  QPixmap colorPatch = *(leftRightColorPatch_->pixmap());
  QPainter painter;
  painter.begin(&colorPatch);
  painter.fillRect(LR_BORDER + LR_BOX, LR_BORDER, LR_BOX, LR_BOX,
                   QColor(color));
  painter.drawRect(LR_BORDER + LR_BOX, LR_BORDER, LR_BOX, LR_BOX);
  painter.end();
  leftRightColorPatch_->setPixmap(colorPatch);
  if (x == 0 && y == 0) {
    leftButton_->setEnabled(false);
  }
  else {
    leftButton_->setEnabled(true);
  }
  if (pairOfInts(x, y) == curMode_->getMaxGridCoordinates()) {
    rightButton_->setEnabled(false);
  }
  else {
    rightButton_->setEnabled(true);
  }
}

void colorDialog::processModeChange(int index) {

  dialogMode newMode =
    modeChoiceBox_->itemData(index).value<dialogMode>();
  setCurMode(newMode);
  fitDialog();
}

QSize colorDialog::sizeHint() const {

  int w = 0, h = 0;
  const QSize modeSizeHint = curMode_->sizeHint();
  w += modeSizeHint.width();
  h += modeSizeHint.height();

  // shouldn't add this if curMode_ is image mode...
  h += leftRightLayout_->minimumSize().height();

  h += buttonsLayout_->minimumSize().height();
  h += style()->pixelMetric(QStyle::PM_LayoutTopMargin)
    + style()->pixelMetric(QStyle::PM_LayoutBottomMargin);
  w += style()->pixelMetric(QStyle::PM_LayoutLeftMargin)
    + style()->pixelMetric(QStyle::PM_LayoutRightMargin);

  const QSize scrollFrameSize = curMode_->scrollFrameSize();
  w += 2*scrollFrameSize.width();
  h += 2*scrollFrameSize.height();

  w += 2*style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  h += 2*style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
#ifdef Q_OS_WIN
  // [yuk. I don't know why these are needed - if only updateGeometry()
  // worked...]
  w += 2;
  h += 2;
#endif
  return QSize(w, h);
}

void colorDialog::fitDialog() {

  const QSize hint = sizeHint();
  int hintWidth = hint.width();
  int hintHeight = hint.height();

  int newx = geometry().x();
  int newy = geometry().y();

  const int desktopWidth = QApplication::desktop()->width();
  const int desktopHeight = QApplication::desktop()->height();

  bool scrollx = false, scrolly = false;
  if (frameWidth_ + hintWidth > desktopWidth) {
    // make it full width
    hintWidth = desktopWidth - frameWidth_;
    scrollx = true;
    newx = geometry().x() - x();
  }
  if (frameHeight_ + hintHeight > desktopHeight) {
    // make it full height
    hintHeight = desktopHeight - frameHeight_;
    scrolly = true;
    newy = geometry().y() - y();
  }
  // if a horizontal scroll bar is going to appear then make room
  if (scrollx && !scrolly) {
    hintHeight += style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    // we may be too big now!
    if (hintHeight + frameHeight_ > desktopHeight) {
      // make it full height
      hintHeight = desktopHeight - frameHeight_;
      newy = geometry().y() - y();
    }
  }
  else if (scrolly && !scrollx) {
    hintWidth += style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    if (hintWidth + frameWidth_ > desktopWidth) {
      // make it full width
      hintWidth = desktopWidth - frameWidth_;
      newx = geometry().x() - x();
    }
  }

  const int leftFrameWidth = geometry().x() - x();
  const int rightFrameWidth =
    frameGeometry().width() - width() - leftFrameWidth;
  const int topFrameHeight = geometry().y() - y();
  const int bottomFrameHeight =
    frameGeometry().height() - height() - topFrameHeight;
  if (newx + hintWidth + rightFrameWidth > desktopWidth) {
    newx -= (newx + hintWidth + rightFrameWidth) - desktopWidth;
  }
  if (newx - leftFrameWidth < 0) {
    newx += leftFrameWidth - newx;
  }
  if (newy + hintHeight + bottomFrameHeight > desktopHeight) {
    newy -= (newy + hintHeight + bottomFrameHeight) - desktopHeight;
  }
  if (newy - topFrameHeight < 0) {
    newy += topFrameHeight - newy;
  }
  // sigh... this keeps getting reset somehow
  setMinimumHeight(0);
  setGeometry(newx, newy, hintWidth, hintHeight);
}

void colorDialog::processLeftClick() {

  const pairOfInts curSelection = curMode_->getCurrentGridSelection();
  // i is column, j is row
  int i = curSelection.x();
  int j = curSelection.y();
  if (i == 0 && j != 0) {
    i = curMode_->getGridWidth() - 1;
    --j;
  }
  else if (i != 0) {
    --i;
  }
  else {
    qWarning() << "Bad i,j in colorDialog left click:" << i << j;
    return;
  }
  const pairOfInts newCoords(i, j);
  curMode_->setCurrentGridSelection(newCoords);
  curMode_->setGridFocus(newCoords);
  if (i == 0 && j == 0) {
    leftButton_->setEnabled(false);
  }
}

void colorDialog::processRightClick() {

  const pairOfInts curSelection = curMode_->getCurrentGridSelection();
  // i is column, j is row
  int i = curSelection.x();
  int j = curSelection.y();
  const int w = curMode_->getGridWidth() - 1;

  if (i == w && j != w) {
    i = 0;
    ++j;
  }
  else if (i != w) {
    ++i;
  }
  else {
    qWarning()<< "Bad i,j in colorDialog right click: " << i << j;
    return;
  }
  const pairOfInts newCoords(i, j);
  curMode_->setCurrentGridSelection(newCoords);
  curMode_->setGridFocus(newCoords);
  const pairOfInts maxGridCoords = curMode_->getMaxGridCoordinates();
  if (i == maxGridCoords.x() && j == maxGridCoords.y()) {
    rightButton_->setEnabled(false);
  }
}

void colorDialog::processCancelClick() {

  emit finished(QDialog::Rejected, inputColor_, colorSelected_);
  close();
}

void colorDialog::processAcceptClick() {

  emit finished(QDialog::Accepted, inputColor_, colorSelected_);
  close();
}

void colorDialog::activateNewChooser() {

  hide();
  QColorDialog::setCustomColor(QColorDialog::customCount()-1,
                               inputColor_.qrgb());
  const QColor returnedColor = QColorDialog::getColor(inputColor_.qc());
  if (returnedColor.isValid()) {
    colorSelected_ = returnedColor;
    processAcceptClick();
  }
  else {
    processCancelClick();
  }
}

void colorDialog::updateMouseMove(QRgb color) {

  curMode_->updateImageColorLabel(color, false);
}

void colorDialog::updateMouseClick(QRgb color) {

  colorSelected_ = dmcOnly_ ? ::rgbToDmc(color) : color;
  curMode_->updateImageColorLabel(colorSelected_.qrgb(), true);
}

void colorDialog::keyPressEvent(QKeyEvent* event) {

  QWidget::keyPressEvent(event);
}

void colorDialog::setCurMode(dialogMode mode) {

  if (curMode_) {
    curMode_->disable();
    leftRightHolder_->hide();
  }

  switch (mode) {
  case CD_SQUARE:
    if (squareMode_.isNull()) {
      squareMode_ = squareDialogMode(dialogLayout_, squareColors_, dmcOnly_,
                                     inputColor_, this);
    }
    curMode_ = &squareMode_;
    leftRightHolder_->show();
    break;
  case CD_LIST:
    if (listMode_.isNull()) {
      listMode_ = listDialogMode(dialogLayout_, listColors_, dmcOnly_,
                                 inputColor_, this);
    }
    curMode_ = &listMode_;
    leftRightHolder_->show();
    break;
  case CD_DMC:
    if (dmcMode_.isNull()) {
      dmcMode_ = dmcDialogMode(dialogLayout_, inputColor_, this);
    }
    curMode_ = &dmcMode_;
    leftRightHolder_->show();
    break;
  case CD_IMAGE:
    if (imageMode_.isNull()) {
      imageMode_ = imageDialogMode(dialogLayout_, inputColor_, dmcOnly_,
                                   this);
    }
    curMode_ = &imageMode_;
    break;
  case CD_NEW:
    activateNewChooser();
    break;
  }
  curMode_->enable();
}
