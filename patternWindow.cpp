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
#include <QtCore/qmath.h>
#include <QtCore/QProcess>
#include <QtCore/QSettings>

#include <QtGui/QPainter>
#include <QtGui/QMessageBox>
#include <QtGui/QScrollArea>
#include <QtGui/QPrinter>
#include <QtGui/QDockWidget>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QScrollBar>
#include <QtGui/QColorDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QPushButton>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFileDialog>

#include "dmcList.h"
#include "patternDockWidget.h"
#include "windowManager.h"
#include "utility.h"
#include "imageUtility.h"
#include "imageProcessing.h"
#include "comboBox.h"
#include "patternImageContainer.h"
#include "helpBrowser.h"
#include "patternImageLabel.h"
#include "dockImage.h"
#include "xmlUtility.h"
#include "patternMetadata.h"

// bounds for allowed symbol sizes (too large and file sizes are ridiculous,
// too small and symbols can't be distinguished)
extern const int MAX_SYMBOL_SIZE = 68;
extern const int MIN_SYMBOL_SIZE = 10;
// coordinates for progress meters (meters aren't parented, so we fix constant
// coords instead of letting the system choose them randomly)
extern const int PROGRESS_X_COORDINATE;
extern const int PROGRESS_Y_COORDINATE;

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
  backHistoryAction_->setEnabled(false);
  connect(backHistoryAction_, SIGNAL(triggered()),
          this, SLOT(backHistoryActionSlot()));

  forwardHistoryAction_ =
    new QAction(QIcon(":rightArrow.png"),
                tr("Redo"), this);
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
  saveToPdfButton_->setToolTip(tr("Save the pattern as a pdf file; the pdf symbol size will reflect the current zoom level on screen"));
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
                             const QVector<triC>& colors, QRgb gridColor,
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

QImage patternWindow::gridedImage(const QImage& image,
                                  int originalSquareDim,
                                  int originalWidth,
                                  int originalHeight,
                                  QRgb gridColor) const {

  QImage returnImage = image;
  ::gridImage(&returnImage, originalSquareDim,
              originalWidth, originalHeight, gridColor);
  return returnImage;
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

  QPrinter printer;
  printer.setOutputFormat(QPrinter::PdfFormat);  // actually default
  const QRect printerRect = printer.pageRect();
  const int printerWidth = printerRect.width();
  const int printerHeight = printerRect.height();
  patternMetadata metadata(printerWidth, 20, 8, 8, this);
  const int metadataReturnCode = metadata.exec();
  if (metadataReturnCode == QDialog::Rejected) {
    return;
  }
  metadata.saveSettings();

  const QString outputFile =
    QFileDialog::getSaveFileName(this, tr("Save pattern"), ".",
                                 tr("Pdf files (*.pdf)\n"
                                    "All (*.*)"));
  if (outputFile.isEmpty()) {
    return;
  }

  printer.setOutputFileName(outputFile);

  // to "print", you draw on the printer object
  // do printer.newPage() for each new page
  QPainter painter;
  painter.begin(&printer);
  setFontMetric(painter.fontMetrics());

  const QImage& squareImage = curImage_->squareImage();
  const int squareDim = curImage_->squareDimension();
  // the symbol dimension for the output pdf
  pdfSymbolDim_ = metadata.pdfSymbolSize();
  const int patternImageWidth =
    (squareImage.width()/squareDim)*pdfSymbolDim_;
  const int patternImageHeight =
    (squareImage.height()/squareDim)*pdfSymbolDim_;

  //// draw title pages with the original and squared images
  drawTitlePage(&painter, &printer, metadata);

  //// do the actual pdf pattern drawing
  // these are set by drawPdfImage
  int widthPerPage; // actual image width per pdf page
  int heightPerPage; // actual image height per pdf page
  int xpages, ypages; // pdf pattern pages used

  const bool cancel =
    drawPdfImage(&painter, &printer, patternImageWidth, patternImageHeight,
                 &xpages, &ypages, &widthPerPage, &heightPerPage);
  if (cancel) {
    // abort probably does nothing; the printer writes to disk as it goes,
    // so we've already written a partial pdf
    painter.end();
    printer.abort();
    QFile fileToRemove(outputFile);
    fileToRemove.remove();
    return;
  }

  //// present the page-number to image-portion correspondence
  int yused = 0; // how much vertical space does the correspondence take?
  if (xpages * ypages > 1) {
    yused = drawPdfLegend(&painter, xpages, ypages,
                          patternImageWidth, patternImageHeight,
                          widthPerPage, heightPerPage,
                          printerWidth, printerHeight);
  }
  else {
    // draw the page number
    const int pageNum = xpages * ypages + 1;
    const int pageNumWidth = sWidth(pageNum);
    const int pageNumHeight = sHeight(pageNum);
    painter.drawText(printerWidth - pageNumWidth, pageNumHeight,
                     ::itoqs(pageNum));
  }

  //// present the color list
  // this will be the next page number
  const int pageNum = xpages * ypages + 2;
  drawPdfColorList(&painter, &printer, pageNum, yused);

  painter.end();

  if (usePdfViewer_) {    
    QProcess::startDetached(pdfViewerPath_,
                            QStringList(QDir::toNativeSeparators(outputFile)));
  }
}

void patternWindow::drawTitlePage(QPainter* painter,
                                  QPrinter* printer,
                                  const patternMetadata& metadata) const {

  QRect availableTextRect = printer->pageRect();
  // the availableTextRect is absolute with respect to the page, but
  // when we draw it's relative to availableTextRect - use draw coords
  availableTextRect.moveTopLeft(QPoint(0, 0));
  if (metadata.title() != "") {
    drawTitleMetadata(painter, metadata.titleFontSize(), true,
                      metadata.title(), &availableTextRect);
  }
  if (metadata.patternBy() != "") {
    drawTitleMetadata(painter, metadata.patternByFontSize(), false,
                      metadata.patternBy(), &availableTextRect);
  }
  if (metadata.photoBy() != "") {
    drawTitleMetadata(painter, metadata.photoByFontSize(), false,
                      metadata.photoBy(), &availableTextRect);
  }
  // title margin
  availableTextRect.setTop(availableTextRect.top() + 5);

  painter->save();
  QPen pen(painter->pen());
  pen.setWidth(2);
  painter->setPen(pen);
  // draw the original image on this page
  const QImage original =
    originalImage().scaled(availableTextRect.size(), Qt::KeepAspectRatio,
                           Qt::SmoothTransformation);
  const int xStart = (availableTextRect.width() - original.width())/2;
  const int yStart = availableTextRect.top();
  const QPoint originalStart(xStart, yStart);
  painter->drawImage(originalStart, original);
  painter->drawRect(QRect(originalStart, original.size()));
  printer->newPage();

  // draw the squared image
  QRect usableRect = printer->pageRect();
  usableRect.moveTopLeft(QPoint(0, 0));
  // figure out if gridding will be reasonable at the zoom level needed
  // to fit the page
  const int newSquareDim =
    ::computeGridForImageFit(curImage_->squareImage().size(),
                             usableRect.size(),
                             curImage_->squareDimension());
  QImage squareImage;
  if (newSquareDim > 4) { // okay to grid
    const int newWidth = newSquareDim *
      curImage_->squareImage().size().width()/curImage_->squareDimension();
    const int newHeight = newSquareDim *
      curImage_->squareImage().size().height()/curImage_->squareDimension();
    squareImage =
      curImage_->squareImage().scaled(newWidth, newHeight,
                                      Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation);
    squareImage = gridedImage(squareImage,
                              curImage_->squareDimension(),
                              curImage_->squareImage().width(),
                              curImage_->squareImage().height());
  }
  else { // don't grid
    squareImage =
      curImage_->squareImage().scaled(usableRect.size(),
                                      Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation);
  }
  const QPoint squareStart((usableRect.width() - squareImage.width())/2, 0);
  painter->drawImage(squareStart, squareImage);
  painter->drawRect(QRect(squareStart, squareImage.size()));
  painter->restore();
  printer->newPage();
}

void patternWindow::drawTitleMetadata(QPainter* painter, int fontSize,
                                      bool bold, const QString& text,
                                      QRect* availableTextRect) const {

  painter->save();
  QFont font = painter->font();
  font.setPointSize(fontSize);
  font.setBold(bold);
  painter->setFont(font);
  QRect usedTextRect;
  painter->drawText(*availableTextRect, Qt::TextWordWrap | Qt::AlignHCenter,
                    text, &usedTextRect);
  (*availableTextRect).setTop(usedTextRect.bottom());
  painter->restore();
}

void patternWindow::drawTitlePageImage(QPainter* painter,
                                       const QImage& image, int startHeight,
                                       int usableWidth,
                                       int usableHeight) const {

  QImage scaledImage = image.scaled(usableWidth, usableHeight,
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);
  painter->drawImage(0, startHeight, scaledImage);
}

bool patternWindow::computeNumPages(int w, int h, int widthPerPage,
                                    int heightPerPage,
                                    int* xpages, int* ypages) const {

  const int landscapePages =
    computeNumPagesHelper(w, h, heightPerPage, widthPerPage,
                          xpages, ypages);
  const int portraitPages =
    computeNumPagesHelper(w, h, widthPerPage, heightPerPage,
                          xpages, ypages);
  bool portrait = true;
  if (portraitPages > landscapePages) {
    portrait = false;
    // recompute xpages, ypages for landscape
    computeNumPagesHelper(w, h, heightPerPage, widthPerPage,
                          xpages, ypages);
  }
  return portrait;
}

int patternWindow::computeNumPagesHelper(int w, int h, int widthPerPage,
                                         int heightPerPage,
                                         int* xpages, int* ypages) const {
  int tXpages = 0; // will become *xpages
  int tYpages = 0; // will become *ypages
  const int xBoxesPerPage = widthPerPage/pdfSymbolDim_; // horizontal boxes per page
  const int yBoxesPerPage = heightPerPage/pdfSymbolDim_; // vertical boxes per page

  tXpages = (w/pdfSymbolDim_)/xBoxesPerPage; // horizontal pages needed
  if ((w/pdfSymbolDim_) % xBoxesPerPage != 0) {
    ++tXpages;
  }
  tYpages = (h/pdfSymbolDim_)/yBoxesPerPage; // vertical pages needed
  if ((h/pdfSymbolDim_) % yBoxesPerPage != 0) {
    ++tYpages;
  }
  *xpages = tXpages;
  *ypages = tYpages;

  return tXpages*tYpages;
}

bool patternWindow::drawPdfImage(QPainter* painter, QPrinter* printer,
                                 int patternImageWidth,
                                 int patternImageHeight,
                                 int* xpages, int* ypages,
                                 int* widthPerPage,
                                 int* heightPerPage) {

  // will become *widthPerPage, *heightPerPage
  int tWidthPerPage, tHeightPerPage;

  const QRect printerRectangle = printer->pageRect();
  int printerWidth = printerRectangle.width();
  int printerHeight = printerRectangle.height();

  const int xBoxes = patternImageWidth/pdfSymbolDim_;
  const int yBoxes = patternImageHeight/pdfSymbolDim_;

  // horizontal boxes per pdf page
  int xBoxesPerPage = printerWidth/pdfSymbolDim_;
  // vertical boxes per pdf page
  int yBoxesPerPage = printerHeight/pdfSymbolDim_;

  // how much room do we want for grid-line-count numbers (in multiples
  // of pdfSymbolDim_ for future use)
  int xstart = 0;
  while (xstart < sWidth("555")) {
    xstart += pdfSymbolDim_;
  }
  xBoxesPerPage -= xstart/pdfSymbolDim_;
  tWidthPerPage = xBoxesPerPage*pdfSymbolDim_; // image width per page

  // make ystart = xstart to simplify the landscape/portrait issue
  // to fix this pass xstart and ystart to computeNumPages
  const int ystart = xstart;
  yBoxesPerPage -= ystart/pdfSymbolDim_;
  tHeightPerPage = yBoxesPerPage*pdfSymbolDim_; // image height per page

  bool portrait = computeNumPages(patternImageWidth, patternImageHeight,
                                  tWidthPerPage, tHeightPerPage,
                                  xpages, ypages);
  if (!portrait) {
    // you can't change the orientation under Windows, so we'll just
    // draw sideways instead
    painter->save();
    painter->rotate(-90);
    painter->translate(-printerHeight, 0);
    // recompute :p
    int temp = printerWidth;
    printerWidth = printerHeight;
    printerHeight = temp;
    temp = tWidthPerPage;
    tWidthPerPage = tHeightPerPage;
    tHeightPerPage = temp;
    temp = xBoxesPerPage;
    xBoxesPerPage = yBoxesPerPage;
    yBoxesPerPage = temp;
  }

  // start printing
  const bool cancel =
    actuallyDrawPdfImage(painter, printer, patternImageWidth,
                         patternImageHeight, *xpages, *ypages,
                         xstart, ystart, xBoxes, yBoxes,
                         xBoxesPerPage, yBoxesPerPage,
                         tWidthPerPage, tHeightPerPage,
                         portrait);
  if (cancel) {
    return true;
  }

  if (!portrait) {
    painter->restore();
  }
  printer->newPage();
  *widthPerPage = tWidthPerPage;
  *heightPerPage = tHeightPerPage;
  return false;
}

bool patternWindow::actuallyDrawPdfImage(QPainter* painter,
                                         QPrinter* printer,
                                         int patternImageWidth,
                                         int patternImageHeight,
                                         int xpages, int ypages,
                                         int xstart, int ystart,
                                         int xBoxes, int yBoxes,
                                         int xBoxesPerPage,
                                         int yBoxesPerPage,
                                         int widthPerPage,
                                         int heightPerPage,
                                         bool portrait) {
  painter->setPen(QPen(Qt::black, 1));
  // image width to use on a particular page (last page may be smaller)
  int widthToUse = widthPerPage;
  int heightToUse = heightPerPage;
  const int f = 5; // fudge room for grid number separation from the grid
  const QHash<QRgb, QPixmap> symbolMap = curImage_->symbolsNoBorder(pdfSymbolDim_);
  const QImage& squareImage = curImage_->squareImage();
  const int squareDim = curImage_->squareDimension();
  const int printerWidth = printer->pageRect().width();
  const int printerHeight = printer->pageRect().height();
  // if QWidget is set to this in the constructor then the dialog cancel
  // button isn't clickable...
  QProgressDialog progressMeter(QObject::tr("Creating pdf..."),
                                QObject::tr("Cancel"), 0, (xpages*ypages)/5);
  progressMeter.setMinimumDuration(4000);
  progressMeter.setWindowModality(Qt::WindowModal);
  progressMeter.move(PROGRESS_X_COORDINATE, PROGRESS_Y_COORDINATE);
  progressMeter.show();
  for (int x = 1; x <= xpages; ++x) {
    for (int y = 1; y <= ypages; ++y) {
      if (progressMeter.wasCanceled()) {
        return true;
      }
      const int pageNum = ypages*(x-1)+y;
      // draw the page number
      const int pageNumXStart = portrait ?
        printerWidth - sWidth(pageNum) : printerHeight - sWidth(pageNum);
      painter->drawText(pageNumXStart, sHeight(pageNum)-5, ::itoqs(pageNum));
      if (pageNum % 5 == 0) {
        progressMeter.setValue(pageNum / 5);
      }
      widthToUse = widthPerPage;
      if (x*widthPerPage > patternImageWidth) {
        widthToUse = patternImageWidth - (x-1)*widthPerPage;
      }
      heightToUse = heightPerPage;
      if (y*(heightPerPage) > patternImageHeight) {
        heightToUse = patternImageHeight - (y-1)*heightPerPage;
      }

      // draw this page's image
      const int patternXBoxStart = ((x-1)*widthPerPage)/pdfSymbolDim_;
      const int patternXBoxEnd = patternXBoxStart + (widthToUse/pdfSymbolDim_);
      const int patternYBoxStart = ((y-1)*heightPerPage)/pdfSymbolDim_;
      const int patternYBoxEnd = patternYBoxStart + (heightToUse/pdfSymbolDim_);
      for (int j = patternYBoxStart, jj = 0; j < patternYBoxEnd;
            ++j, ++jj) {
        for (int i = patternXBoxStart, ii = 0; i < patternXBoxEnd;
              ++i, ++ii) {
          const QPixmap& thisSymbol =
            symbolMap[squareImage.pixel(i*squareDim, j*squareDim)];
          painter->drawPixmap(xstart + ii*pdfSymbolDim_,
                              ystart + jj*pdfSymbolDim_, thisSymbol);
        }
      }

      //// draw grid lines and counts (thick every 5, thin every 1)
      const int thickCount = 5;
      //// x grid lines
      painter->setPen(QPen(Qt::black, 1));
      int tx = 0;
      //// draw the thin x grid lines
      while (tx*pdfSymbolDim_ <= widthToUse) {
        painter->drawLine(tx*pdfSymbolDim_ + xstart, ystart,
                          tx*pdfSymbolDim_ + xstart, heightToUse + ystart);
        tx += 1;
      }
      //// draw the thin y grid lines
      int ty = 0;
      while (ty*pdfSymbolDim_ <= heightToUse) {
        painter->drawLine(xstart, ty*pdfSymbolDim_ + ystart, widthToUse + xstart,
                     ty*pdfSymbolDim_ + ystart);
        ty += 1;
      }

      //// thick lines
      tx = 0; // x grid count for this page
      if ((x-1)*xBoxesPerPage % thickCount != 0) {
        // to the next multiple of thickCount
        tx = thickCount - ((x-1)*xBoxesPerPage % thickCount);
      }

      const int savedTx = tx;
      // draw the x grid counts
      painter->setPen(QPen(Qt::black, 1));
      while (tx*pdfSymbolDim_ <= widthToUse) {
        const int tgridx = (x-1)*xBoxesPerPage + tx;
        if (tx == 0) { // avoid collision
          painter->drawText(xstart + tx*pdfSymbolDim_, ystart - f,
                            ::itoqs(tgridx));
        }
        else {
          painter->drawText(xstart + tx*pdfSymbolDim_ - sWidth(tgridx),
                            ystart - f, ::itoqs(tgridx));
        }
        tx += thickCount;
      }

      // draw the thick x grid lines
      tx = savedTx;
      painter->setPen(QPen(Qt::darkGray, 3));
      while (tx*pdfSymbolDim_ <= widthToUse) {
        painter->drawLine(tx*pdfSymbolDim_ + xstart, ystart,
                          tx*pdfSymbolDim_ + xstart, heightToUse + ystart);
        tx += thickCount;
      }

      // draw the final line
      if ((x-1)*(widthPerPage) + tx*pdfSymbolDim_ > patternImageWidth) {
        painter->setPen(QPen(Qt::black, 1));
        painter->drawText(xstart + widthToUse - sWidth(xBoxes), ystart - f,
                          ::itoqs(xBoxes));
        painter->setPen(QPen(Qt::darkGray, 3));
        painter->drawLine(widthToUse + xstart, ystart, widthToUse + xstart,
                          heightToUse + ystart);
      }

      ty = 0; // y grid count for this page
      if ((y-1)*yBoxesPerPage % thickCount != 0) {
        // to the next multiple of 5
        ty = thickCount - ((y-1)*yBoxesPerPage % thickCount);
      }

      const int savedTy = ty;
      // draw the y grid counts
      painter->setPen(QPen(Qt::black, 1));
      while (ty*pdfSymbolDim_ <= heightToUse) {
        const int tgridy = (y-1)*yBoxesPerPage + ty;
        if (ty == 0) { // avoid confusion
          painter->drawText(xstart - sWidth(tgridy) - f,
                       ty*pdfSymbolDim_ + ystart + sHeight(tgridy), ::itoqs(tgridy));
        }
        else {
          painter->drawText(xstart - sWidth(tgridy) - f, ty*pdfSymbolDim_ + ystart,
                       ::itoqs(tgridy));
        }
        ty += thickCount;
      }

      // draw the thick y grid lines
      ty = savedTy;
      painter->setPen(QPen(Qt::darkGray, 3));
      while (ty*pdfSymbolDim_ <= heightToUse) {
        painter->drawLine(xstart, ty*pdfSymbolDim_ + ystart, widthToUse + xstart,
                          ty*pdfSymbolDim_ + ystart);
        ty += thickCount;
      }

      // draw the final line
      if ((y-1)*(heightPerPage) + ty*pdfSymbolDim_ > patternImageHeight) {
        painter->setPen(QPen(Qt::black, 1));
        painter->drawText(xstart - sWidth(yBoxes) - f, heightToUse + ystart,
                          ::itoqs(yBoxes));
        painter->setPen(QPen(Qt::darkGray, 3));
        painter->drawLine(xstart, heightToUse + ystart, widthToUse + xstart,
                          heightToUse + ystart);
      }

      painter->setPen(QPen(Qt::black, 1)); // reset

      if (x < xpages || y < ypages) {
        printer->newPage();
      }
    }
  }
  return false;
}

int patternWindow::drawPdfLegend(QPainter* painter,
                                 int xpages, int ypages,
                                 int imageWidth, int imageHeight,
                                 int widthPerPage, int heightPerPage,
                                 int printerWidth, int printerHeight) const
{
  // max page number width
  const int maxPageNumWidth =
    sWidth(QString(::itoqs(xpages * ypages).size(), '5')) + 10;
  const int maxPageNumHeight = sHeight("5");

  // draw the page number for this page
  const int thisPageNum = xpages * ypages + 1;
  painter->drawText(printerWidth - sWidth(thisPageNum),
                    sHeight(thisPageNum), ::itoqs(thisPageNum));

  const qreal minLegendWidth = xpages * maxPageNumWidth;
  const qreal minLegendHeight = ypages * maxPageNumHeight;
  qreal ratio; // legend to original
  qreal legendWidth, legendHeight;
  if (imageWidth > imageHeight) {
    legendWidth = printerWidth/4 > minLegendWidth ?
      printerWidth/4 : minLegendWidth;
    ratio = legendWidth/imageWidth;
    legendHeight = ratio * imageHeight;
  }
  else {
    legendHeight = printerHeight/5 > minLegendHeight ?
      printerHeight/5 : minLegendHeight;
    ratio = legendHeight/imageHeight;
    legendWidth = ratio * imageWidth;
  }
  if (widthPerPage > imageWidth) {
    widthPerPage = imageWidth;
  }
  if (heightPerPage > imageHeight) {
    heightPerPage = imageHeight;
  }
  // width of each "page" on the legend
  qreal legendBoxWidth = widthPerPage*ratio;
  if (legendBoxWidth < maxPageNumWidth) {
    legendWidth = (legendWidth/legendBoxWidth)*maxPageNumWidth;
    legendBoxWidth = maxPageNumWidth;
  }
  // floating point zero (not, but good enough)
  qreal epsilon = .0000001;
  // xx is the last "page" box width
  qreal xx = legendWidth - floor(legendWidth/legendBoxWidth)*legendBoxWidth;
  if (xx < epsilon) { // computer zero
    //qDebug() << "xx = 0:" << xx;
    xx = legendBoxWidth;
  }
  else if (xx > 0 && xx < maxPageNumWidth) {
    //qDebug() << "xx < maxPageNumWidth:" << xx << maxPageNumWidth <<
    //legendBoxWidth;
    legendWidth -= xx;
    xx = (maxPageNumWidth > legendBoxWidth) ?
      legendBoxWidth : maxPageNumWidth;
    legendWidth += xx;
  }

  // height of each "page" box on the legend
  qreal legendBoxHeight = heightPerPage*ratio;
  if (legendBoxHeight < maxPageNumHeight) {
    legendHeight = (legendHeight/legendBoxHeight)*maxPageNumHeight;
    legendBoxHeight = maxPageNumHeight;
  }
  // yy is the last "page" box height
  qreal yy = legendHeight -
    floor(legendHeight/legendBoxHeight)*legendBoxHeight;
  if (yy < epsilon) {
    //qDebug() << "yy = 0:" << yy;
    yy = legendBoxHeight;
  }
  else if (yy > 0 && yy < maxPageNumHeight) {
    legendHeight -= yy;
    yy = (maxPageNumHeight > legendBoxHeight) ?
      legendBoxHeight : maxPageNumHeight;
    legendHeight += yy;
  }

  // the border
  const qreal xstart = printerWidth/2 - legendWidth/2;
  const qreal ystart = pdfSymbolDim_;
  painter->drawRect(QRectF(xstart, ystart, legendWidth, legendHeight));
  // the interior vertical lines
  for (int i = 1; i*legendBoxWidth < legendWidth; ++i) {
    painter->drawLine(QPointF(xstart + i*legendBoxWidth, ystart),
                      QPointF(xstart + i*legendBoxWidth,
                              ystart + legendHeight));
  }
  // the interior horizontal lines
  for (int j = 1; j*legendBoxHeight < legendHeight; ++j) {
    painter->drawLine(QPointF(xstart, ystart + j*legendBoxHeight),
                      QPointF(xstart + legendWidth,
                              ystart + j*legendBoxHeight));
  }
  // add the page numbers to the legend boxes
  for (int x = 1; x <= xpages; ++x) {
    for (int y = 1; y <= ypages; ++y) {
      const qreal thisBoxWidth = (x < xpages || xpages == 1) ?
        legendBoxWidth : xx;
      const qreal thisBoxHeight = (y < ypages || ypages == 1) ?
        legendBoxHeight : yy;
      const int pageNum = ypages*(x-1) + y;
      painter->
        drawText(QPointF(xstart+(x-1)*legendBoxWidth + thisBoxWidth/2 -
                         (qreal)sWidth(pageNum)/2,
                         ystart+(y-1)*legendBoxHeight + thisBoxHeight/2 +
                         (qreal)sHeight(pageNum)/4),
                 ::itoqs(pageNum));
    }
  }
  return ystart + legendHeight;
}

void patternWindow::drawPdfColorList(QPainter* painter, QPrinter* printer,
                                     int pageNum, int yused) const {

  const QRect printerRectangle = printer->pageRect();
  const int printerWidth = printerRectangle.width();
  const int printerHeight = printerRectangle.height();
  // have the list font match the symbol size, within reason
  int symbolDim = qMax(pdfSymbolDim_, sHeight("B"));
  symbolDim = qMin(symbolDim, 40);
  const QFont originalFont(painter->font());
  ::setFontHeight(painter, symbolDim);
  const QFont listFont = painter->font();

  const QFontMetrics listFontMetric(listFont);
  const int fontHeight = listFontMetric.height();

  //// list color count and box dimensions
  const int xBoxes =
    curImage_->squareImage().width()/curImage_->squareDimension();
  const int yBoxes =
    curImage_->squareImage().height()/curImage_->squareDimension();
  QRect textBoundingRect;
  painter->drawText(QRect(0, yused + fontHeight, printerWidth, 4*fontHeight),
                    Qt::TextWordWrap,
                    "The pattern uses " + ::itoqs(curImage_->colors().size()) +
                    " colors and is " + ::itoqs(xBoxes) + " squares wide by " +
                    ::itoqs(yBoxes) + " squares high.", &textBoundingRect);
  yused += textBoundingRect.height() + fontHeight;
  if (!colorsAreDmc(curImage_->colors())) {
    // line wrap the explanation for non-DMC colors
    // (or just use the QRect version of drawText as above)
    const QString nonDmcString("For any color that isn't DMC, the Code column gives the RGB value of the color"
                               " and the Name column gives the code and DMC name of the nearest DMC color.");
    const QStringList partsList = nonDmcString.split(" ");
    int numLines = 1;
    const int spaceWidth = listFontMetric.width(" ");
    QString stringSoFar = partsList[0];
    int widthSoFar = listFontMetric.width(stringSoFar);
    for (int i = 1, size = partsList.size(); i < size; ++i) {
      if (widthSoFar + listFontMetric.width(partsList[i]) + spaceWidth <
          printerWidth) {
        stringSoFar += " " + partsList[i];
        widthSoFar += listFontMetric.width(partsList[i] + " ");
      }
      else { // new line
        painter->drawText(0, yused + numLines*fontHeight, stringSoFar);
        ++numLines;
        stringSoFar = partsList[i];
        widthSoFar = listFontMetric.width(stringSoFar);
      }
    }
    // draw the last line
    painter->drawText(0, yused + numLines*fontHeight, stringSoFar);
    yused += numLines*fontHeight;
  }
  // save this height so it can be restored for a second column
  const int yusedSaved = yused;

  //// now draw the color list
  QFont boldFont = listFont;
  boldFont.setBold(true);
  painter->setFont(boldFont);
  const QFontMetrics boldFontMetric(boldFont);

  //// tab stops
  const int padding = 10;
  const int swatchTab = symbolDim + 5;
  const int countTab = 2 * swatchTab + padding;
  const int codeTab = countTab + listFontMetric.width("999999") + padding;
  const int nameTab = codeTab + listFontMetric.width("255 255 255") + padding;
  const int endTab = nameTab + boldFontMetric.width("~8888:Ultra V DK Turquoise");

  drawListHeader(painter, 0, yused + 2*fontHeight, countTab, codeTab, nameTab);
  yused += 2*fontHeight;
  painter->drawLine(0, yused + 3, endTab, yused + 3);
  yused += 5;
  painter->setFont(listFont);

  int xtab = 0;
  bool partial = true; // the first page list may be a partial page
  QVector<floss> flossVector = ::rgbToFloss(curImage_->colors());
  qSort(flossVector.begin(), flossVector.end());

  // build a color count map
  QHash<QRgb, int> countsHash;
  ::colorCounts(curImage_->squareImage(), curImage_->squareDimension(),
                &countsHash);

  QPixmap thisSymbol;
  QPainter symbolPainter;
  QPixmap thisPixmap(symbolDim, symbolDim);
  QString thisCodeString;
  for (QVector<floss>::const_iterator it = flossVector.begin(),
          end = flossVector.end(); it != end; ++it) {
    if (yused + fontHeight > printerHeight) {
      // if we're currently in a second column or there isn't room for a
      // second column, then start a new page
      if (xtab > 0 || endTab + 50 + endTab > printerWidth) {
        xtab = 0;
        printer->newPage();
        // page #
        painter->save();
        painter->setFont(originalFont);
        painter->drawText(printerWidth - sWidth(pageNum), sHeight(pageNum),
                          ::itoqs(pageNum));
        painter->restore();
        ++pageNum;
        partial = false;
        yused = 0;
        painter->setFont(boldFont);
        drawListHeader(painter, xtab, fontHeight, countTab, codeTab, nameTab);
        painter->setFont(listFont);
        yused += fontHeight;
        painter->drawLine(xtab, yused + 3, endTab, yused + 3);
        yused += 5;
      }
      else { // start a second column
        xtab = endTab + 50;
        painter->setFont(boldFont);
        if (partial) { // second column on first page of listing
          yused = yusedSaved;
          drawListHeader(painter, xtab, 2*fontHeight + yused, countTab,
                         codeTab, nameTab);
          yused += 2*fontHeight;
        }
        else {
          yused = 0;
          drawListHeader(painter, xtab, fontHeight, countTab, codeTab,
                         nameTab);
          yused += fontHeight;
        }
        painter->setFont(listFont);
        painter->drawLine(xtab, yused + 3, endTab + xtab, yused + 3);
        yused += 5;
      }
    }
    //// symbol
    thisSymbol = curImage_->symbolNoBorder((*it).color(), symbolDim);
    symbolPainter.begin(&thisSymbol);
    symbolPainter.drawRect(0, 0, thisSymbol.width()-1, 
                           thisSymbol.height()-1);
    symbolPainter.end();
    painter->drawPixmap(xtab, yused + 5, thisSymbol);
    //// color swatch ("sample")
    thisPixmap.fill((*it).color().qc());
    painter->drawPixmap(swatchTab + xtab + 2, yused + 5, thisPixmap);
    painter->drawRect(swatchTab + xtab + 2, yused + 5,
                      symbolDim, symbolDim);
    //// color count
    painter->drawText(countTab + xtab, yused + symbolDim,
                      ::itoqs(countsHash[(*it).color().qrgb()]));
    //// floss code (or rgb code)
    const int code = (*it).code();
    if (code != -1) { // valid floss
      if (code >= 0) {
        painter->drawText(codeTab + xtab, yused+symbolDim, ::itoqs(code));
      }
      else { // the dmc code is a string in this case
        thisCodeString = "N/A";
        if (code == WHITE_CODE) {
          thisCodeString = "White";
        }
        else if (code == ECRU_CODE) {
          thisCodeString = "Ecru";
        }
        else {
          qWarning() << "String code error.";
        }
        painter->drawText(codeTab + xtab, yused + symbolDim, thisCodeString);
      }
    }
    else { // insert the rgb code instead of the dmc code
      painter->drawText(codeTab + xtab, yused + symbolDim,
                        ::ctos((*it).color()));
    }
    //// color name
    painter->drawText(nameTab + xtab, yused + symbolDim, (*it).name());

    yused += symbolDim + 5;
  }
}

void patternWindow::drawListHeader(QPainter* painter, int xStart, int y,
                                   int countTab, int codeTab,
                                   int nameTab) const {

  painter->drawText(xStart + countTab, y, tr("Count"));
  painter->drawText(xStart + codeTab, y, tr("Code"));
  painter->drawText(xStart + nameTab, y, tr("Name"));
}

bool patternWindow::eventFilter(QObject* watched, QEvent* event) {

  if (event->type() == QEvent::QEvent::KeyPress) {
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
    QMessageBox::information(this, curImage_->name(), curImage_->name() +
                             tr(" currently has dimensions ") +
                             ::itoqs(width) + "x" + ::itoqs(height) +
                             tr(", box dimension ") + ::itoqs(symbolDim) +
                             tr(", and is ") + ::itoqs(xBoxes) +
                             tr(" by ") + ::itoqs(yBoxes) + tr(" boxes."));
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
    qWarning() << "Misplaced container on patternHistoryRestore" <<
      imageIndex;
  }
}

void patternWindow::updateCurrentSettings(const QDomElement& xml) {

  QDomElement settings(xml.firstChildElement("pattern_window_settings"));
  if (settings.isNull()) {
    return;
  }
  imageLabel_->setGridColor(::xmlStringToRgb(::getElementText(settings,
                                                           "grid_color")));
  processGridChange(::stringToBool(::getElementText(settings, "grid_on")));
  const int currentIndex =
    ::getElementText(settings, "current_index").toInt();
  patternImagePtr container = getImageFromIndex(currentIndex);
  if (container) {
    setCur(container);
  }
  else {
    qWarning() << "Misplaced container on patternUpdateSettings" <<
      currentIndex;
  }
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
    if (QFile::exists("/usr/bin/evince")) {
      pdfViewerPath_ = "/usr/bin/evince";
    }
    else if (QFile::exists("/usr/bin/kpdf")) {
      pdfViewerPath_ = "/usr/bin/kpdf";
    }
    else if (QFile::exists("/usr/bin/okular")) {
      pdfViewerPath_ = "/usr/bin/okular";
    }
    else if (QFile::exists("/usr/bin/xpdf")) {
      pdfViewerPath_ = "/usr/bin/xpdf";
    }
#endif
#ifdef Q_OS_WIN
    // try to find acroread
    QDir dir("C:/Program Files/Adobe/");
    if (dir.exists()) {
      // sort by time - if somebody has multiple versions of Reader
      // installed (not officially supported by Adobe) and they
      // install an older version after a newer version then this will
      // choose the wrong version
      QStringList readers = dir.entryList(QStringList("Reader *"), QDir::Dirs,
                                          QDir::Time|QDir::Reversed);
      if (!readers.empty() && dir.exists(readers[0] + "/Reader")) {
        dir.cd(readers[0] + "/Reader");
        if (dir.exists("AcroRd32.exe")) {
          pdfViewerPath_ = dir.absoluteFilePath("AcroRd32.exe");
        }
        else if (dir.exists("AcroRd64.exe")) {
          pdfViewerPath_ = dir.absoluteFilePath("AcroRd64.exe");
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
  
