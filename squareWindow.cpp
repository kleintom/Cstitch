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

#include "squareWindow.h"

#include <QtWidgets/QScrollArea>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QMenu>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QColorDialog>

#include "symbolChooser.h"
#include "colorDialog.h"
#include "windowManager.h"
#include "utility.h"
#include "grid.h"
#include "imageProcessing.h"
#include "leftRightAccessors.h"
#include "squareToolDock.h"
#include "squareImageContainer.h"
#include "helpBrowser.h"
#include "squareDockWidget.h"
#include "squareTools.h"
#include "xmlUtility.h"

squareWindow::squareWindow(const QImage& newImage, int imageIndex,
                           int squareDimension, const QVector<triC>& colors,
                           flossType type, windowManager* winManager)
  : imageCompareBase(winManager),
    leftImage_(NULL), rightImage_(NULL), curImage_(NULL),
    noopTool_(this), changeOneTool_(this), changeAllTool_(this),
    fillTool_(this), detailTool_(this), curTool_(&noopTool_),
    colorChooseDialog_(NULL) {

  leftLabel_ = new squareImageLabel(this);
  leftScroll()->setWidget(leftLabel());
  rightLabel_ = new squareImageLabel(this);
  rightScroll()->setWidget(rightLabel());

  constructActionsAndMenus();
  constructDocks(colors);

  // track mouse movements for color updates etc
  rightLabel()->setMouseTracking(true);
  leftLabel()->setMouseTracking(true);

  addImage(originalImage(), 0, QVector<triC>(), flossVariable, 0);
  addImage(newImage, squareDimension, colors, type, imageIndex);
  // everything is hidden now, so addImage doesn't know what's what,
  // so set focus by hand
  rightScroll()->setFocus();
  bothScrollsVisible(true);

  centerSplitter();
  setConnections();
  setPermanentStatus(tr("Select an image (by clicking on it) and then click 'Pattern' to continue."));
}

void squareWindow::constructActionsAndMenus() {

  gridAction_ =
    new QAction(QIcon(":grid.png"), tr("Turn grid on/off"), this);
  gridAction_->setCheckable(true);
  gridAction_->setChecked(true);
  gridColorAction_ = new QAction(tr("Change grid color"), this);

  removeRareColorsAction_ = new QAction(tr("Remove rare colors"), this);

  patternButton_ = new QPushButton(QIcon(":pattern.png"),
                                   tr("Pattern (3/4)"), this);
  patternButton_->setShortcut(QKeySequence("Ctrl+return"));
  patternButton_->setToolTip(tr("Create a patterned image from the currently selected image"));

  backHistoryAction_ = new QAction(QIcon(":leftArrow.png"),
                                   tr("Undo"), this);
  backHistoryAction_->setShortcut(QKeySequence("Ctrl+z"));
  backHistoryAction_->setEnabled(false);
  forwardHistoryAction_ = new QAction(QIcon(":rightArrow.png"),
                                      tr("Redo"), this);
  forwardHistoryAction_->setShortcut(QKeySequence("Ctrl+y"));
  forwardHistoryAction_->setEnabled(false);

  imageMenu()->addAction(gridAction_);
  imageMenu()->addAction(gridColorAction_);
  imageMenu()->addAction(imageInfoAction());
  imageMenu()->addAction(removeRareColorsAction_);

  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarWidget(patternButton_);
  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarAction(gridAction_);
  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarAction(backHistoryAction_);
  addToolbarAction(forwardHistoryAction_);
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

void squareWindow::constructDocks(const QVector<triC>& colors) {

  // the tool dock
  toolDockHolder_ = new QDockWidget(tr("Tools"));
  toolDockHolder_->setFeatures(QDockWidget::NoDockWidgetFeatures);

  toolDock_ = new squareToolDock(this);
  toolDockHolder_->setWidget(toolDock_);
  addDockWidget(Qt::RightDockWidgetArea, toolDockHolder_);

  // the color list dock
  colorListDock_ = new squareDockWidget(colors, this);
  setListDockWidget(colorListDock_);

  changeAllContextAction* changeAllAction =
    new changeAllContextAction(QIcon(":changeAll.png"),
                               tr("Change color"), this);
  connect(changeAllAction,
          SIGNAL(changeAllContextTrigger(const QColor& )),
          this, SLOT(processChangeAll(const QColor& )));
  colorListDock_->addContextAction(changeAllAction, true);
  updateColorListAction_ = new QAction(tr("Update color list"), this);
  updateColorListAction_->setEnabled(false);
  imageMenu()->addAction(updateColorListAction_);
  connect(updateColorListAction_, SIGNAL(triggered()),
          this, SLOT(checkColorList()));
}

void squareWindow::setConnections() {

  connect(gridAction_, SIGNAL(triggered(bool )),
          this, SLOT(processGridChange(bool )));
  connect(gridColorAction_, SIGNAL(triggered()),
          this, SLOT(processGridColorChange()));
  connect(removeRareColorsAction_, SIGNAL(triggered()),
          this, SLOT(replaceRareColors()));
  connect(patternButton_, SIGNAL(clicked()),
          this, SLOT(processPatternButton()));
  connect(backHistoryAction_, SIGNAL(triggered()),
          this, SLOT(processBackHistoryAction()));
  connect(forwardHistoryAction_, SIGNAL(triggered()),
          this, SLOT(processForwardHistoryAction()));
  connect(toolDock_, SIGNAL(announceToolChanged(squareToolCode )),
          this, SLOT(processToolChanged(squareToolCode)));
  connect(toolDock_, SIGNAL(toolLabelColorRequested(const triC& , flossType )),
          this, SLOT(toolDockLabelColorRequested(const triC& , flossType )));
  connect(rightLabel(), SIGNAL(mouseMoved(QMouseEvent* )),
          this, SLOT(processRightMouseMove(QMouseEvent* )));
  connect(leftLabel(), SIGNAL(mouseMoved(QMouseEvent* )),
          this, SLOT(processLeftMouseMove(QMouseEvent* )));
  connect(rightLabel(), SIGNAL(announceImageClick(QMouseEvent* )),
          this, SLOT(processRightImageClick(QMouseEvent* )));
  connect(leftLabel(), SIGNAL(announceImageClick(QMouseEvent* )),
          this, SLOT(processLeftImageClick(QMouseEvent* )));
  connect(rightLabel(), SIGNAL(mouseReleased(QMouseEvent* )),
          this, SLOT(processMouseRelease(QMouseEvent* )));
  connect(leftLabel(), SIGNAL(mouseReleased(QMouseEvent* )),
          this, SLOT(processMouseRelease(QMouseEvent* )));
  connect(toolDock_, SIGNAL(addContextAction(contextColorAction* )),
          colorListDock_, SLOT(addContextAction(contextColorAction* )));
  connect(toolDock_, SIGNAL(removeContextAction(contextColorAction* )),
          colorListDock_, SLOT(removeContextAction(contextColorAction* )));
  connect(toolDock_, SIGNAL(detailCall(int )),
          this, SLOT(processDetailCall(int )));
  connect(toolDock_, SIGNAL(detailClearCall()),
          this, SLOT(processDetailClearCall()));
  connect(toolDock_, SIGNAL(toolFlossTypeChanged(flossType )),
          this, SLOT(processToolFlossTypeChanged(flossType )));
}

void squareWindow::addImage(const QImage& image, int squareDimension,
                            const QVector<triC>& colors, flossType type,
                            int index) {

  const QString name = imageNameFromIndex(index);
  // always try to set the left side to the original image on a new add
  if (index != 0 &&
      (!leftImage_ || !leftImage_->isOriginal())) {
    setLeftToOriginalImage();
  }

  // create a new right image
  if (index) { // not the original image
    rightImage_ = new mutableSquareImageContainer(name, colors, image,
                                                  squareDimension, type);
  }
  else {
    rightImage_ = new immutableSquareImageContainer(name, image);
  }

  // new menu entries
  QAction* leftMenuAction = new QAction(name, this);
  // note: rightImage() returns imagePtr, not squareImagePtr,
  // which is what's required (unfortunately QVariant can't recognize
  // QVariant<squareImageContainer*> as a QVariant<imageContainer*>)
  leftMenuAction->setData(QVariant::fromValue(rightImage()));
  addLeftImageMenuAction(leftMenuAction);
  QAction* rightMenuAction = new QAction(name, this);
  rightMenuAction->setData(QVariant::fromValue(rightImage()));
  addRightImageMenuAction(rightMenuAction);
  rightImage_->setCurrentToolFlossType(type);
  setCur(rightImage_);
  if ((!curImage_->isValid()) && curImage_->numColors()) {
    QMessageBox::information(this, tr("Info"),
                             tr("This image has %1 colors, which is too many "
                                "to produce a pattern, but you can still use "
                                "this image for comparison with other squared "
                                "images.").arg(::itoqs(curImage_->numColors())));
  }
}

void squareWindow::setCur(imagePtr container) {

  ensureDetailSquaresCleared();
  if (colorChooseDialog_) {
    colorDialogTracking(false, NULL);
    colorChooseDialog_->close();
    colorChooseDialog_ = NULL;
  }
  imageCompareBase::setCur(container);

  // updates the color list
  updateSubWidgetStates();
  activeSquareLabel()->setGridOn(gridOn());
}

void squareWindow::ensureDetailSquaresCleared() {

  // [curImage_ is only ever null on the first call to setCur]
  if (curImage_ && activeSquareLabel()->clearHashes()) {
    detailTool_.clearCache();
    toolDock_->detailListIsEmpty(true);
  }
}

void squareWindow::updateSubWidgetStates() {

  const bool curValid = curImage_->isValid();
  toolDock_->setEnabled(curValid);
  patternButton_->setEnabled(curValid);
  // an image is valid if it doesn't have too many (or no) colors
  if (curValid) {
    colorListDock_->setColorList(curImage_->colors());
    curTool_->setMouseHint();
    toolDock_->setFlossType(curImage_->getCurrentToolFlossType());
  }
  else {
    colorListDock_->setColorList(QVector<triC>());
    colorListDock_->setNumColors(curImage_->numColors());
    toolDock_->setToolToNoop();
    if (curImage_->numColors()) {
      setStatus(tr("This image has %1 colors, %2 too many for further "
                   "processing.")
                .arg(::itoqs(curImage_->numColors() -
                             symbolChooser::maxNumberOfSymbols()))
                .arg(::itoqs(curImage_->numColors())));
    }
    else {
      setStatus("");
    }
  }
  updateColorListAction_->setEnabled(curImage_->colorListCheckNeeded());
  colorListDock_->setEnabled(true);
  setSaveImageActionEnabled(true);
  zoomInAction()->setEnabled(true);
  zoomOutAction()->setEnabled(true);
  originalSizeAction()->setEnabled(true);
  updateHistoryButtonStates();
}

void squareWindow::updateHistoryButtonStates() {

  backHistoryAction_->setEnabled(curImage_->backHistory());
  forwardHistoryAction_->setEnabled(curImage_->forwardHistory());
}

void squareWindow::colorDialogTracking(bool usingColorDialog,
                                       colorDialog* dialog,
                                       bool changeAllRequest) {

  if (usingColorDialog) {
    deactivateWidgetsForColorDialog(true);
    if (changeAllRequest) { // context call for change all colors
      connect(dialog, SIGNAL(finished(int, const triC& , const flossColor& )),
              this, SLOT(changeAllDialogFinished(int, const triC& ,
                                                 const flossColor& )));
    }
    else {
      curTool_->connectChangeColorDialog(dialog);
    }
    connect(dialog, SIGNAL(colorDialogClosing()),
            this, SLOT(processColorDialogClosing()));
  }
  else {
    deactivateWidgetsForColorDialog(false);
  }
}

void squareWindow::deactivateWidgetsForColorDialog(bool b) {

  const bool bb = !b;
  setToolbarEnabled(bb);
  menuBar()->setEnabled(bb);
  colorListDock_->setEnabled(bb);
  toolDock_->setEnabled(bb);
}

bool squareWindow::eventFilter(QObject* watched, QEvent* event) {

  // Mouse clicks mean different things depending on what the current tool
  // is, so we sometimes want to ignore their associated focusIns.  For
  // example, if we're in draw mode then a click on the inactive image
  // shouldn't make it the active image, so we need to ignore that focus.
  if (event->type() == QEvent::FocusIn) {

    const QFocusEvent* focusEvent = static_cast<QFocusEvent*>(event);
    switch( focusEvent->reason() ) {
    case Qt::ActiveWindowFocusReason:
    case Qt::PopupFocusReason:
    case Qt::ShortcutFocusReason:
    case Qt::MenuBarFocusReason:
    case Qt::OtherFocusReason:
      return true; // ignore it

    case Qt::TabFocusReason:
    case Qt::BacktabFocusReason:
      // was getting a tab focus for left scroll when focus was on
      // right scroll and the clear detail button was pushed on the
      // detailWidget on the toolDock (!@#), so ignoring tabs
//      if ( watched == rightScroll() ) {
//      setCur(rightImage_);
//      }
//      else if ( watched == leftScroll() ) {
//      setCur(leftImage_);
//      }
      return true;

    case Qt::MouseFocusReason:
      // if we're using a tool then mouse clicks are for choosing colors,
      // not for changing focus
      if (colorChooseDialog_) {
        return true;
      }
      if (rightScroll()->underMouse() || leftScroll()->underMouse()) {
        if (curTool_ == &noopTool_) {
          // then click sets focus
          if (watched == rightScroll() && activeScroll() == leftScroll()) {
            setCur(rightImage_);
          }
          else if (watched == leftScroll() && 
                   activeScroll() == rightScroll()) {
            setCur(leftImage_);
          }
        }
        else {
          // focus has already been changed; we may need to change it back
          if (leftScroll()->underMouse() && curImage_ == rightImage_) {
            rightScroll()->setFocus();
          }
          else if (rightScroll()->underMouse() &&
                   curImage_ == leftImage_) {
            leftScroll()->setFocus();
          }
        }
        return true;
      }
      else { // not under mouse, so ignore it
        return true;
      }

    default:
      return false; // pass it on
    }
  }
  else if (event->type() == QEvent::KeyPress) {
    QKeyEvent* e = static_cast<QKeyEvent*>(event);
    return filterKeyEvent(e);
  }
  return imageCompareBase::eventFilter(watched, event);
}

bool squareWindow::filterKeyEvent(QKeyEvent* event) {

  // ignore key events when the color dialog is active
  if (colorChooseDialog_) {
    return true; // stop further filtering
  }
  else {
    return imageCompareBase::filterKeyEvent(event);
  }
}

bool squareWindow::gridOn() const {

  // never grid the original
  if (curImage_->isOriginal()) {
    return false;
  }
  else {
    return gridAction_->isChecked();
  }
}

void squareWindow::processGridChange(bool checked) {

  gridAction_->setChecked(checked);
  const bool gridLeft = checked && (leftImage_ != squareImagePtr(NULL)) &&
    (!leftImage_->isOriginal());
  leftLabel_->setGridOn(gridLeft);
  const bool gridRight = checked && (rightImage_ != squareImagePtr(NULL)) &&
    (!rightImage_->isOriginal());
  rightLabel_->setGridOn(gridRight);
}

void squareWindow::processGridColorChange() {

  const QColor newGridColor =
    QColorDialog::getColor(QColor(leftLabel_->gridColor()), this);
  if (newGridColor.isValid()) {
    leftLabel_->setGridColor(newGridColor.rgb());
    rightLabel_->setGridColor(newGridColor.rgb());
  }
}

void squareWindow::processToolChanged(squareToolCode currentTool) {

  switch(currentTool) {
  case T_CHANGE_ALL:
    curTool_ = &changeAllTool_;
    break;
  case T_CHANGE_ONE:
    curTool_ = &changeOneTool_;
    break;
  case T_FILL_REGION:
    curTool_ = &fillTool_;
    break;
  case T_DETAIL:
    curTool_ = &detailTool_;
    break;
  case T_NOOP:
    curTool_ = &noopTool_;
    break;
  default:
    qWarning() << "Bad tool in processToolChanged" << currentTool;
    curTool_ = &noopTool_;
  }

  //if a color dialog is open, close it when we change tools
  if (colorChooseDialog_) {
    colorDialogTracking(false, NULL);
    colorChooseDialog_->close();
    colorChooseDialog_ = NULL;
  }
  ensureDetailSquaresCleared();
  // let the user know what mouse buttons do for the current tool
  curTool_->setMouseHint();
}

void squareWindow::processPatternButton(squareImagePtr image,
                                        int patternIndex) {

  if (!image) { // user pressed the pattern button - we're live
    image = curImage_;
    ensureDetailSquaresCleared();
    checkColorList();
  }
  else { // this is a restore call
    // there are no detail squares to clear
    // checkColorList()
    image->checkColorList();
  }
  const int parentIndex = imageNameToIndex(image->name());
  const int squareDimension = image->originalDimension();
  const patternWindowSaver saver(patternIndex, parentIndex, squareDimension,
                                 image->backImageHistoryXml());
  winManager()->addPatternWindow(image->image(),
                                 squareDimension,
                                 image->flossColors(),
                                 leftLabel_->gridColor(),
                                 saver,
                                 parentIndex, patternIndex);
}

void squareWindow::checkColorList() {

  const QVector<triC> colorsToRemove = curImage_->checkColorList();
  updateColorListAction_->setEnabled(curImage_->colorListCheckNeeded());
  const int numColorsRemoved = colorsToRemove.size();
  if (numColorsRemoved > 0) {
    colorListDock_->removeColors(colorsToRemove);
    //: singular/plural
    showTemporaryStatusMessage(tr("Removed %n color(s)", "", numColorsRemoved));
  }
  else {
    showTemporaryStatusMessage(tr("The color list is up to date"));
  }
}

void squareWindow::processChangeAll(const QColor& oldColor) {

  activateColorDialog(oldColor, toolDock_->getFlossType(), QVector<triC>());
}

void squareWindow::
activateColorDialog(const triC& currentColor, flossType type,
                    const QVector<triC>& replacementColors,
                    bool changeToolColor) {

  int frameWidth, frameHeight;
  winManager()->frameWidthAndHeight(&frameWidth, &frameHeight);
  if (replacementColors.isEmpty() && !changeToolColor) {
    colorChooseDialog_ = new colorDialog(curImage_->colors(), currentColor,
                                         type, frameWidth, frameHeight);
    colorDialogTracking(true, colorChooseDialog_, true);
  }
  else {
    colorChooseDialog_ = new colorDialog(replacementColors,
                                         curImage_->colors(), currentColor,
                                         type, frameWidth, frameHeight);
    colorDialogTracking(true, colorChooseDialog_);
  }
  colorChooseDialog_->show();
}

void squareWindow::processChangeAll(const triC& oldColor,
                                    const flossColor& newColor) {

  const dockListUpdate update = curImage_->changeColor(oldColor.qrgb(),
                                                       newColor);
  curImageUpdated(update);
}

void squareWindow::processImageClickLR(const leftRightAccessor& lra,
                                       QMouseEvent* event) {

  if (curTool_ != &noopTool_) {
    if (lra.scroll()->lineWidth() == ACTIVE_LINE_WIDTH) {
      processActiveImageClick(event);
    }
    else {
      processInactiveImageClick(event);
    }
  }
}

void squareWindow::processLeftImageClick(QMouseEvent* event) {

  if (colorChooseDialog_ == NULL) {
    processImageClickLR(leftRightAccessor(this, IC_LEFT), event);
  }
  else {
    processLeftDialogMouseClick(event);
  }
}

void squareWindow::processRightImageClick(QMouseEvent* event) {

  if (colorChooseDialog_ == NULL) {
    processImageClickLR(leftRightAccessor(this, IC_RIGHT), event);
  }
  else {
    processRightDialogMouseClick(event);
  }
}

void squareWindow::processInactiveImageClick(QMouseEvent* event) {

  curTool_->inactiveImageClick(event);
}

void squareWindow::processActiveImageClick(QMouseEvent* event) {

  curTool_->activeImageClick(event);
}

void squareWindow::processRightDialogMouseMove(QMouseEvent* event) {

  processDialogMouseEvent(event, leftRightAccessor(this, IC_RIGHT), true);
}

void squareWindow::processLeftDialogMouseMove(QMouseEvent* event) {

  processDialogMouseEvent(event, leftRightAccessor(this, IC_LEFT), true);
}

void squareWindow::processRightDialogMouseClick(QMouseEvent* event) {

  processDialogMouseEvent(event, leftRightAccessor(this, IC_RIGHT), false);
}

void squareWindow::processLeftDialogMouseClick(QMouseEvent* event) {

  processDialogMouseEvent(event, leftRightAccessor(this, IC_LEFT), false);
}

void squareWindow::processDialogMouseEvent(QMouseEvent* event,
                                           const leftRightAccessor& lra,
                                           bool move) {

  // is the dialog currently in pick-from-image mode?
  if (colorChooseDialog_->modeIsImageMode()) {
    const QRgb color = ::colorFromScaledImageCoords(event->x(), event->y(),
                                                    lra.label()->width(),
                                                    lra.label()->height(),
                                                    lra.image()->image());
    if (move) {
      colorChooseDialog_->updateMouseMove(color);
    }
    else { // click
      colorChooseDialog_->updateMouseClick(color);
    }
  }
}

void squareWindow::processLeftMouseMove(QMouseEvent* event) {

  if (colorChooseDialog_ == NULL) {
    if (leftScroll()->lineWidth() == ACTIVE_LINE_WIDTH) {
      processActiveMouseMove(event);
    }
    else {
      processInactiveMouseMove(event);
    }
  }
  else {
    processLeftDialogMouseMove(event);
  }
}

void squareWindow::processRightMouseMove(QMouseEvent* event) {

  if (colorChooseDialog_ == NULL) {
    if (rightScroll()->lineWidth() == ACTIVE_LINE_WIDTH) {
      processActiveMouseMove(event);
    }
    else {
      processInactiveMouseMove(event);
    }
  }
  else {
    processRightDialogMouseMove(event);
  }
}

void squareWindow::processInactiveMouseMove(QMouseEvent* event) {

  // CAUTION: during a delete event during the time between which
  // a side is deleted and focus is changed, the inactive window
  // can refer to an empty image
  const squareImageLabel* label = inactiveSquareLabel();
  const squareImagePtr container = inactiveImage();
  // update the dock color swatch
  if (label && container) {
    const int w = label->width();
    const int h = label->height();
    const int x = event->x();
    const int y = event->y();
    const QImage& image = container->image();
    colorListDock_->updateColorSwatch(image.pixel((x*image.width())/w,
                                                  (y*image.height())/h));
  }
}

void squareWindow::processActiveMouseMove(QMouseEvent* event) {

  curTool_->activeMouseMove(event);
}

void squareWindow::processMouseRelease(QMouseEvent* ) {

  curTool_->mouseRelease();
}

void squareWindow::changeAllDialogFinished(int returnCode,
                                           const triC& oldColor,
                                           const flossColor& newColor) {

  if (returnCode == QDialog::Accepted) {
    processChangeAll(oldColor.qrgb(), newColor);
  }
}

void squareWindow::changeColorDialogFinished(int returnCode, const triC& ,
                                             const flossColor& newColor) {

  if (returnCode == QDialog::Accepted) {
    toolDock_->setToolLabelColor(newColor);
  }
}

void squareWindow::processColorDialogClosing() {

  colorDialogTracking(false, NULL);
  colorChooseDialog_ = NULL; // the dialog deleted itself on close
}

void squareWindow::requestSquareColor(int originalX, int originalY,
                                      int squareDim, const triC& oldColor) {

  // squareDim is to be used on the original image to get a reference
  // square around originalX, originalY, so don't make it too small
  if (squareDim < 5) {
    squareDim = 5;
  }

  // get a list of the colors in the _original_ square around
  // originalX, originalY
  const QImage& original = originalImage();
  const int w = original.width();
  const int h = original.height();
  const int startOfSquareX = (originalX/squareDim)*squareDim;
  const int startOfSquareY = (originalY/squareDim)*squareDim;
  QVector<triC> neighborColors;
  for (int i = 0; i < squareDim; ++i) {
    for (int j = 0; j < squareDim; ++j) {
      if (startOfSquareX + i < w && startOfSquareY + j < h) {
        const triC thisColor = triC(original.pixel(startOfSquareX + i,
                                                   startOfSquareY + j));
        if (!neighborColors.contains(thisColor)) {
          neighborColors.push_back(thisColor);
        }
      }
    }
  }
  activateColorDialog(oldColor, toolDock_->getFlossType(), neighborColors);
}

QImage squareWindow::gridedImage(const QImage& image, int originalSquareDim,
                                 int originalWidth,
                                 int originalHeight) const {

  if (!gridOn()) {
    return image;
  }
  QImage returnImage = image.copy();
  if (returnImage.isNull()) {
    qWarning() << "Empty image in squareWindow::gridedImage" <<
      image.width() << image.height();
    return image;
  }
  ::gridImage(&returnImage, originalSquareDim, originalWidth,
              originalHeight, leftLabel_->gridColor());
  return returnImage;
}

void squareWindow::updateImageLabelImage(const QRect& updateRectangle) {

  const QVector<triC>& colors = curImage_->colors();
  QList<QRgb> rgbColors;
  for (int i = 0, size = colors.size(); i < size; ++i) {
    rgbColors.push_back(colors[i].qrgb());
  }
  activeSquareLabel()->updateImage(curImage_->image(),
                                   rgbColors, updateRectangle);
}

void squareWindow::curImageUpdated(const dockListUpdate& update,
                                   const QRect& updateRectangle) {

  if (update.removeColors()) {
    colorListDock_->removeColors(update.colorsToRemove());
  }
  if (update.singleColor()) {
    if (update.colorIsNew()) {
      colorListDock_->addToList(update.color());
    }
    else if (update.color().isValid()) {
      colorListDock_->moveTo(update.color());
    }
    // else do nothing
  }
  else {
    colorListDock_->addColors(update.colors());
  }
  if (curImage_->colorListCheckNeeded()) {
    updateColorListAction_->setEnabled(true);
  }
  updateImageLabelImage(updateRectangle);
  updateHistoryButtonStates();
}

void squareWindow::processForwardHistoryAction() {

  const dockListUpdate update = curImage_->moveHistoryForward();
  curImageUpdated(update);
}

void squareWindow::processBackHistoryAction() {

  const dockListUpdate update = curImage_->moveHistoryBack();
  curImageUpdated(update);
}

QImage squareWindow::curImageForSaving() const {

  return gridedImage(curImage_->scaledImage(),
                     curImage_->originalDimension(),
                     curImage_->originalWidth(),
                     curImage_->originalHeight());
}

void squareWindow::keyPressEvent(QKeyEvent* event) {

  imageCompareBase::keyPressEvent(event);
}

void squareWindow::processDetailCall(int numColors) {

  const dockListUpdate update =
    curImage_->performDetailing(originalImage(), detailTool_.coordinates(),
                                numColors, toolDock_->getFlossType());
  ensureDetailSquaresCleared();
  curImageUpdated(update);
}

void squareWindow::processDetailClearCall() {

  ensureDetailSquaresCleared();
}

void squareWindow::displayImageInfo() {

  const squareImageLabel* curLabel = activeSquareLabel();
  const int width = curLabel->width();
  const int height = curLabel->height();
  if (curImage_->isOriginal()) {
    displayOriginalImageInfo(width, height);
  }
  else {
    checkColorList();
    const int squareDim = curImage_->originalDimension();
    const int xBoxes = curImage_->originalWidth()/squareDim;
    const int yBoxes = curImage_->originalHeight()/squareDim;
    const flossType colorsType = ::getFlossType(curImage_->flossColors());
    const QString flossString = imageInfoFlossString(colorsType);
    // for translation purposes, it seems best to not be clever about
    // splitting the cases...
    if (flossString == "") {
      QMessageBox::information(this, curImage_->name(),
                               tr("%1 currently has dimensions %2x%3, box "
                                  "dimension %4, and is %5 by %6 boxes.")
                               .arg(curImage_->name())
                               .arg(::itoqs(width))
                               .arg(::itoqs(height))
                               .arg(::itoqs(squareDim))
                               .arg(::itoqs(xBoxes))
                               .arg(::itoqs(yBoxes)));
    }
    else {
      QMessageBox::information(this, curImage_->name(),
                               tr("%1 currently has dimensions %2x%3, box "
                                  "dimension %4, is %5 by %6 boxes, and %7.")
                               .arg(curImage_->name())
                               .arg(::itoqs(width))
                               .arg(::itoqs(height))
                               .arg(::itoqs(squareDim))
                               .arg(::itoqs(xBoxes))
                               .arg(::itoqs(yBoxes))
                               .arg(flossString));
                                    
    }
  }
}

void squareWindow::imageDeleted(int imageIndex) {

  winManager()->squareWindowImageDeleted(imageIndex);
}

void squareWindow::writeCurrentHistory(QDomDocument* doc,
                                       QDomElement* appendee,
                                       int imageIndex) {

  squareImagePtr container = squareImageFromIndex(imageIndex);
  if (container) {
    container->writeImageHistory(doc, appendee);
  }
  else {
    qWarning() << "Lost image in imageHistoryFromIndex:" << imageIndex;
  }
}

void squareWindow::updateImageHistory(const QDomElement& xml) {

  const int imageIndex = ::getElementText(xml, "index").toInt();
  squareImagePtr container = squareImageFromIndex(imageIndex);
  if (container) {
    container->updateImageHistory(xml);
  }
  else {
    qWarning() << "Lost image in updateImageHistory:" << imageIndex;
  }
}

void squareWindow::recreatePatternImage(const patternWindowSaver& saver) {

  squareImagePtr thisImage =
    squareImageFromIndex(saver.parentIndex());
  if (thisImage) {
    QDomElement historyElement(saver.squareHistory().
                               firstChildElement("square_history"));
    thisImage->updateImageHistory(historyElement);
    processPatternButton(thisImage, saver.index());
    thisImage->rewindAndClearHistory();
  }
  else {
    qWarning() << "Lost image in recreatePatternWindow:" <<
      saver.parentIndex();
  }
}

void squareWindow::appendCurrentSettings(QDomDocument* doc,
                                         QDomElement* appendee) const {

  QDomElement settings(doc->createElement("square_window_settings"));
  appendee->appendChild(settings);
  imageCompareBase::appendCurrentSettings(doc, &settings);
  ::appendTextElement(doc, "grid_on",
                      ::boolToString(gridAction_->isChecked()), &settings);
  ::appendTextElement(doc, "grid_color",
                      ::rgbToString(leftLabel_->gridColor()), &settings);
}

QString squareWindow::updateCurrentSettings(const QDomElement& xml) {

  QDomElement settings(xml.firstChildElement("square_window_settings"));
  if (settings.isNull()) {
    return QString();
  }
  const QRgb gridColor(::xmlStringToRgb(::getElementText(settings,
                                                         "grid_color")));
  leftLabel_->setGridColor(gridColor);
  rightLabel_->setGridColor(gridColor);
  processGridChange(::stringToBool(::getElementText(settings, "grid_on")));

  QString errorMessage = imageCompareBase::updateCurrentSettings(settings);
  if (!errorMessage.isNull()) {
    errorMessage = "Square Window setting error(s):<br />" + errorMessage;
  }
  return errorMessage;
}

void squareWindow::imageListEmpty() { winManager()->squareWindowEmpty(); }

helpMode squareWindow::getHelpMode() const {

  return helpMode::H_SQUARE;
}

void squareWindow::setScaledImageWidth(int width, imageLabelBase* label,
                                       imagePtr image) {

  squareImagePtr squareImage = (image == leftImage_) ?
    leftImage_ : rightImage_;
  label->setImageSize(squareImage->setScaledWidth(width));
  updateZoomValues();
}

void squareWindow::setScaledImageHeight(int height, imageLabelBase* label,
                                        imagePtr image) {

  squareImagePtr squareImage = (image == leftImage_) ?
    leftImage_ : rightImage_;
  label->setImageSize(squareImage->setScaledHeight(height));
  updateZoomValues();
}

void squareWindow::updateDualZooming(bool dualZoomOn) {

  // [this only ever happens during construction...]
  if (!leftImage_ || !rightImage_) {
    return;
  }
  // allow inactive image to be updated
  inactiveImage()->resetZoom();
  if (!curImage_->isOriginal()) {
    imageCompareBase::updateDualZooming(dualZoomOn);
  }
  else {
    // when dual zooming is on, we restrict the original image to sizes
    // acceptable for the nonactive squared image (not so when dual
    // zooming is off), so enforce that now
    inactiveLabel()->setImageSize
      (inactiveImage()->setScaledSize(curImage_->scaledSize()));
    activeLabel()->
      setImageSize(curImage_->setScaledSize(inactiveImage()->scaledSize()));
    updateZoomValues();
  }
}

void squareWindow::zoom(int zoomIncrement) {

  const bool zoomIn = (zoomIncrement > 0);
  if (dualZoomingActive()) {
    // if either side is the original image, set the non-original side
    // first and then set original off of that
    if (leftImage()->isOriginal() || rightImage()->isOriginal()) {
      imageLabelBase* originalSide;
      imageLabelBase* nonOriginalSide;
      imagePtr originalImage;
      imagePtr nonOriginalImage;
      if (leftImage()->isOriginal()) {
        originalSide = leftLabel();
        originalImage = leftImage();
        nonOriginalSide = rightLabel();
        nonOriginalImage = rightImage();
      }
      else {
        originalSide = rightLabel();
        originalImage = rightImage();
        nonOriginalSide = leftLabel();
        nonOriginalImage = leftImage();
      }
      nonOriginalSide->setImageSize(nonOriginalImage->zoom(zoomIn));
      originalSide->setImageSize
        (originalImage->setScaledSize(nonOriginalImage->scaledSize()));
    }
    else { // neither side is the original image
      // Note: don't just zoom both sides here, because repeated zooming
      // on images with different square sizes will produce large
      // differences
      activeLabel()->setImageSize(curImage_->zoom(zoomIn));
      inactiveImage()->resetZoom();
      inactiveLabel()->setImageSize
        (inactiveImage()->setScaledSize(curImage_->scaledSize()));
    }
  }
  else { // just zoom the active side
    activeLabel()->setImageSize(curImage()->zoom(zoomIn));
  }
  resetZooms(true);
  updateZoomValues();
}

void squareWindow::resetZooms(bool excludeCurs) {

  // note: exclude both left and right, not just cur, since the inactive
  // scale needs to be saved for zooming when cur is the original image
  imagePtr left;
  imagePtr right;
  if (excludeCurs) {
    left = leftImage_;
    right = rightImage_;
  }

  const QSize nullSize(0, 0);
  const QList<imagePtr> images = imageCompareBase::images();
  for (int i = 0, size = images.size(); i < size; ++i) {
    imagePtr thisImage = images[i];
    if (thisImage != left && thisImage != right) {
      thisImage->setScaledSize(nullSize);
    }
  }
}

void squareWindow::replaceRareColors() {

  const dockListUpdate update = curImage_->replaceRareColors();
  curImageUpdated(update);
}

void squareWindow::updateImageLabelImage() {

  const QVector<triC>& colors = curImage_->colors();
  QList<QRgb> rgbColors;
  for (int i = 0, size = colors.size(); i < size; ++i) {
    rgbColors.push_back(colors[i].qrgb());
  }
  activeSquareLabel()->setNewImage(curImage_->image(),
                                   rgbColors,
                                   curImage_->xSquareCount(),
                                   curImage_->ySquareCount(),
                                   curImage_->isOriginal());
}

void squareWindow::toolDockLabelColorRequested(const triC& currentColor,
                                               flossType type) {

  activateColorDialog(currentColor, type, QVector<triC>(), true);
}

void squareWindow::processToolFlossTypeChanged(flossType newType) {

  curImage_->setCurrentToolFlossType(newType);
}

void squareWindow::checkAllColorLists() {

  const QList<imagePtr> curImages = images();
  for (int i = 0, size = curImages.size(); i < size; ++i) {
    squareImagePtr thisImage(curImages[i]->squareContainer());
    if (thisImage == curImage_) {
      checkColorList();
    }
    else {
      thisImage->checkColorList();
    }
  }
}
