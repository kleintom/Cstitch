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

#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QPainter>
#include <QCloseEvent>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QDesktopWidget>

#include "colorLists.h"
#include "colorButton.h"
#include "imageLabel.h"
#include "imageProcessing.h"
#include "buttonGrid.h"

// a less than functor used to sort colors by distance from the input color
class triCDistanceSort {
 public:
  explicit triCDistanceSort(const triC& color) : color_(color) {}
  bool operator()(const triC& color1, const triC& color2) const {
    return ::ds(color_, color1) < ::ds(color_, color2);
  }
 private:
  const triC color_;
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

void baseDialogMode::constructList(QVBoxLayout* dialogLayout,
                                   const QVector<floss>& floss,
                                   QWidget* parent) {

  scrollArea_ = new QScrollArea(parent);
  scrollArea_->setAlignment(Qt::AlignCenter);
  dialogLayout->insertWidget(0, scrollArea_);

  QVector<triC> colors;
  if (colors.isEmpty()) {
    colors.reserve(floss.size());
    for (int i = 0; i < floss.size(); i++) {
      colors.push_back(floss[i].color());
    }
  }
  // second parameter is button icon size
  grid_ = new colorButtonGrid(floss, colors, 22, QObject::tr("Choose a floss"),
                              3, parent);
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
                 flossType type, const triC& inputColor, QWidget* parent)
  : baseDialogMode(type) {

  const int gridWidth = 10;
  setGridWidth(gridWidth);
  colors = ::rgbToColorList(colors,
                            colorTransformer::createColorTransformer(type),
                            true);
  std::sort(colors.begin(), colors.end(), triCDistanceSort(inputColor));
  constructGrid(dialogLayout, gridWidth, colors, parent);
  disable();
}

gridDialogMode::
gridDialogMode(QVBoxLayout* dialogLayout, QVector<triC> colors,
               flossType type, const triC& inputColor, QWidget* parent)
  : baseDialogMode(type) {

  const int gridWidth = qMax(static_cast<int>(ceil(sqrt(colors.size()))),
                             10);
  setGridWidth(gridWidth);
  colors = ::rgbToColorList(colors,
                            colorTransformer::createColorTransformer(type),
                            true);
  std::sort(colors.begin(), colors.end(), triCDistanceSort(inputColor));
  constructGrid(dialogLayout, gridWidth, colors, parent);
  disable();
}

fixedGridBaseDialogMode::fixedGridBaseDialogMode(QVBoxLayout* dialogLayout,
                                                 const triC& inputColor,
                                                 flossType type,
                                                 QVector<triC> colorList,
                                                 QWidget* parent)
  : baseDialogMode(type) {

  std::sort(colorList.begin(), colorList.end(), triCDistanceSort(inputColor));
  int gridWidth = qMax(static_cast<int>(ceil(sqrt(colorList.size()))), 10);
  gridWidth += gridWidth/4;
  setGridWidth(gridWidth);
  constructGrid(dialogLayout, gridWidth, colorList, parent);
  disable();
}

fixedListBaseDialogMode::fixedListBaseDialogMode(QVBoxLayout* dialogLayout,
                                                 flossType type,
                                                 QVector<floss> flossList,
                                                 QWidget* parent)
  : baseDialogMode(type) {
  setGridWidth(3);
  constructList(dialogLayout, flossList, parent);
  disable();
}

dmcGridDialogMode::dmcGridDialogMode(QVBoxLayout* dialogLayout,
                             const triC& inputColor,
                             QWidget* parent)
  : fixedGridBaseDialogMode(dialogLayout, inputColor, flossDMC, ::loadDMC(),
                            parent) {}

dmcListDialogMode::dmcListDialogMode(QVBoxLayout* dialogLayout,
                             QWidget* parent)
  : fixedListBaseDialogMode(dialogLayout, flossDMC, ::initializeDMC(),
                            parent) {}

anchorGridDialogMode::anchorGridDialogMode(QVBoxLayout* dialogLayout,
                             const triC& inputColor,
                             QWidget* parent)
  : fixedGridBaseDialogMode(dialogLayout, inputColor, flossAnchor,
                            ::loadAnchor(), parent) {}

anchorListDialogMode::anchorListDialogMode(QVBoxLayout* dialogLayout,
                             QWidget* parent)
  : fixedListBaseDialogMode(dialogLayout, flossAnchor, ::initializeAnchor(),
                            parent) {}

imageDialogMode::
imageDialogMode(QVBoxLayout* dialogLayout, const triC& inputColor,
                flossType type, QWidget* parent) {

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
  const QColor baseColor = ::transformColor(inputColor, type).qc();
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
                         const triC& inputColor, flossType type,
                         int frameWidth, int frameHeight)
  : cancelAcceptDialogBase(NULL), dialogLayout_(new QVBoxLayout),
    inputColor_(inputColor), colorSelected_(inputColor), flossType_(type),
    listColors_(listColors), curMode_(NULL),
    frameWidth_(frameWidth), frameHeight_(frameHeight) {

  setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
  dialogLayout_->setSpacing(0);
  setLayout(dialogLayout_);

  constructorHelper(false, type);
  setCurMode(CD_LIST);

  setWindowTitle(tr("Select color"));
  fitDialog();
}

colorDialog::colorDialog(const QVector<triC>& squareColors,
                         const QVector<triC>& listColors,
                         const triC& inputColor, flossType type,
                         int frameWidth, int frameHeight)
  : cancelAcceptDialogBase(NULL), dialogLayout_(new QVBoxLayout),
    inputColor_(inputColor), colorSelected_(inputColor), flossType_(type),
    listColors_(listColors),
    squareColors_(::rgbToColorList(squareColors, 
                                   colorTransformer::createColorTransformer(type),
                                   true)),
    curMode_(NULL), frameWidth_(frameWidth), frameHeight_(frameHeight) {

  setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
  dialogLayout_->setSpacing(0);
  setLayout(dialogLayout_);
  if (squareColors.size() > 1) {
    constructorHelper(true, type);
    setCurMode(CD_SQUARE);
  }
  else {
    constructorHelper(false, type);
    setCurMode(CD_LIST);
  }
  setWindowTitle(tr("Select color"));
  fitDialog();
}

void colorDialog::constructorHelper(bool useSquareColors, flossType type) {

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
    modeChoiceBox_->addItem(tr("Choose a square color"),
                            QVariant::fromValue(CD_SQUARE));
  }
  modeChoiceBox_->addItem(tr("Choose a list color"),
                          QVariant::fromValue(CD_LIST));
  if (type == flossDMC || type == flossVariable) {
    modeChoiceBox_->addItem(tr("Choose a DMC color (Grid)"),
                            QVariant::fromValue(CD_DMC_GRID));
  }
  if (type == flossDMC || type == flossVariable) {
    modeChoiceBox_->addItem(tr("Choose a DMC color (List)"),
                            QVariant::fromValue(CD_DMC_LIST));
  }
  if (type == flossAnchor || type == flossVariable) {
    modeChoiceBox_->addItem(tr("Choose an Anchor color(Grid)"),
                            QVariant::fromValue(CD_ANCHOR_GRID));
  }
  if (type == flossAnchor || type == flossVariable) {
    modeChoiceBox_->addItem(tr("Choose an Anchor color(List)"),
                            QVariant::fromValue(CD_ANCHOR_LIST));
  }
  modeChoiceBox_->addItem(tr("Choose from an image"),
                          QVariant::fromValue(CD_IMAGE));
  if (type == flossVariable) {
    modeChoiceBox_->addItem(tr("Choose a new color"),
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
  // [yuk. I don't know why these are needed - if only updateGeometry()
  // worked...]
  w += 2;
  h += 2;
  QRect screenGeometry = QApplication::desktop()->screenGeometry();
  int maxW = screenGeometry.width();
  int maxH = screenGeometry.height() - 200;
  return QSize(w, h).boundedTo(QSize(maxW, maxH));
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

  emit finished(QDialog::Rejected, inputColor_, flossColor());
  close();
}

void colorDialog::processMaybeNewAcceptClick(bool fromNewChooser) {

  const flossType type =
    fromNewChooser ? flossVariable : curMode_->flossMode();
  const flossColor newColor(colorSelected_, type);
  emit finished(QDialog::Accepted, inputColor_, newColor);
  close();
}

void colorDialog::activateNewChooser() {

  hide();
  QColorDialog::setCustomColor(QColorDialog::customCount()-1,
                               inputColor_.qrgb());
  const QColor returnedColor = QColorDialog::getColor(inputColor_.qc());
  if (returnedColor.isValid()) {
    colorSelected_ = returnedColor;
    processMaybeNewAcceptClick(true);
  }
  else {
    processCancelClick();
  }
}

void colorDialog::updateMouseMove(QRgb color) {

  curMode_->updateImageColorLabel(color, false);
}

void colorDialog::updateMouseClick(QRgb color) {

  colorSelected_ = ::transformColor(color, flossType_);
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
      squareMode_ = squareDialogMode(dialogLayout_, squareColors_, flossType_,
                                     inputColor_, this);
    }
    curMode_ = &squareMode_;
    leftRightHolder_->show();
    break;
  case CD_LIST:
    if (listMode_.isNull()) {
      listMode_ = gridDialogMode(dialogLayout_, listColors_, flossType_,
                                 inputColor_, this);
    }
    curMode_ = &listMode_;
    leftRightHolder_->show();
    break;
  case CD_DMC_GRID:
    if (dmcGridMode_.isNull()) {
      dmcGridMode_ = dmcGridDialogMode(dialogLayout_, inputColor_, this);
    }
    curMode_ = &dmcGridMode_;
    leftRightHolder_->show();
    break;
  case CD_DMC_LIST:
    if (dmcListMode_.isNull()) {
      dmcListMode_ = dmcListDialogMode(dialogLayout_, this);
    }
    curMode_ = &dmcListMode_;
    leftRightHolder_->show();
    break;
  case CD_ANCHOR_GRID:
    if (anchorMode_.isNull()) {
      anchorMode_ = anchorGridDialogMode(dialogLayout_, inputColor_, this);
    }
    curMode_ = &anchorMode_;
    leftRightHolder_->show();
    break;
  case CD_ANCHOR_LIST:
    if (anchorListMode_.isNull()) {
      anchorListMode_ = anchorListDialogMode(dialogLayout_, this);
    }
    curMode_ = &anchorListMode_;
    leftRightHolder_->show();
    break;
  case CD_IMAGE:
    if (imageMode_.isNull()) {
      imageMode_ = imageDialogMode(dialogLayout_, inputColor_, flossType_,
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
