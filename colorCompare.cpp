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

#include "colorCompare.h"

#include <QtWidgets/QScrollArea>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QMessageBox>

#include "colorLists.h"
#include "dockListWidget.h"
#include "windowManager.h"
#include "imageLabel.h"
#include "utility.h"
#include "imageUtility.h"
#include "grid.h"
#include "imageProcessing.h"
#include "leftRightAccessors.h"
#include "helpBrowser.h"
#include "dimensionComputer.h"
#include "xmlUtility.h"

// min/max user-selectable square sizes
extern const int SQUARE_SIZE_MIN = 1;
extern const int SQUARE_SIZE_MAX = 50;

colorCompare::colorCompare(const QImage& image, int index,
                           const QVector<triC>& colors, flossType type,
                           windowManager* winMgr)
  : imageCompareBase(winMgr),
    leftImage_(NULL), rightImage_(NULL), curImage_(NULL) {

  leftLabel_ = new imageLabel(this);
  leftLabel_->setMouseTracking(true);
  leftScroll()->setWidget(leftLabel());
  rightLabel_ = new imageLabel(this);
  rightLabel_->setMouseTracking(true);
  rightScroll()->setWidget(rightLabel());

  listDock_ = new dockListSwatchWidget(QVector<triC>());
  listDock_->enableContextMenu(false);
  setListDockWidget(listDock_);
  connect(rightLabel_, SIGNAL(mouseMoved(QMouseEvent* )),
          this, SLOT(processRightMouseMove(QMouseEvent*  )));
  connect(leftLabel_, SIGNAL(mouseMoved(QMouseEvent* )),
          this, SLOT(processLeftMouseMove(QMouseEvent* )));

  dimensionsAction_ = new QAction(QIcon(":dims.png"),
                                  tr("Compute dimensions"), this);
  imageMenu()->addAction(dimensionsAction_);
  imageMenu()->addAction(imageInfoAction());
  connect(dimensionsAction_, SIGNAL(triggered()),
          this, SLOT(showDimensions()));

  createSquaringOptions();
  populateToolbar();

  addImage(originalImage(), 0, QVector<triC>(), flossVariable);
  addImage(image, index, colors, type);
  setInitialSquareSize(image.size());
  // everything is hidden now, so set focus by hand
  rightScroll()->setFocus();
  bothScrollsVisible(true);
  centerSplitter();

  const QString status =
    tr("Choose an image (by clicking on it) and then click 'Square' to continue.");
  setPermanentStatus(status);
}

void colorCompare::createSquaringOptions() {

  // a spin box for the granularity of boxify
  squareSizeBox_ = new QSpinBox(this);
  squareSizeBox_->setToolTip(tr("Select the size of the squares to be created (in pixels)"));
  squareSizeBox_->setRange(SQUARE_SIZE_MIN, SQUARE_SIZE_MAX);
  squareSizeBox_->setValue(6);

  // types of squaring option box
  squareModeBox_ = new QComboBox(this);
  squareModeBox_->setToolTip(tr("Select the method for assigning colors to squares"));
  connect(squareModeBox_, SIGNAL(currentIndexChanged(int )),
          this, SLOT(changeProcessMode(int )));
  squareModeBox_->addItem(tr("Median"), QVariant(SQ_MEDIAN));
  squareModeBox_->setItemData(0, tr("Make the new color of a given square the \"average\" of the original colors in that square"), Qt::ToolTipRole);
  squareModeBox_->addItem(tr("Mode"), QVariant(SQ_MODE));
  squareModeBox_->setItemData(1, tr("Make the new color of a given square the color that occurs most often in that square"), Qt::ToolTipRole);
  squareMode_ = SQ_MEDIAN;

  squareButton_ = new QPushButton(QIcon(":square.png"),
                                  tr("Square (2/4)"), this);
  squareButton_->setShortcut(QKeySequence("Ctrl+return"));
  squareButton_->setToolTip(tr("Square the currently selected image"));
  connect(squareButton_, SIGNAL(clicked()),
          this, SLOT(processSquareButton()));
}

void colorCompare::populateToolbar() {

  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarWidget(squareModeBox_);
  addToolbarWidget(squareSizeBox_);
  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarAction(dimensionsAction_);
  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarWidget(squareButton_);
  addToolbarSeparator();
  addToolbarSeparator();
  addLeftRightToolBarBoxes();
  addToolbarSeparator();
  addToolbarSeparator();
  addLeftRightFocusButtons();
  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarZoomIcons();
  addToolbarSeparator();
  addToolbarSeparator();
}

void colorCompare::setInitialSquareSize(const QSize imageSize) {

  const int w = imageSize.width();
  const int h = imageSize.height();

  // aim for 150x150 squares = 22500 total squares
  const int totalSquaresDesired = 22500;
  int size = SQUARE_SIZE_MAX;
  while ((size > 3) && ((w/size)*(h/size) < totalSquaresDesired)) {
    --size;
  }
  squareSizeBox_->setValue(size);
}

void colorCompare::addImage(const QImage& image, int index,
                            const QVector<triC>& colors, flossType type) {

  const QString imageName = imageNameFromIndex(index);

  // always try to set the left side to the original image on a new add
  if (index != 0 &&
      (!leftImage_ || !leftImage_->isOriginal())) {
    setLeftToOriginalImage();
  }

  // create a new right image for this image
  if (index) { // not the original image
    rightImage_ =
      new mutableImageContainer(imageName, image, colors, type);
  }
  else {
    rightImage_ = new immutableImageContainer(imageName, image);
  }
  QAction* leftAction = new QAction(imageName, this);
  leftAction->setData(QVariant::fromValue(rightImage_));
  addLeftImageMenuAction(leftAction);
  QAction* rightAction = new QAction(imageName, this);
  rightAction->setData(QVariant::fromValue(rightImage_));
  addRightImageMenuAction(rightAction);

  setCur(rightImage_);
}

void colorCompare::setColorCompareVisibles(bool visible,
                                           const QVector<triC>& v) {

  squareButton_->setEnabled(visible);
  listDock_->setEnabled(visible);
  listDock_->setColorList(v);
  setZoomActionsEnabled(visible);
}

void colorCompare::setCur(imagePtr container) {

  imageCompareBase::setCur(container);
  setColorCompareVisibles(true, curImage_->colors());
}

// update the list dock color swatch with the color under mouse
void colorCompare::processLeftMouseMove(QMouseEvent* event) {

  listDock_->
    updateColorSwatch(::colorFromScaledImageCoords(event->x(),
                                                   event->y(),
                                                   leftLabel_->width(),
                                                   leftLabel_->height(),
                                                   leftImage_->image()));
}

// update the list dock color swatch with the color under mouse
void colorCompare::processRightMouseMove(QMouseEvent* event) {

  listDock_->
    updateColorSwatch(::colorFromScaledImageCoords(event->x(),
                                                   event->y(),
                                                   rightLabel_->width(),
                                                   rightLabel_->height(),
                                                   rightImage_->image()));
}

void colorCompare::changeProcessMode(int squareBoxIndex) {

  squareMode_ = squareModeBox_->itemData(squareBoxIndex).toInt();
}

void colorCompare::processSquareButton(imagePtr container,
                                       QString squareModeString,
                                       int squareSize, int newIndex) {

  int squareMode = squareMode_;
  if (!container) { // user pushed the process button
    container = curImage_;
    squareSize = squareSizeBox_->value();
  }
  else { // doing a restore
    // The saved mode string should always have been the enumeration value
    // instead of the UI string, but for backwards compatibility we'll deal with
    // it as it is.
    if (squareModeString == "median") {
      squareMode = SQ_MEDIAN;
    }
    else if (squareModeString == "mode") {
      squareMode = SQ_MODE;
    }
    else {
      qWarning() << "Unrecognized squareModeString: \"" << squareModeString << 
        "\", index: " << newIndex << "; things probably won't go well from here.";
      return;
    }
  }

  if (container) {
    QImage newImage;
    QVector<triC> colorsUsed;
    switch(squareMode) {
    case SQ_MEDIAN:
    {
      squareModeString = "median";
      grid newGrid(container->image());
      if (newGrid.empty()) {
        qWarning() << "Empty grid in process square:" <<
          container->originalWidth() << container->originalHeight();
        return;
      }
      //qDebug() << "to grid time:" << double(t.elapsed())/1000.;
      colorsUsed = ::median(&newGrid, grid(originalImage()),
                            squareSize);
      //qDebug() << "median time:" << double(t.elapsed())/1000.;
      if (!colorsUsed.empty()) {
        newImage = newGrid.toImage();
        if (newImage.isNull()) {
          qWarning() << "Empty image in process square" <<
            newGrid.width() << newGrid.height();
          return;
        }
        //qDebug() << "to QImage time: " << double(t.elapsed())/1000.;
      }
      else { // processing was cancelled
        return;
      }
      break;
    }
    case SQ_MODE:
    {
      squareModeString = "mode";
      newImage = container->image();
      colorsUsed = ::mode(&newImage, squareSize);
      //qDebug() << "mode time: " << double(t.elapsed())/1000.;
      break;
    }
    default:
    {
      qWarning() << "Square mode error:" << squareMode_;
      return;
    }
    }

    // only keep full squares on the new image; this may decrease the size
    // of the image from the original
    const int width = container->originalWidth();
    const int height = container->originalHeight();
    const int newWidth = (width/squareSize)*squareSize;
    const int newHeight = (height/squareSize)*squareSize;
    if (width != newWidth || height != newHeight) {
      newImage = newImage.copy(0, 0, newWidth, newHeight);
    }
    if (newImage.isNull()) {
      qWarning() << "Empty image in process square copy.";
      return;
    }

    const int parentIndex = imageNameToIndex(container->name());
    const squareWindowSaver saver(newIndex, parentIndex,
                                  squareModeString, squareSize);
    winManager()->addSquareWindow(newImage, squareSize, colorsUsed,
                                  container->flossMode(), saver,
                                  imageNameToIndex(container->name()),
                                  newIndex);
  }
}

QImage colorCompare::curImageForSaving() const {

  return curImage_->scaledImage();
}

void colorCompare::showDimensions() {

  dimensionComputer computer(originalImage().size(),
                             squareSizeBox_->value(), this);
  const int returnValue = computer.exec();
  if (returnValue == QDialog::Accepted) {
    squareSizeBox_->setValue(computer.getDimension());
  }
}

void colorCompare::displayImageInfo() {

  const imageLabelBase* curLabel = activeLabel();
  const int width = curLabel->width();
  const int height = curLabel->height();
  if (curImage_->isOriginal()) {
    displayOriginalImageInfo(width, height);
  }
  else {
    const flossType colorsType = ::getFlossType(curImage_->flossColors());
    QString flossString = imageInfoFlossString(colorsType);
    flossString = (flossString == "") ? "" : tr(" and ") + flossString;
    //: %4 is either empty or contains text generated elsewhere
    QMessageBox::information(this, curImage_->name(),
                             tr("%1 currently has dimensions %2x%3%4.")
                               .arg(curImage_->name())
                               .arg(::itoqs(width))
                               .arg(::itoqs(height))
                               .arg(flossString));
  }
}

void colorCompare::imageDeleted(int imageIndex) {

  winManager()->colorCompareImageDeleted(imageIndex);
}

int colorCompare::recreateImage(const squareWindowSaver& saver) {

  imagePtr thisImage = getImageFromIndex(saver.parentIndex());
  if (thisImage) {
    processSquareButton(thisImage, saver.creationMode(),
                        saver.squareDimension(), saver.index());
  }
  else {
    qWarning() << "Lost image in colorCompare::recreateImage:" <<
      saver.parentIndex();
    return -1;
  }
  return saver.hidden() ? saver.index() : -1;
}

void colorCompare::appendCurrentSettings(QDomDocument* doc,
                                         QDomElement* appendee) const {

  QDomElement settings(doc->createElement("color_compare_settings"));
  appendee->appendChild(settings);
  imageCompareBase::appendCurrentSettings(doc, &settings);
}

QString colorCompare::updateCurrentSettings(const QDomElement& xml) {

  const QDomElement settings(xml.firstChildElement("color_compare_settings"));
  if (settings.isNull()) {
    return QString();
  }

  QString errorMessage = imageCompareBase::updateCurrentSettings(settings);
  if (!errorMessage.isNull()) {
    errorMessage = "Color Compare setting error(s):<br />" + errorMessage;
  }
  return errorMessage;
}

void colorCompare::imageListEmpty() { winManager()->colorCompareEmpty(); }

helpMode colorCompare::getHelpMode() const {

  return helpMode::H_COLOR_COMPARE;
}

void colorCompare::updateImageLabelImage() {
  activeImageLabel()->updateImage(QPixmap::fromImage(curImage_->image()));
}

// these are here just so we don't have to include imageLabel.h in
// colorCompare.h (forward declare can't say imageLabel is derived from
// imageLabelBase)
imageLabelBase* colorCompare::leftLabel() const { return leftLabel_; }
imageLabelBase* colorCompare::rightLabel() const { return rightLabel_; }
