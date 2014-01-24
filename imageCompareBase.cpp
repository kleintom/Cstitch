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

#include "imageCompareBase.h"

#include <algorithm>

#include <QtCore/qmath.h>

#include <QtWidgets/QSplitter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>

#include "windowManager.h"
#include "imageLabel.h"
#include "comboBox.h"
#include "leftRightAccessors.h"
#include "imageUtility.h"

imageCompareBase::imageCompareBase(windowManager* windowMgr)
  : imageSaverWindow(tr("Colors"), windowMgr),
    leftImageSavedSize_(QSize()), rightImageSavedSize_(QSize()) {

  constructScrolling();

  splitter_ = new QSplitter;
  splitter_->addWidget(leftScroll_);
  splitter_->addWidget(rightScroll_);
  setCentralWidget(splitter_);

  leftFocusAction_ = new QAction(QIcon(":leftImage.png"),
                                tr("Focus left side"), this);
  rightFocusAction_ = new QAction(QIcon(":rightImage.png"),
                                tr("Focus right side"), this);

  leftImageListBox_ = new comboBox(this);
  leftImageListBox_->setToolTip(tr("Select the left side image"));
  rightImageListBox_ = new comboBox(this);
  rightImageListBox_->setToolTip(tr("Select the right side image"));

  addZoomActionsToImageMenu();
  dualZoomingAction_ = new QAction(tr("Dual zooming"), this);
  dualZoomingAction_->setCheckable(true);
  dualZoomingAction_->setChecked(true);

  imageMenu()->addAction(dualZoomingAction_);
  imageMenu()->addAction(dualScrollingAction_);

  switchAction_ = imageMenu()->addAction(tr("Switch sides"));
  centerSplitterAction_ = imageMenu()->addAction(tr("Center splitter"));

  leftImageMenu_ = new QMenu(tr("&Left Image"), this);
  menuBar()->insertMenu(helpMenu()->menuAction(), leftImageMenu_);
  leftShowHide_ = leftImageMenu_->addAction(tr("Show left image"));
  leftShowHide_->setCheckable(true);
  leftDelete_ = leftImageMenu_->addAction(tr("Delete left image"));
  leftImageMenu_->addSeparator();

  rightImageMenu_ = new QMenu(tr("&Right Image"), this);
  menuBar()->insertMenu(helpMenu()->menuAction(), rightImageMenu_);
  rightShowHide_ = rightImageMenu_->addAction(tr("Show right image"));
  rightShowHide_->setCheckable(true);
  rightDelete_ = rightImageMenu_->addAction(tr("Delete right image"));
  rightImageMenu_->addSeparator();

  setConnections();
}

void imageCompareBase::constructScrolling() {

  leftScroll_ = new QScrollArea(this);
  leftScroll_->viewport()->setBackgroundRole(QPalette::Dark);
  leftScroll_->viewport()->setAutoFillBackground(true);
  leftScroll_->setFrameStyle(QFrame::Box|QFrame::Plain);
  leftScroll_->installEventFilter(this);
  leftScroll_->viewport()->installEventFilter(this);
  leftScroll_->hide();

  rightScroll_ = new QScrollArea(this);
  rightScroll_->viewport()->setBackgroundRole(QPalette::Dark);
  rightScroll_->viewport()->setAutoFillBackground(true);
  rightScroll_->setFrameStyle(QFrame::Box|QFrame::Plain);
  rightScroll_->installEventFilter(this);
  rightScroll_->viewport()->installEventFilter(this);
  rightScroll_->hide();

  dualScrollingAction_ = new QAction(tr("Dual scrolling"), this);
  dualScrollingAction_->setCheckable(true);
  dualScrollingAction_->setChecked(true);
  connect(dualScrollingAction_, SIGNAL(triggered(bool )),
          this, SLOT(processDualScrollingChange(bool )));
}

void imageCompareBase::setConnections() {

  connect(switchAction_, SIGNAL(triggered()),
          this, SLOT(switchSides()));
  connect(centerSplitterAction_, SIGNAL(triggered()),
          this, SLOT(centerSplitter()));
  connect(leftFocusAction_, SIGNAL(triggered()),
          this, SLOT(focusLeft()));
  connect(leftImageMenu_, SIGNAL(triggered(QAction* )),
          this, SLOT(processLeftImageMenuTrigger(QAction* )));
  connect(leftImageListBox_, SIGNAL(activated(const QString& )),
          this, SLOT(processLeftBox(const QString& )));
  connect(leftShowHide_, SIGNAL(triggered(bool )),
          this, SLOT(showHideLeft(bool )));
  connect(leftDelete_, SIGNAL(triggered()),
          this, SLOT(deleteLeft()));
  connect(rightFocusAction_, SIGNAL(triggered()),
          this, SLOT(focusRight()));
  connect(rightImageMenu_, SIGNAL(triggered(QAction* )),
          this, SLOT(processRightImageMenuTrigger(QAction* )));
  connect(rightImageListBox_, SIGNAL(activated(const QString& )),
          this, SLOT(processRightBox(const QString& )));
  connect(rightShowHide_, SIGNAL(triggered(bool )),
          this, SLOT(showHideRight(bool )));
  connect(rightDelete_, SIGNAL(triggered()),
          this, SLOT(deleteRight()));
  connect(dualZoomingAction_, SIGNAL(triggered(bool )),
          this, SLOT(updateDualZooming(bool )));

  // connect scrolling on the two images
  setScrollingConnections(true);
}

void imageCompareBase::setCur(imagePtr container) {

  setCurImage(container);

  leftRightAccessor lra(this, container);
  if (lra.valid()) {
    lra.scroll()->setLineWidth(ACTIVE_LINE_WIDTH);
    lra.oppositeScroll()->setLineWidth(INACTIVE_LINE_WIDTH);
    lra.scroll()->show();
    lra.scroll()->setFocus();
    lra.toolbarLabel()->cur();
    lra.showHideAction()->setEnabled(true);
    lra.showHideAction()->setChecked(true);
    if (!container->isOriginal()) {
      lra.deleteAction()->setEnabled(true);
    } else {
      lra.deleteAction()->setEnabled(false);
    }
    lra.imageMenu()->setEnabled(true);
    lra.imageListBox()->setEnabled(true);
    updateImageLabelImage();
    const QSize newSize = getSavedImageSize(container);
    setScaledImageSize(newSize, lra.label(), lra.image());
  }

  if (leftScroll_->isHidden() || rightScroll_->isHidden()) {
    bothScrollsVisible(false);
  } else {
    bothScrollsVisible(true);
  }
  updateImageLists();
}

void imageCompareBase::setScaledImageSize(const QSize size,
                                          imageLabelBase* label,
                                          imagePtr image) {

  label->setImageSize(image->setScaledSize(size));
  updateZoomValues();
}

void imageCompareBase::setScaledImageWidth(int width, imageLabelBase* label,
                                           imagePtr image) {

  label->setImageWidth(width);
  image->setScaledSize(label->size());
  updateZoomValues();
}

void imageCompareBase::setScaledImageHeight(int height, imageLabelBase* label,
                                            imagePtr image) {

  label->setImageHeight(height);
  image->setScaledSize(label->size());
  updateZoomValues();
}

QSize imageCompareBase::getSavedImageSize(const imagePtr newImage) {

  if (curImage() == rightImage()) {
    if (rightImageSavedSize_.isEmpty()) {
      rightImageSavedSize_ = newImage->scaledSize();
    }
    return rightImageSavedSize_;
  }
  else {
    if (leftImageSavedSize_.isEmpty()) {
      leftImageSavedSize_ = newImage->scaledSize();
    }
    return leftImageSavedSize_;
  }
}

void imageCompareBase::setSavedImageSize() {

  // set from labels, not images (square images are unreliable here)
  rightImageSavedSize_ = rightLabel()->size();
  leftImageSavedSize_ = leftLabel()->size();
}

void imageCompareBase::bothScrollsVisible(bool b) {

  centerSplitterAction_->setEnabled(b);
  switchAction_->setEnabled(b);
  if (b) {
    leftShowHide_->setEnabled(b);
    rightShowHide_->setEnabled(b);
  }
  else {
    // don't allow hiding of the only visible image
    if (leftScroll_->isVisible()) {
      leftShowHide_->setEnabled(false);
    }
    if (rightScroll_->isVisible()) {
      rightShowHide_->setEnabled(false);
    }
  }
}

void imageCompareBase::updateImageLists() {

  updateImageListsLR(leftRightAccessor(this, IC_LEFT));
  updateImageListsLR(leftRightAccessor(this, IC_RIGHT));
}

void imageCompareBase::updateImageListsLR(const leftRightAccessor& lra) {

  QString imageName;
  if (lra.image()) {
    imageName = lra.image()->name();
  }

  QString oppositeName;
  if (lra.oppositeImage()) {
    oppositeName = lra.oppositeImage()->name();
  }

  lra.imageListBox()->clear();
  QList<QAction*> actions = lra.imageMenu()->actions();

  QString thisString;
  QVariant thisVariant;
  for (QList<QAction*>::iterator it = actions.begin(), end = actions.end();
      it != end; ++it) {
    thisVariant = (*it)->data();
    if (thisVariant.canConvert<imagePtr>()) {
      thisString = thisVariant.value<imagePtr>()->name();
      if (thisString == oppositeName) {
        // can't show the same image on both sides
        (*it)->setEnabled(false);
      }
      else {
        (*it)->setEnabled(true);
        lra.imageListBox()->addItem(thisString);
      }
    }
  }
  // set to empty if imageName == ""
  lra.imageListBox()->setCurrentIndex(lra.imageListBox()->findText(imageName));
  lra.focusAction()->setEnabled(lra.image() ? true : false);
}

void imageCompareBase::setLeftToOriginalImage() {

  imagePtr container =
    getImageFromIndex(imageNameToIndex(Original()));

  if (container) {
    setLeftImage(container);
    setCur(container);
  }
}

void imageCompareBase::addLeftRightToolBarBoxes() {

  LLabel_ = new stateLabel("L");
  LLabel_->off();
  addToolbarWidget(LLabel_);
  addToolbarWidget(leftImageListBox_);
  addToolbarSeparator();

  RLabel_ = new stateLabel("R");
  RLabel_->off();
  addToolbarWidget(RLabel_);
  addToolbarWidget(rightImageListBox_);

  LLabel_->setBuddy(RLabel_);
  RLabel_->setBuddy(LLabel_);
}

void imageCompareBase::addLeftRightFocusButtons() {

  addToolbarAction(leftFocusAction_);
  addToolbarAction(rightFocusAction_);
}

void imageCompareBase::originalSize() {

  zoomToOriginalHelperFunction helper(this);
  zoomToHelper(helper);
}

void imageCompareBase::originalSizeHelper(const leftRightAccessor& lra,
                                          const QSize& ) {

  lra.label()->setImageSize
    (lra.image()->setScaledSize(originalImage().size()));
}

void imageCompareBase::zoom(int zoomIncrement) {

  // we do it the squareWindow way to avoid code duplication
  const bool zoomIn = (zoomIncrement > 0);
  activeLabel()->setImageSize(curImage()->zoom(zoomIn));
  if (dualZoomingActive()) {
    inactiveLabel()->setImageSize(curImage()->scaledSize());
  }
  updateZoomValues();
}

void
imageCompareBase::zoomToHelper(const zoomHelperFunction& helperFunction) {

  leftRightAccessor first(this, curImage()); // process first
  leftRightAccessor second(this, inactiveImage());
  const QSize scrollSize = activeScroll()->maximumViewportSize();
  // always process the non-original image first [this is a nod to
  // squareImage in order to prevent code duplication]
  if (dualZoomingAction_->isChecked() && curImage()->isOriginal()) {
    first = leftRightAccessor(this, inactiveImage());
    second = leftRightAccessor(this, curImage());
  }
  helperFunction(first, scrollSize);
  if (dualZoomingAction_->isChecked()) {
    const QSize newSize =
      second.image()->setScaledSize(first.label()->size());
    second.label()->setImageSize(newSize);
  }
  updateZoomValues();
}

void imageCompareBase::zoomToWidth() {

  zoomToWidthHelperFunction helper(this);
  zoomToHelper(helper);
}

void imageCompareBase::zoomToWidthHelper(const leftRightAccessor& lra,
                                         const QSize& scrollSize) {

  const QSize imageSize = lra.image()->originalSize();
  const int newWidth =
    ::computeMaxZoomWidth(scrollSize, imageSize, ::scrollbarWidth());
  if (newWidth != -1) {
    setScaledImageWidth(newWidth, lra.label(), lra.image());
  }
  else {
    setScaledImageHeight(scrollSize.height(), lra.label(), lra.image());
  }
}

void imageCompareBase::zoomToHeight() {

  zoomToHeightHelperFunction helper(this);
  zoomToHelper(helper);
}

void imageCompareBase::zoomToHeightHelper(const leftRightAccessor& lra,
                                          const QSize& scrollSize) {

  const QSize imageSize = lra.image()->originalSize();
  const int newHeight =
    ::computeMaxZoomHeight(scrollSize, imageSize, ::scrollbarWidth());
  if (newHeight != -1) {
    setScaledImageHeight(newHeight, lra.label(), lra.image());
  }
  else {
    setScaledImageWidth(scrollSize.width(), lra.label(), lra.image());
  }
}

void imageCompareBase::zoomToImage() {

  zoomToImageHelperFunction helper(this);
  zoomToHelper(helper);
}

void imageCompareBase::zoomToImageHelper(const leftRightAccessor& lra,
                                         const QSize& scrollSize) {

  const QSize imageSize = lra.image()->originalSize();
  // how high would the image be if we made it full scroll width?
  const int newHeight =
    qCeil(scrollSize.width()
          * static_cast<qreal>(imageSize.height())/imageSize.width());
  if (newHeight <= scrollSize.height()) {
    setScaledImageWidth(scrollSize.width(), lra.label(), lra.image());
  }
  else {
    setScaledImageHeight(scrollSize.height(), lra.label(), lra.image());
  }
}

void imageCompareBase::updateScrollValues() {

  if (dualScrollingAction_->isChecked() && leftImage() && rightImage()) {
    if (curImage() == leftImage()) {
      processLHSBsliderMoved(leftScroll_->horizontalScrollBar()->value());
      processLVSBsliderMoved(leftScroll_->verticalScrollBar()->value());
    }
    else {
      processRHSBsliderMoved(rightScroll_->horizontalScrollBar()->value());
      processRVSBsliderMoved(rightScroll_->verticalScrollBar()->value());
    }
  }
}

QSize imageCompareBase::curImageViewSize() const {

  imageLabelBase* curLabel = NULL;
  if (curImage() == leftImage()) {
    curLabel = leftLabel();
  }
  else if (curImage() == rightImage()) {
    curLabel = rightLabel();
  }

  return curLabel ? QSize(curLabel->width(), curLabel->height()) : QSize();
}

bool imageCompareBase::eventFilter(QObject* watched, QEvent* event) {

  // make sure we setCur on an image click
  if (event->type() == QEvent::FocusIn) {
    if (watched == leftScroll_) {
      setCur(leftImage());
    }
    else {
      setCur(rightImage());
    }
  }
  else if (event->type() == QEvent::KeyPress) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    return filterKeyEvent(keyEvent);
  }
  else if (event->type() == QEvent::Wheel) {
    QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
    // shift scroll should give a horizontal scroll
    if (wheelEvent->modifiers() == Qt::ShiftModifier) {
      // must check both the scroll and its viewport
      if (watched == rightScroll_ || watched == rightScroll_->viewport()) {
        rightScroll_->horizontalScrollBar()->event(event);
        return true;
      }
      else if (watched == leftScroll_ || watched == leftScroll_->viewport()) {
        leftScroll_->horizontalScrollBar()->event(event);
        return true;
      }
    }
  }
  return imageSaverWindow::eventFilter(watched, event);
}

bool imageCompareBase::horizontalWheelScrollEvent(QObject* watched,
                                                  QWheelEvent* event) const{

  // must check both the scroll and its viewport
  if (watched == rightScroll_ || watched == rightScroll_->viewport()) {
    rightScroll_->horizontalScrollBar()->event(event);
    return true;
  }
  else if (watched == leftScroll_ || watched == leftScroll_->viewport()) {
    leftScroll_->horizontalScrollBar()->event(event);
    return true;
  }
  return false;
}  

bool imageCompareBase::filterKeyEvent(QKeyEvent* event) {

  if (event->key() == Qt::Key_Left &&
      (event->modifiers() & Qt::ShiftModifier) && leftImage() &&
      leftScroll_->isVisible() && curImage() != leftImage()) {
    setCur(leftImage());
    return true;
  }
  else if (event->key() == Qt::Key_Right &&
           (event->modifiers() & Qt::ShiftModifier) && rightImage() &&
           rightScroll_->isVisible() && curImage() != rightImage()) {
    setCur(rightImage());
    return true;
  }
  else if ((event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
           && (event->modifiers() & Qt::ShiftModifier)
           && (leftScroll_->isVisible() || rightScroll_->isVisible())) {
    comboBox* curBox =
      (leftImage() == curImage()) ? leftImageListBox_ : rightImageListBox_;
    if (event->key() == Qt::Key_Up) {
      curBox->moveToPreviousItem();
    }
    else {
      curBox->moveToNextItem();
    }
    return true;
  }
  return false;
}

void imageCompareBase::processBoxLR(const leftRightAccessor& lra,
                                    const QString& imageName) {

  QAction* action =
    ::actionFromImageName(lra.imageMenu()->actions(),
                          findActionName<imagePtr>(imageName));
  if (action) {
    processImageMenuTriggerLR(action, lra);
  }
}

void imageCompareBase::processLeftBox(const QString& imageName) {

  processBoxLR(leftRightAccessor(this, IC_LEFT), imageName);
}

void imageCompareBase::processRightBox(const QString& imageName) {

  processBoxLR(leftRightAccessor(this, IC_RIGHT), imageName);
}

void imageCompareBase::
processImageMenuTriggerLR(QAction* action, const leftRightAccessor& lra) {

  if (!action->data().isNull()) {
    imagePtr container = action->data().value<imagePtr>();
    if (lra.image() == leftImage()) {
      setLeftImage(container);
    }
    else if (lra.image() == rightImage()) {
      setRightImage(container);
    }
    else {
      qWarning() << "Left/right mismatch:" << lra.image() <<
        leftImage() << rightImage();
      return;
    }
    setCur(container);
  }
}

void imageCompareBase::processLeftImageMenuTrigger(QAction* action) {

  processImageMenuTriggerLR(action,
                            leftRightAccessor(this, IC_LEFT));
}

void imageCompareBase::processRightImageMenuTrigger(QAction* action) {

  processImageMenuTriggerLR(action,
                            leftRightAccessor(this, IC_RIGHT));
}

void imageCompareBase::focusLR(const leftRightAccessor& lra) {

  if (lra.image()) {
    setCur(lra.image());
  }
}

void imageCompareBase::focusLeft() {

  focusLR(leftRightAccessor(this, IC_LEFT));
}

void imageCompareBase::focusRight() {

  focusLR(leftRightAccessor(this, IC_RIGHT));
}

void imageCompareBase::showLR(const leftRightAccessor& lra) {

  if (lra.image()) {
    setCur(lra.image());
  } else {
    qWarning() << "No image on show";
  }
}

void imageCompareBase::hideLR(const leftRightAccessor& lra) {

  lra.scroll()->hide();
  lra.toolbarLabel()->off();
  lra.showHideAction()->setEnabled(true);
  lra.showHideAction()->setChecked(false);
  if (lra.oppositeScroll()->isVisible()) {
    setCur(lra.oppositeImage());
  }
  else {
    qWarning() << "Failure in hideLR" << lra.image()->name();
    showLR(lra);
  }
}

void imageCompareBase::showHideLeft(bool show) {

  if (show) {
    showLR(leftRightAccessor(this, IC_LEFT));
  }
  else {
    hideLR(leftRightAccessor(this, IC_LEFT));
  }
}

void imageCompareBase::showHideRight(bool show) {

  if (show) {
    showLR(leftRightAccessor(this, IC_RIGHT));
  }
  else {
    hideLR(leftRightAccessor(this, IC_RIGHT));
  }
}

void imageCompareBase::deleteImageActions(const QString& imageName) {

  QAction* leftAction =
    ::actionFromImageName(leftImageMenu_->actions(),
                          findActionName<imagePtr>(imageName));
  QAction* rightAction =
    ::actionFromImageName(rightImageMenu_->actions(),
                          findActionName<imagePtr>(imageName));
  // delete automatically removes the action from any menus it's on
  delete leftAction;
  delete rightAction;
  imageDeleted(imageNameToIndex(imageName));
}

void imageCompareBase::deleteLR(const leftRightAccessor& lra) {

  // don't delete original
  const QString imageName = lra.image()->name();
  if (!lra.image()->isOriginal()) {
    deleteImageActions(imageName);
    // determine the replacement image
    comboBox* box = lra.imageListBox();
    box->removeItem(box->findText(imageName));
    QString replacement;
    if (box->findText(Original()) != -1) {
      replacement = Original();
    }
    else if (box->count() > 0) {
      // the most recent image
      replacement = box->itemText(box->count() - 1);
    }
    if (!replacement.isNull()) {
      if (leftImage() && leftImage()->name() == imageName) {
        setLeftImage(getImageFromName(replacement));
        setCur(leftImage());
      }
      else if (rightImage() && rightImage()->name() == imageName) {
        setRightImage(getImageFromName(replacement));
        setCur(rightImage());
      }
      else {
        qWarning() << "Deleting an image not shown:" << imageName;
      }
    }
    else {
      imageListEmpty();
      return;
    }
  }
}

void imageCompareBase::deleteLeft() {

  deleteLR(leftRightAccessor(this, IC_LEFT));
}

void imageCompareBase::deleteRight() {

  deleteLR(leftRightAccessor(this, IC_RIGHT));
}

void imageCompareBase::centerSplitter() {

  // [inexplicably, qsplitter doesn't honor small sizes, like 7,7, by which
  // I mean the actual splitter sizes set by those inputs are not equal -
  // I'm ignoring that case]
  QList<int> splitterWidths;
  splitterWidths << (leftImage() ? leftImage()->scaledWidth() : 0)
                 << (rightImage() ? rightImage()->scaledWidth() : 0);
  splitter_->setSizes(splitterWidths);
}

void imageCompareBase::switchSides() {

  if (leftScroll_->isVisible() && rightScroll_->isVisible()) {
    imagePtr tmpContainer;

    tmpContainer = rightImage();
    setRightImage(leftImage());
    setLeftImage(tmpContainer);

    // setCur on both sides to make sure they're both updated
    if (curImage() == leftImage()) {
      setCur(rightImage());
      setCur(leftImage());
    }
    else {
      setCur(leftImage());
      setCur(rightImage());
    }
  }
}

// this probably isn't what you want - some active subwidget will probably
// capture key strokes before they ever get here.  Use eventFilters instead
void imageCompareBase::keyPressEvent(QKeyEvent* event) {

  imageZoomWindow::keyPressEvent(event);
}

imagePtr imageCompareBase::
getImageFromName(const QString& imageName) const {

  return getImageFromIndex(imageNameToIndex(imageName));
}

imagePtr imageCompareBase::getImageFromIndex(int index) const {

  const QAction* actionMatch =
    ::actionFromImageName(leftImageMenu_->actions(),
                          findActionName<imagePtr>(imageNameFromIndex(index)));
  if (actionMatch) {
    return actionMatch->data().value<imagePtr>();
  }
  else {
    return imagePtr(NULL);
  }
}

void imageCompareBase::processLHSBsliderMoved(int value) {

  const int newValue =
    convertScrollValue(leftScroll_->horizontalScrollBar()->maximum(),
                       rightScroll_->horizontalScrollBar()->maximum(),
                       value);
  rightScroll_->horizontalScrollBar()->setValue(newValue);
}

void imageCompareBase::processLHSBValue(int value) {

  if (!leftScroll_->horizontalScrollBar()->isSliderDown()) {
    processLHSBsliderMoved(value);
  }
}

void imageCompareBase::processLVSBsliderMoved(int value) {

  const int newValue =
    convertScrollValue(leftScroll_->verticalScrollBar()->maximum(),
                       rightScroll_->verticalScrollBar()->maximum(),
                       value);
  rightScroll_->verticalScrollBar()->setValue(newValue);
}

void imageCompareBase::processLVSBValue(int value) {

  if (!leftScroll_->verticalScrollBar()->isSliderDown()) {
    processLVSBsliderMoved(value);
  }
}

void imageCompareBase::processRHSBsliderMoved(int value) {

  const int newValue =
    convertScrollValue(rightScroll_->horizontalScrollBar()->maximum(),
                       leftScroll_->horizontalScrollBar()->maximum(),
                       value);
  leftScroll_->horizontalScrollBar()->setValue(newValue);
}

void imageCompareBase::processRHSBValue(int value) {

  if (!rightScroll_->horizontalScrollBar()->isSliderDown()) {
    processRHSBsliderMoved(value);
  }
}

void imageCompareBase::processRVSBsliderMoved(int value) {

  const int newValue =
    convertScrollValue(rightScroll_->verticalScrollBar()->maximum(),
                       leftScroll_->verticalScrollBar()->maximum(),
                       value);
  leftScroll_->verticalScrollBar()->setValue(newValue);
}

void imageCompareBase::processRVSBValue(int value) {

  if (!rightScroll_->verticalScrollBar()->isSliderDown()) {
    processRVSBsliderMoved(value);
  }
}

void imageCompareBase::processDualScrollingChange(bool dualScrollingOn) {

  setScrollingConnections(dualScrollingOn);

  if (dualScrollingOn) {
    if (leftImage() && rightImage()) {
      if (curImage() == leftImage()) {
        processLHSBsliderMoved(leftScroll_->horizontalScrollBar()->value());
        processLVSBsliderMoved(leftScroll_->verticalScrollBar()->value());
      }
      else {
        processRHSBsliderMoved(rightScroll_->horizontalScrollBar()->value());
        processRVSBsliderMoved(rightScroll_->verticalScrollBar()->value());
      }
    }
  }
}

void imageCompareBase::setScrollingConnections(bool checked) {

    if (checked) {
    // connect scrolling on the two images
    connect(rightScroll_->horizontalScrollBar(), SIGNAL(sliderMoved(int )),
            this, SLOT(processRHSBsliderMoved(int )));
    connect(rightScroll_->horizontalScrollBar(), SIGNAL(valueChanged(int )),
            this, SLOT(processRHSBValue(int )));
    connect(rightScroll_->verticalScrollBar(), SIGNAL(sliderMoved(int )),
            this, SLOT(processRVSBsliderMoved(int )));
    connect(rightScroll_->verticalScrollBar(), SIGNAL(valueChanged(int )),
            this, SLOT(processRVSBValue(int )));
    connect(leftScroll_->horizontalScrollBar(), SIGNAL(sliderMoved(int )),
            this, SLOT(processLHSBsliderMoved(int )));
    connect(leftScroll_->horizontalScrollBar(), SIGNAL(valueChanged(int )),
            this, SLOT(processLHSBValue(int )));
    connect(leftScroll_->verticalScrollBar(), SIGNAL(sliderMoved(int )),
            this, SLOT(processLVSBsliderMoved(int )));
    connect(leftScroll_->verticalScrollBar(), SIGNAL(valueChanged(int )),
            this, SLOT(processLVSBValue(int )));
  }
  else {
    // disconnect scrolling on the two images
    disconnect(rightScroll_->horizontalScrollBar(), SIGNAL(sliderMoved(int )),
            this, SLOT(processRHSBsliderMoved(int )));
    disconnect(rightScroll_->horizontalScrollBar(), SIGNAL(valueChanged(int )),
            this, SLOT(processRHSBValue(int )));
    disconnect(rightScroll_->verticalScrollBar(), SIGNAL(sliderMoved(int )),
            this, SLOT(processRVSBsliderMoved(int )));
    disconnect(rightScroll_->verticalScrollBar(), SIGNAL(valueChanged(int )),
            this, SLOT(processRVSBValue(int )));
    disconnect(leftScroll_->horizontalScrollBar(), SIGNAL(sliderMoved(int )),
            this, SLOT(processLHSBsliderMoved(int )));
    disconnect(leftScroll_->horizontalScrollBar(), SIGNAL(valueChanged(int )),
            this, SLOT(processLHSBValue(int )));
    disconnect(leftScroll_->verticalScrollBar(), SIGNAL(sliderMoved(int )),
            this, SLOT(processLVSBsliderMoved(int )));
    disconnect(leftScroll_->verticalScrollBar(), SIGNAL(valueChanged(int )),
            this, SLOT(processLVSBValue(int )));
  }
}

void imageCompareBase::removeImage(int imageIndex) {

  const QString imageName = imageNameFromIndex(imageIndex);
  if (leftImage() && leftImage()->name() == imageName) {
    deleteLR(leftRightAccessor(this, IC_LEFT));
  }
  else if (rightImage() && rightImage()->name() == imageName) {
    deleteLR(leftRightAccessor(this, IC_RIGHT));
  }
  else { // deleting an image not currently being shown
    deleteImageActions(imageName);
    updateImageLists();
    // if that was the last non-original image, let it be known
    const int leftCount = leftImageListBox_->count();
    const int rightCount = rightImageListBox_->count();
    if ((leftCount == 0 && rightCount == 1) ||
        (leftCount == 1 && rightCount == 0)) {
      imageListEmpty();
    }
  }
}

void imageCompareBase::updateDualZooming(bool dualZoomOn) {

  if (dualZoomOn) {
    inactiveLabel()->setImageSize
      (inactiveImage()->setScaledSize(curImage()->scaledSize()));
    updateZoomValues();
  }
}

void imageCompareBase::processFirstShow() {

  imageSaverWindow::processFirstShow();
  const QSize scrollSize = activeScroll()->maximumViewportSize();
  const QSize imageSize = curImage()->originalSize();
  const qreal scrollRatio =
    static_cast<qreal>(scrollSize.width())/scrollSize.height();
  const qreal imageRatio =
    static_cast<qreal>(imageSize.width())/imageSize.height();
  if (scrollRatio > imageRatio) {
    zoomToWidth();
  }
  else {
    zoomToHeight();
  }
}

QList<imagePtr> imageCompareBase::images() const {

  QList<imagePtr> returnList;
  const QList<QAction*> actions = leftImageMenu_->actions();
  for (int i = 0, size = actions.size(); i < size; ++i) {
    const QAction* thisAction = actions[i];
    if (!thisAction->data().isNull() &&
        thisAction->data().canConvert<imagePtr>()) {
      returnList.push_back(thisAction->data().value<imagePtr>());
    }
  }
  return returnList;
}

void imageCompareBase::addLeftImageMenuAction(QAction* action) {

  leftImageMenu_->addAction(action);
}

void imageCompareBase::addRightImageMenuAction(QAction* action) {

  rightImageMenu_->addAction(action);
}
