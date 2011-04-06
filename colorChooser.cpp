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

#include "colorChooser.h"

#include <QtCore/qmath.h>

#include <QtGui/QSpinBox>
#include <QtGui/QScrollArea>
#include <QtGui/QDockWidget>
#include <QtGui/QMenu>
#include <QtGui/QComboBox>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

#include "utility.h"
#include "imageLabel.h"
#include "dockListWidget.h"
#include "imageUtility.h"
#include "imageProcessing.h"
#include "colorChooserProcessModes.h"
#include "windowManager.h"
#include "helpBrowser.h"
#include "symbolChooser.h"
#include "xmlUtility.h"
#include "windowSavers.h"

extern const int DOCK_WIDTH;

// winMgr is used to store the currently loaded image (application wide)
// and is where we send our new image when the user clicks "Process",
// among other things.
colorChooser::colorChooser(windowManager* winMgr)
  : imageZoomWindow("", winMgr), processMode_(), clickedDock_(NULL) {

  imageLabel_ = new imageLabel(this);
  connect(imageLabel_, SIGNAL(announceImageClick(QMouseEvent* )),
          this, SLOT(processColorAdd(QMouseEvent* )));

  imageScroll_ = new QScrollArea(this);
  imageScroll_->setWidget(imageLabel_);
  setCentralWidget(imageScroll_);

  constructMenuObjects();
  constructProcessingObjects();
  popDock();
  processProcessChange(0); // set processMode_ to the first box entry

  setStatus(tr("Click the left folder icon to open a new image or ") +
            tr("the right folder icon to open a saved project."));
  setPermanentStatus(tr("Click 'Choose colors' to continue."));
  setPermanentStatusEnabled(false);

  setWidgetActive(false);
  winManager()->addColorChooserWindow(this);
}

void colorChooser::constructMenuObjects() {

  clearListAction_ = new QAction(tr("Clear \"Clicked colors\" list"), this);
  connect(clearListAction_, SIGNAL(triggered()),
          this, SLOT(clearList()));

  addZoomActionsToImageMenu();
  imageMenu()->addAction(imageInfoAction());
  imageMenu()->addAction(clearListAction_);
}

void colorChooser::constructProcessingObjects() {

  processButton_ = new QPushButton(QIcon(":colorCompare.png"),
                                   tr("Choose colors (1/4)"), this);
  processButton_->setShortcut(QKeySequence("Ctrl+return"));
  processButton_->
    setToolTip(tr("Process this image and move to color compare (2/4)"));
  // a spin box to choose the number of colors for chooseNumColors
  numColorsBox_ = new QSpinBox(this);
  numColorsBox_->setToolTip(tr("The number of colors to choose"));
  numColorsBox_->setRange(0, symbolChooser::maxNumberOfSymbols());
  numColorsBox_->setValue(80);
  numColorsBox_->setEnabled(false);

  // the processing type chooser
  processModeBox_ = new QComboBox(this);
  processModeBox_->setToolTip(tr("Select the mode used to choose colors"));
  // first is the item text, second is the toolTip for that item
  QList<QStringPair> modeStrings = processMode_.modeStrings();
  for (int i = 0, size = modeStrings.size(); i < size; ++i) {
    processModeBox_->addItem(modeStrings[i].first);
    processModeBox_->setItemData(i, modeStrings[i].second, Qt::ToolTipRole);
  }

  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarWidget(processModeBox_);
  addToolbarWidget(numColorsBox_);
  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarWidget(processButton_);
  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarZoomIcons();
  addToolbarSeparator();
  addToolbarSeparator();

  const int boxHeight = numColorsBox_->height();
  processButton_->resize(processButton_->width(), boxHeight);
  processModeBox_->resize(processModeBox_->width(), boxHeight);

  connect(processModeBox_, SIGNAL(currentIndexChanged(int )),
          this, SLOT(processProcessChange(int )));
  connect(processButton_, SIGNAL(clicked()),
          this, SLOT(processProcessing()));

}

void colorChooser::popDock() {

  clickedDock_ = new dockListSwatchWidget(QVector<triC>(), this);
  setListDockWidget(clickedDock_);
  connect(clickedDock_, SIGNAL(colorRemoved(const triC& )),
          this, SLOT(removeColor(const triC& )));
  connect(imageLabel_, SIGNAL(mouseMoved(QMouseEvent* )),
          this, SLOT(processMouseMove(QMouseEvent* )));
  generatedDock_ = new dockListWidget(QVector<triC>(), this);
  generatedDock_->enableContextMenu(false);
  generatedDockHolder_ = new QDockWidget(tr("Generated colors"), this);
  generatedDockHolder_->setFeatures(QDockWidget::NoDockWidgetFeatures);
  generatedDockHolder_->setWidget(generatedDock_);
  addDockWidget(Qt::RightDockWidgetArea, generatedDockHolder_);
}

void colorChooser::processMouseMove(QMouseEvent* event) {

  clickedDock_->
    updateColorSwatch(::colorFromScaledImageCoords(event->x(), event->y(),
                                                   imageLabel_->width(),
                                                   imageLabel_->height(),
                                                   originalImage()));
}

void colorChooser::setNewImage(const QImage& newImage) {

  // [Warning: we used to setPermStatus here, but the bold font causes the
  // size of imageScroll to decrease by 1, which causes zoomToImage to
  // not work unless you are 1) shown, by which I mean _on_ _screen_ of
  // course, 2) updateGeometry()ed, 3) pending event queue cleared
  // So we don't do setPermStatus here anymore]
  setPermanentStatusEnabled(true);
  setLabelImage(newImage);
  zoomToImage();
  processMode_.clearColorLists();
  clickedDock_->setColorList(processMode_.colorList());
  generatedDock_->clearList();
  generatedDockHolder_->setEnabled(false);
  setStatus(processMode_.statusHint());
}

void colorChooser::setLabelImage(const QImage& image) {

  if (imageLabel_->imageIsNull()) { // this is the first load
    setWidgetActive(true);
  }
  imageLabel_->setImageAndSize(QPixmap::fromImage(image));
}

void colorChooser::setWidgetActive(bool active) {

  imageMenu()->setEnabled(active);
  imageScroll_->setEnabled(active);
  setListDockEnabled(active);
  generatedDockHolder_->setEnabled(active);
  processModeBox_->setEnabled(active);
  setZoomActionsEnabled(active);
  processButton_->setEnabled(active);
  setSaveActionsEnabled(active);
  if (active) {
    numColorsBox_->setEnabled(processMode_.numColorsBoxActive());
  }
  else {
    numColorsBox_->setEnabled(false);
  }
}

void colorChooser::zoom(int zoomIncrement) {

  if (imageLabel_->width() + zoomIncrement <= 0) {
    return;
  }
  imageLabel_->setImageWidth(imageLabel_->width() + zoomIncrement);
}

void colorChooser::originalSize() {

  imageLabel_->setImageWidth(originalImage().width());
}

void colorChooser::zoomToWidth() {

  // scroll area size _without scrollbars_
  const QSize scrollSize = imageScroll_->maximumViewportSize();
  const QSize imageSize = originalImage().size();
  // if you call width directly on the actual scrollbar widget before
  // it's been made visible you get a goofy response
  const int scrollBarWidth =
    style()->pixelMetric(QStyle::PM_ScrollBarExtent);
  const int newWidth =
    ::computeMaxZoomWidth(scrollSize, imageSize, scrollBarWidth);

  if (newWidth != -1) {
    imageLabel_->setImageWidth(newWidth);
  }
  else {
    imageLabel_->setImageHeight(scrollSize.height());
  }
}

void colorChooser::zoomToHeight() {

  // scroll area size _without scrollbars_
  const QSize scrollSize = imageScroll_->maximumViewportSize();
  const QSize imageSize = originalImage().size();
  // if you call width directly on the actual scrollbar widget before
  // it's been made visible you get a goofy response
  const int scrollBarWidth =
    style()->pixelMetric(QStyle::PM_ScrollBarExtent);
  const int newHeight =
    ::computeMaxZoomHeight(scrollSize, imageSize, scrollBarWidth);

  if (newHeight != -1) {
    imageLabel_->setImageHeight(newHeight);
  }
  else {
    imageLabel_->setImageWidth(scrollSize.width());
  }
}

void colorChooser::zoomToImage() {

  // scroll area size _without scrollbars_
  const QSize scrollSize = imageScroll_->maximumViewportSize();
  const QSize imageSize = originalImage().size();
  const int newHeight =
    qCeil(imageSize.height()
          * static_cast<qreal>(scrollSize.width())/imageSize.width());
  if (newHeight <= scrollSize.height()) {
    imageLabel_->setImageWidth(scrollSize.width());
  }
  else {
    imageLabel_->setImageHeight(scrollSize.height());
  }
}

void colorChooser::clearList() {

  if (processMode_.resetColorList()) {
    clickedDock_->clearList();
  }
}

void colorChooser::processProcessChange(int boxIndex) {

  processMode_.setNewMode(processModeBox_->itemText(boxIndex));
  const processChange update = processMode_.makeProcessChange();
  imageLabel_->setMouseTracking(update.mouseTracking());
  numColorsBox_->setEnabled(update.numColorsBoxEnabled());
  clickedDock_->enableContextMenu(update.listRemoveEnabled());
  clearListAction_->setEnabled(update.listRemoveEnabled());
  clickedDock_->setColorList(update.clickedColors());
  setListDockTitle(update.dockTitle());
  const QVector<triC>& generatedColors = update.generatedColors();
  generatedDock_->setColorList(generatedColors);
  generatedDockHolder_->setEnabled(!generatedColors.isEmpty());
  setStatus(processMode_.statusHint());
}

void colorChooser::processColorAdd(QMouseEvent* event) {

  const QRgb color = ::colorFromScaledImageCoords(event->x(), event->y(),
                                                  imageLabel_->width(),
                                                  imageLabel_->height(),
                                                  originalImage());
  // addedColor may be a DMC version of color
  bool added = false;
  const triC addedColor = processMode_.addColor(color, &added);
  if (added) {
    clickedDock_->addToList(addedColor);
  }
  else {
    // the color already exists
    clickedDock_->moveTo(addedColor);
  }
}

void colorChooser::processProcessing() {

  if (numColorsBox_->value() == 0 && !processMode_.userColorsExist()) {
    QMessageBox::information(this, tr("No colors requested"),
                        tr("There are currently no colors being requested - ") +
                             tr("either set the number of colors box on the toolbar to something larger than 0 ") +
                             tr("or click on at least one color on the image to add it to the \"Clicked colors\" list on the right."));
    return;
  }
  QImage workingImage = winManager()->originalImage().copy();
  if (workingImage.isNull()) {
    qWarning() << "Empty image in processProcessing.";
    return;
  }
  workingImage = workingImage.convertToFormat(QImage::Format_RGB32);
  const triState returnCode =
    processMode_.performProcessing(&workingImage, numColorsBox_->value(),
                                   winManager()->getOriginalImageColorCount());
  //qDebug() << "processing time: " << double(t.elapsed())/1000.;
  if (returnCode != triNoop) {
    const colorCompareSaver saver(-1, 0, processMode_.modeText(),
                                  processMode_.colorList());
    winManager()->addColorCompareImage(workingImage,
                                       processMode_.colorList(),
                                       processMode_.modeIsDmcOnly(),
                                       saver);
  }
  else { // processing cancelled
    return;
  }
  if (returnCode == triTrue) { // update dock
    generatedDockHolder_->setEnabled(true);
    generatedDock_->setColorList(processMode_.generatedColorList());
  }
}

void colorChooser::removeColor(const triC& color) {

  processMode_.removeColor(color);
}

void colorChooser::displayImageInfo() {

  displayOriginalImageInfo(imageLabel_->width(), imageLabel_->height());
}

int colorChooser::recreateImage(const colorCompareSaver& saver) {

  // set the widget's current processing mode box
  setModeBox(saver.creationMode());

  QImage workingImage = winManager()->originalImage().copy();
  if (workingImage.isNull()) {
    qWarning() << "Empty image in recreateImage.";
    return -1;
  }
  workingImage = workingImage.convertToFormat(QImage::Format_RGB32);
  // we don't need to do processMode_.performProcessing since we already
  // have the color list it would produce
  processMode_.restoreSavedImage(&workingImage, saver.colors(),
                                 winManager()->getOriginalImageColorCount());
  winManager()->addColorCompareImage(workingImage,
                                     saver.colors(),
                                     processMode_.modeIsDmcOnly(),
                                     saver,
                                     saver.index());
  return saver.hidden() ? saver.index() : -1;
}

void colorChooser::setModeBox(const QString& mode) {

  for (int i = 0, size = processModeBox_->count(); i < size; ++i) {
    if (processModeBox_->itemText(i) == mode) {
      processModeBox_->setCurrentIndex(i);
    }
  }
}

void colorChooser::appendCurrentSettings(QDomDocument* doc,
                                         QDomElement* appendee) const {

  QDomElement settings(doc->createElement("color_chooser_settings"));
  appendee->appendChild(settings);

  ::appendTextElement(doc, "mode", processMode_.modeText(), &settings);

  QDomElement colorLists(doc->createElement("color_lists"));
  settings.appendChild(colorLists);
  processMode_.appendColorLists(doc, &colorLists);
}

void colorChooser::updateCurrentSettings(const QDomElement& xml) {

  QDomElement settings(xml.firstChildElement("color_chooser_settings"));
  if (settings.isNull()) {
    return;
  }
  QDomElement colorLists(settings.firstChildElement("color_lists"));
  processMode_.setColorLists(colorLists);

  // set mode after color lists in order to pick up the new lists
  const QString mode = ::getElementText(settings, "mode");
  setModeBox(mode);
  clickedDock_->setColorList(processMode_.clickedColorList());
}

helpMode colorChooser::getHelpMode() const {

  return helpMode(helpMode::H_COLOR_CHOOSER);
}
