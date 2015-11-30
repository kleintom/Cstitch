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

#include "patternWindow.h"

#include <QtCore/QTimer>
#include <QtCore/QSettings>

#include <QtWidgets/QScrollArea>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>

#include "patternPrinter.h"
#include "patternDockWidget.h"
#include "windowManager.h"
#include "utility.h"
#include "comboBox.h"
#include "patternImageContainer.h"
#include "helpBrowser.h"
#include "patternImageLabel.h"
#include "dockImage.h"
#include "xmlUtility.h"
#include "floss.h"

// bounds for allowed symbol sizes (too large and file sizes are ridiculous,
// too small and symbols can't be distinguished)
extern const int MAX_SYMBOL_SIZE = 60;
extern const int MIN_SYMBOL_SIZE = 15;

patternWindow::patternWindow(windowManager* winMgr)
  : imageSaverWindow(tr("Symbols"), winMgr), pdfSymbolDim_(0),
    curImage_(patternImagePtr(NULL)),
    fontMetrics_(QFontMetrics(font())) {

  installEventFilter(this);
  imageLabel_ = new patternImageLabel(this);
  imageLabel_->setGridColor(Qt::black);
  connect(imageLabel_, SIGNAL(announceImageClick(QMouseEvent* )),
          this, SLOT(imageClickSlot(QMouseEvent* )));
  scroll_ = new QScrollArea(this);
  scroll_->installEventFilter(this);
  scroll_->viewport()->installEventFilter(this);
  scroll_->setWidget(imageLabel_);
  setCentralWidget(scroll_);

  dockImageHolder_ = new QDockWidget(this);
  dockImageHolder_->setFeatures(QDockWidget::NoDockWidgetFeatures);
  dockImage_ = new dockImage(winManager()->originalImage(), this);
  dockImageHolder_->setWidget(dockImage_);
  addDockWidget(Qt::RightDockWidgetArea, dockImageHolder_);
  connect(scroll_->horizontalScrollBar(), SIGNAL(valueChanged(int )),
          this, SLOT(labelScrollChange( )));
  connect(scroll_->verticalScrollBar(), SIGNAL(valueChanged(int )),
          this, SLOT(labelScrollChange( )));
  connect(dockImage_, SIGNAL(viewportUpdated(qreal , qreal, bool , bool )),
          this, SLOT(processDockImageUpdate(qreal , qreal, bool , bool )));

  const QFontMetrics metrics(font());
  basePatternDim_ = (metrics.width("@") > metrics.height()) ?
    metrics.width("@") : metrics.height();
  basePatternDim_ = qMax(basePatternDim_, 30);
  listDock_ = new patternDockWidget(basePatternDim_, this);
  setListDockWidget(listDock_);
  connect(listDock_, SIGNAL(changeSymbol(const triC& )),
          this, SLOT(changeSymbolSlot(const triC& )));

  constructActions();
  constructMenus();
  constructToolbar();
  constructPdfViewerDialog();

  setPermanentStatus(tr("Click the 'To pdf' button to save the pattern as a pdf."));
  setStatus(tr("left click: change symbol; right click: switch between square and symbol images"));
}

void patternWindow::constructActions() {

  backHistoryAction_ =
    new QAction(QIcon(":leftArrow.png"),
                tr("Undo"), this);
  backHistoryAction_->setShortcut(QKeySequence("Ctrl+z"));
  backHistoryAction_->setEnabled(false);
  connect(backHistoryAction_, SIGNAL(triggered()),
          this, SLOT(backHistoryActionSlot()));

  forwardHistoryAction_ =
    new QAction(QIcon(":rightArrow.png"),
                tr("Redo"), this);
  forwardHistoryAction_->setShortcut(QKeySequence("Ctrl+y"));
  forwardHistoryAction_->setEnabled(false);
  connect(forwardHistoryAction_, SIGNAL(triggered()),
          this, SLOT(forwardHistoryActionSlot()));

  switchAction_ = 
    new QAction(QIcon(":switchImages.png"), 
                tr("Switch between pattern and square images"), this);
  connect(switchAction_, SIGNAL(triggered()),
          this, SLOT(switchActionSlot()));

  gridAction_ = 
    new QAction(QIcon(":grid.png"), tr("Turn grid on/off"), this);
  gridAction_->setCheckable(true);
  gridAction_->setChecked(true);
  imageLabel_->setGridOn(true);
  connect(gridAction_, SIGNAL(triggered(bool )),
          this, SLOT(processGridChange(bool )));

  deleteImageAction_ = new QAction(tr("Delete image"), this);
  connect(deleteImageAction_, SIGNAL(triggered()),
          this, SLOT(processDelete()));

  gridColorAction_ = new QAction(tr("Change grid color"), this);
  connect(gridColorAction_, SIGNAL(triggered()),
          this, SLOT(processGridColorChange()));

  connect(originalSizeAction(), SIGNAL(triggered()),
          this, SLOT(originalSize()));

  setPdfViewerAction_ = new QAction(tr("Set pdf viewer"), this);
  connect(setPdfViewerAction_, SIGNAL(triggered()),
          this, SLOT(updatePdfViewerOptions()));
}

void patternWindow::constructMenus() {

  imageMenu()->addAction(switchAction_);
  imageMenu()->addAction(zoomInAction());
  imageMenu()->addAction(zoomOutAction());
  imageMenu()->addAction(originalSizeAction());
  imageMenu()->addAction(gridAction_);
  imageMenu()->addAction(gridColorAction_);
  imageMenu()->addAction(imageInfoAction());
  helpMenu()->addAction(setPdfViewerAction_);

  imageListMenu_ = new QMenu(tr("I&mages"), this);
  menuBar()->insertMenu(helpMenu()->menuAction(), imageListMenu_);
  connect(imageListMenu_, SIGNAL(triggered(QAction* )),
          this, SLOT(processImageListMenu(QAction* )));
  imageListMenu_->addAction(deleteImageAction_);
  imageListMenu_->addSeparator();
}

void patternWindow::constructToolbar() {

  saveToPdfButton_ = new QPushButton(tr("To pdf (4/4)"), this);
  saveToPdfButton_->setShortcut(QKeySequence("Ctrl+return"));
  saveToPdfButton_->setToolTip(tr("Save the pattern as a pdf file"));
  connect(saveToPdfButton_, SIGNAL(clicked()),
          this, SLOT(saveSlot()));
  imageListBox_ = new comboBox(this);
  connect(imageListBox_, SIGNAL(activated(const QString& )),
          this, SLOT(processImageBox(const QString& )));
  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarWidget(saveToPdfButton_);
  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarAction(switchAction_);
  addToolbarAction(gridAction_);
  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarAction(backHistoryAction_);
  addToolbarAction(forwardHistoryAction_);
  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarAction(zoomInAction());
  addToolbarAction(zoomOutAction());
  addToolbarAction(originalSizeAction());
  addToolbarSeparator();
  addToolbarSeparator();
  addToolbarWidget(imageListBox_);
  addToolbarSeparator();
  addToolbarSeparator();
}

void patternWindow::addImage(const QImage& squareImage, int squareDimension,
                             const QVector<flossColor>& colors, QRgb gridColor,
                             int imageIndex) {

  patternImageContainer* container =
    new patternImageContainer(squareImage, imageNameFromIndex(imageIndex),
                              squareDimension, basePatternDim_, colors);
  const patternImagePtr imagePtr(container);
  connect(container, SIGNAL(symbolChanged(QRgb , const QPixmap& )),
          listDock_, SLOT(changeSymbol(QRgb , const QPixmap& )));
  QAction* menuAction = new QAction(container->name(), this);
  menuAction->setData(QVariant::fromValue(imagePtr));
  imageListMenu_->addAction(menuAction);
  imageListBox_->addItem(container->name());
  imageLabel_->setGridColor(gridColor);
  setCur(imagePtr);
}

void patternWindow::setCur(patternImagePtr container) {

  if (curImage_ != container) {
    curImage_ = container;
    imageLabel_->setSymbols(curImage_->symbols());
    imageLabel_->setSquares(curImage_->colorSquares());
    imageLabel_->setImage(curImage_->squareImage(),
                          curImage_->squareDimension(),
                          curImage_->symbolDimension());
    dockImage_->setImage(curImage_->squareImage());
    // wait for the geometry to settle itself before
    // processing the scroll change
    QTimer::singleShot(50, this, SLOT(labelScrollChange()));
    listDock_->setSymbolList(curImage_->symbols());
    imageListBox_->setCurrentIndex(imageListBox_->findText(curImage_->name()));
    updateHistoryButtonStates();
  }
}

void patternWindow::processImageListMenu(QAction* action) {

  if (! action->data().isNull() &&
      action->data().canConvert<patternImagePtr>()) {
    patternImagePtr container =
      action->data().value<patternImagePtr>();
    setCur(container);
  }
}

void patternWindow::processImageBox(const QString& imageName) {

  QAction* action =
    ::actionFromImageName(imageListMenu_->actions(),
                          findPatternActionName(imageName));
  if (action) {
    processImageListMenu(action);
  }
}

void patternWindow::processDelete() {

  QAction* action =
    ::actionFromImageName(imageListMenu_->actions(),
                          findPatternActionName(curImage_->name()));
  delete action;
  imageListBox_->removeItem(imageListBox_->findText(curImage_->name()));
  int removedImageIndex = imageNameToIndex(curImage_->name());
  curImage_ = patternImagePtr(NULL);
  winManager()->patternWindowImageDeleted(removedImageIndex);
  // find a new image - just show the last image since we have no
  // reason to favor any particular image
  const QAction* lastAction = imageListMenu_->actions().last();
  if (! lastAction->data().isNull() &&
      lastAction->data().canConvert<patternImagePtr>()) {
    setCur(lastAction->data().value<patternImagePtr>());
  }
  else {
    // close up shop if there are no images left
    hide();
    winManager()->patternWindowEmpty();
  }
}

void patternWindow::processGridChange(bool checked) {

  gridAction_->setChecked(checked);
  imageLabel_->setGridOn(checked);
}

void patternWindow::processGridColorChange() {

  const QColor chosenColor =
    QColorDialog::getColor(QColor(imageLabel_->gridColor()), this);
  if (chosenColor.isValid()) {
    imageLabel_->setGridColor(chosenColor.rgb());
  }
}

void patternWindow::changeSymbolSlot(const triC& color) {

  const bool updateImage = curImage_->changeSymbol(color);
  if (updateImage) {
    updateImageLabelSymbols();
  }
  updateHistoryButtonStates();
}

void patternWindow::updateHistoryButtonStates() {

  backHistoryAction_->setEnabled(curImage_->backHistory());
  forwardHistoryAction_->setEnabled(curImage_->forwardHistory());
}

void patternWindow::imageClickSlot(QMouseEvent* event) {

  if (event->button() == Qt::RightButton) {
    switchActionSlot();
  }
  else if (event->button() == Qt::LeftButton) {
    const bool updateImage =
      curImage_->mouseActivatedChangeSymbol(imageLabel_->width(),
                                            imageLabel_->height(), event);
    if (updateImage) {
      updateImageLabelSymbols();
    }
    updateHistoryButtonStates();
  }
}

void patternWindow::updateImageLabelSymbols() {

  imageLabel_->setSymbols(curImage_->symbols());
  imageLabel_->update();
}

void patternWindow::updateImageLabelSize() {

  imageLabel_->setSymbols(curImage_->symbols());
  imageLabel_->setSquares(curImage_->colorSquares());
  const int newDimension = curImage_->symbolDimension();
  imageLabel_->setPatternDim(newDimension);
  labelScrollChange();
  zoomInAction()->setEnabled(newDimension < MAX_SYMBOL_SIZE);
  zoomOutAction()->setEnabled(newDimension > MIN_SYMBOL_SIZE);
}

void patternWindow::originalSize() {

  curImage_->setSymbolDimension(basePatternDim_);
  zoomInAction()->setEnabled(true);
  zoomOutAction()->setEnabled(true);
  updateImageLabelSize();
}

void patternWindow::zoom(int zoomIncrement) {

  const int zoomDelta = (zoomIncrement > 0) ? 2 : -2;
  curImage_->setSymbolDimension(curImage_->symbolDimension() + zoomDelta);
  updateImageLabelSize();
}

void patternWindow::switchActionSlot() {

  if (curImage_->viewingSquareImage()) {
    curImage_->setViewingSquareImage(false);
    imageLabel_->viewSquareImage(false);
  }
  else {
    curImage_->setViewingSquareImage(true);
    imageLabel_->viewSquareImage(true);
  }
}

void patternWindow::forwardHistoryActionSlot() {

  curImage_->moveHistoryForward();
  updateImageLabelSymbols();
  updateHistoryButtonStates();
}

void patternWindow::backHistoryActionSlot() {

  curImage_->moveHistoryBack();
  updateImageLabelSymbols();
  updateHistoryButtonStates();
}

QImage patternWindow::curImageForSaving() const {

  QImage returnImage;
  if (curImage_->viewingSquareImage()) {
    returnImage = curImage_->squareImage();
    returnImage = returnImage.scaledToWidth(imageLabel_->width());
  }
  else {
    returnImage = curImage_->patternImageCurSymbolSize();
  }

  if (gridAction_->isChecked()) {
    if (curImage_->viewingSquareImage()) {
      ::gridImage(&returnImage, curImage_->squareDimension(),
                  curImage_->squareImage().width(),
                  curImage_->squareImage().height(),
                  imageLabel_->gridColor());
    }
    else {
      ::gridImage(&returnImage, curImage_->symbolDimension(),
                  returnImage.width(), returnImage.height(),
                  imageLabel_->gridColor());
    }
  }
  return returnImage;
}

QSize patternWindow::curImageViewSize() const {

  return QSize(imageLabel_->width(), imageLabel_->height());
}

void patternWindow::saveSlot() {

  patternPrinter printer(curImage_, originalImage());
  printer.save(usePdfViewer_, pdfViewerPath_);
}

bool patternWindow::eventFilter(QObject* watched, QEvent* event) {

  if (event->type() == QEvent::KeyPress) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if ((keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down)
        && (keyEvent->modifiers() & Qt::ShiftModifier)) {
      if (keyEvent->key() == Qt::Key_Up) {
        imageListBox_->moveToPreviousItem();
      }
      else {
        imageListBox_->moveToNextItem();
      }
      return true;
    }
  }
  return imageSaverWindow::eventFilter(watched, event);
}

// this probably isn't what you want - some active subwidget will probably
// capture key strokes before they ever get here.  Use eventFilters instead
void patternWindow::keyPressEvent(QKeyEvent* ) {

}

void patternWindow::labelScrollChange() {

  const int labelImageWidth = imageLabel_->width() - 1;
  const int labelImageHeight = imageLabel_->height() - 1;
  const qreal x1 =
    scroll_->horizontalScrollBar()->value()/static_cast<qreal>(labelImageWidth);
  const qreal x2 =
    (scroll_->horizontalScrollBar()->value() +
     scroll_->viewport()->width() - 1)/static_cast<qreal>(labelImageWidth);
  const qreal y1 =
    scroll_->verticalScrollBar()->value()/static_cast<qreal>(labelImageHeight);
  const qreal y2 =
    (scroll_->verticalScrollBar()->value() +
     scroll_->viewport()->height() - 1)/static_cast<qreal>(labelImageHeight);
  dockImage_->updateViewport(x1, x2, y1, y2);
}

void patternWindow::processDockImageUpdate(qreal xPercentage,
                                           qreal yPercentage,
                                           bool rightEdge,
                                           bool bottomEdge) {

  scroll_->horizontalScrollBar()->setValue(qRound(xPercentage*imageLabel_->width()));
  scroll_->verticalScrollBar()->setValue(qRound(yPercentage*imageLabel_->height()));
  if (rightEdge) {
    scroll_->horizontalScrollBar()->setValue(scroll_->horizontalScrollBar()->maximum());
  }
  if (bottomEdge) {
    scroll_->verticalScrollBar()->setValue(scroll_->verticalScrollBar()->maximum());
  }
}

void patternWindow::resizeEvent(QResizeEvent* event) {

  labelScrollChange();
  imageSaverWindow::resizeEvent(event);
}

void patternWindow::displayImageInfo() {

  if (curImage_) {
    const int width = imageLabel_->width();
    const int height = imageLabel_->height();
    const int symbolDim = curImage_->symbolDimension();
    const int xBoxes = width/symbolDim;
    const int yBoxes = height/symbolDim;
    QMessageBox::information(this, curImage_->name(),
                             tr("%1 currently has dimensions %2x%3, box "
                                "dimension %4, and is %5 by %6 boxes.")
                             .arg(curImage_->name())
                             .arg(::itoqs(width))
                             .arg(::itoqs(height))
                             .arg(::itoqs(symbolDim))
                             .arg(::itoqs(xBoxes))
                             .arg(::itoqs(yBoxes)));
  }
}

void patternWindow::writeCurrentHistory(QDomDocument* doc,
                                        QDomElement* appendee,
                                        int imageIndex) {

  patternImagePtr container = getImageFromIndex(imageIndex);
  if (container) {
    container->writeSymbolHistory(doc, appendee);
  }
  else {
    qWarning() << "Misplaced container in patternWriteHistory" << 
      imageIndex;
  }
}


patternImagePtr patternWindow::getImageFromIndex(int index) const {

  const QAction* actionMatch =
    ::actionFromImageName(imageListMenu_->actions(),
                          findPatternActionName(imageNameFromIndex(index)));
  if (actionMatch) {
    return actionMatch->data().value<patternImagePtr>();
  }
  else {
    return patternImagePtr(NULL);
  }
}

void patternWindow::updateHistory(const QDomElement& xml) {

  const int imageIndex = ::getElementText(xml, "index").toInt();
  const QDomElement& xmlHistory = xml.firstChildElement("symbol_history");
  patternImagePtr container = getImageFromIndex(imageIndex);
  if (container) {
    container->updateHistory(xmlHistory);
    updateImageLabelSymbols();
    updateHistoryButtonStates();
  }
  else {
    qWarning() << "Misplaced container on patternHistoryRestore:" <<
      imageIndex;
  }
}

QString patternWindow::updateCurrentSettings(const QDomElement& xml) {

  QDomElement settings(xml.firstChildElement("pattern_window_settings"));
  if (settings.isNull()) {
    return QString();
  }
  imageLabel_->setGridColor(::xmlStringToRgb(::getElementText(settings,
                                                           "grid_color")));
  processGridChange(::stringToBool(::getElementText(settings, "grid_on")));


  // TODO: De-duplicate this from the imageCompareBase version.  :-(
  // Seems like imagePtr and patternImagePtr should be related, but they're not
  // (because imageContainer and patternImageContainer aren't either, and really
  // don't share a common interface, which is what makes this a mess), and so
  // functions here "common" to imageCompareBase and patternWindow like
  // getImageFromIndex and setCur can't be virtualized into a common base.
  // getImageFromIndex could/maybe should be templated and factored down, but
  // setCur can't be - we could template this code as well and pass a
  // doubly-templated functor to call the right setCur on the right object, but
  // yuk.  Live with the duplication for now.
  QString errorMessage;
  if (!settings.firstChildElement("current_index").isNull()) {
    // KEEP THIS IN SYNC WITH imageCompareBase
    bool parseOk = false;
    const int curIndex =
      ::getElementText(settings, "current_index").toInt(&parseOk);
    if (parseOk) {
      patternImagePtr curImage = getImageFromIndex(curIndex);
      if (curImage) {
        setCur(curImage);
      }
      else {
        errorMessage +=
          tr("Pattern Window setting error(s):<br />"
             "Image Missing error: image %1").arg(curIndex);
      }
    }
    else {
      errorMessage += tr("Pattern Window setting error(s):<br />"
                         "Parse error parsing \"%1\" for index")
        .arg(::getElementText(settings, "current_index"));
    }
  }

  return errorMessage;
}

void patternWindow::appendCurrentSettings(QDomDocument* doc,
                                          QDomElement* appendee) const {

  if (curImage_) {
    QDomElement settings(doc->createElement("pattern_window_settings"));
    appendee->appendChild(settings);
    ::appendTextElement(doc, "current_index",
                        QString::number(imageNameToIndex(curImage_->name())),
                        &settings);
    ::appendTextElement(doc, "grid_on",
                        ::boolToString(imageLabel_->gridOn()), &settings);
    ::appendTextElement(doc, "grid_color",
                        ::rgbToString(imageLabel_->gridColor()), &settings);
  }
}

void patternWindow::processFirstShow() {

  imageSaverWindow::processFirstShow();
  // update the dock image now that we have correct geometry measurements
  labelScrollChange();
}

helpMode patternWindow::getHelpMode() const {

  return helpMode::H_PATTERN;
}

void patternWindow::constructPdfViewerDialog() {

  usePdfViewer_ = false;
  pdfViewerPath_ = "";
  const QSettings settings("cstitch", "cstitch");
  if (settings.contains("use_pdf_viewer") &&
      settings.contains("pdf_viewer_path")) {
    usePdfViewer_ = settings.value("use_pdf_viewer").toBool();
    pdfViewerPath_ = settings.value("pdf_viewer_path").toString();
  }
  else { // very first run on this machine
    // find a default viewer if we can
#ifdef Q_OS_LINUX
    QStringList viewers;
    viewers << "/usr/bin/evince" << "/usr/bin/kpdf" << "/usr/bin/okular" <<
      "/usr/bin/xpdf" << "/usr/bin/acroread";
    for (int i = 0, size = viewers.size(); i < size; ++i) {
      const QString thisViewer = viewers[i];
      if (QFile::exists(thisViewer)) {
        pdfViewerPath_ = thisViewer;
        break;
      }
    }
#endif
#ifdef Q_OS_WIN
    // try to find acroread
    QStringList adobeDirectories;
    adobeDirectories << "C:/Program Files/Adobe/" <<
      "C:/Program Files (x86)/Adobe/";
    for (int i = 0, size = adobeDirectories.size(); i < size; ++i) {
      QDir thisDir = adobeDirectories[i];
      if (thisDir.exists()) {
        // sort by time - if somebody has multiple versions of Reader
        // installed (not officially supported by Adobe) and they
        // install an older version after a newer version then this will
        // choose the wrong version
        const QStringList readers =
          thisDir.entryList(QStringList("Reader *"), QDir::Dirs,
                            QDir::Time|QDir::Reversed);
        if (!readers.empty() && thisDir.exists(readers[0] + "/Reader")) {
          thisDir.cd(readers[0] + "/Reader");
          if (thisDir.exists("AcroRd32.exe")) {
            pdfViewerPath_ = thisDir.absoluteFilePath("AcroRd32.exe");
            break;
          }
          else if (thisDir.exists("AcroRd64.exe")) {
            pdfViewerPath_ = thisDir.absoluteFilePath("AcroRd64.exe");
            break;
          }
        }
      }
    }
#endif // Q_OS_WIN
    if (!pdfViewerPath_.isEmpty()) {
      usePdfViewer_ = true;
    }
  }
}

void patternWindow::updatePdfViewerOptions() {

  pdfViewerDialog viewerDialog(usePdfViewer_, pdfViewerPath_, this);
  const int rc = viewerDialog.exec();
  if (rc == QDialog::Accepted) {
    QSettings settings("cstitch", "cstitch");
    usePdfViewer_ = viewerDialog.useViewer();
    settings.setValue("use_pdf_viewer", usePdfViewer_);
    pdfViewerPath_ = viewerDialog.viewerPath();
    settings.setValue("pdf_viewer_path", pdfViewerPath_);
  }
}

bool patternWindow::horizontalWheelScrollEvent(QObject* ,
                                               QWheelEvent* event) const {

  scroll_->horizontalScrollBar()->event(event);
  return true;
}

pdfViewerDialog::pdfViewerDialog(bool useViewer, const QString& curViewerPath,
                                 QWidget* parent) 
  : cancelAcceptDialogBase(parent),
    useViewer_(new QGroupBox(tr("Use viewer to display saved pdf patterns"))),
    dialogLayout_(new QVBoxLayout),
    groupLayout_(new QVBoxLayout),
    currentPath_(new QLineEdit),
    choosePath_(new QPushButton(tr("Choose new pdf viewer"))) {

  useViewer_->setCheckable(true);
  useViewer_->setChecked(useViewer);
  currentPath_->setText(curViewerPath);

  connect(choosePath_, SIGNAL(clicked()),
          this, SLOT(updateViewerPath()));

  groupLayout_->addWidget(currentPath_);
  groupLayout_->addWidget(choosePath_);
  useViewer_->setLayout(groupLayout_);

  dialogLayout_->addWidget(useViewer_);
  dialogLayout_->addWidget(cancelAcceptWidget());
  setLayout(dialogLayout_);
  
  setWindowTitle(tr("Set pdf viewer"));
}

void pdfViewerDialog::updateViewerPath() {

  const QString newPath =
    QFileDialog::getOpenFileName(this, tr("Choose new pdf viewer"),
                                 currentPath_->text());
  if (!newPath.isEmpty()) {
    currentPath_->setText(newPath);
  }
} 

QString pdfViewerDialog::viewerPath() const {

  return currentPath_->text();
}

bool pdfViewerDialog::useViewer() const {
 
  return useViewer_->isChecked();
}
