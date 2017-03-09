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

#include "imageZoomWindow.h"

#include <QtCore/QString>

#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QMessageBox>

#include "windowManager.h"
#include "utility.h"
#include "imageUtility.h"
#include "helpBrowser.h"
#include "quickHelp.h"
#include "floss.h"

extern const int ZOOM_INCREMENT = 100;

imageZoomWindow::imageZoomWindow(const QString& dockName,
                                 windowManager* winMgr)
  : Original_(tr("Original")), constructedAndShown_(false),
    windowManager_(winMgr) {

  createDock(dockName);
  createMenus();
  createToolbarAndMenuActions();
  createStatusBar();
}

void imageZoomWindow::createDock(const QString& dockName) {

  // the color/symbol list dock widget
  dockHolder_ = new QDockWidget(dockName, this);
  dockHolder_->setFeatures(QDockWidget::NoDockWidgetFeatures);
}

void imageZoomWindow::createMenus() {

  fileMenu_ = menuBar()->addMenu(tr("&File"));
  imageMenu_ = menuBar()->addMenu(tr("&Image"));
  helpMenu_ = menuBar()->addMenu(tr("&Help"));
}

void imageZoomWindow::createToolbarAndMenuActions() {

  toolBar_ = addToolBar(tr("Toolbar"));
  toolBar_->setMovable(false);

  // users add these zoom tools using addToolbarZoomIcons()
  zoomInAction_ = new QAction(QIcon(":zoomIn.png"), tr("Zoom in"), this);
  //zoomInAction_->setShortcut(QKeySequence(QKeySequence::ZoomIn));
  zoomInAction_->setShortcut(QKeySequence("Ctrl+="));
  zoomOutAction_ = new QAction(QIcon(":zoomOut.png"), tr("Zoom out"), this);
  zoomOutAction_->setShortcut(QKeySequence("Ctrl+-"));
  originalSizeAction_ = new QAction(QIcon(":zoomToOriginal.png"),
                                    tr("Original size"), this);
  zoomToWidthAction_ = new QAction(QIcon(":zoomToWidth.png"),
                                   tr("Zoom to width"), this);
  zoomToHeightAction_ = new QAction(QIcon(":zoomToHeight.png"),
                                    tr("Zoom to height"), this);
  zoomToImageAction_ = new QAction(QIcon(":zoomToImage.png"),
                                   tr("Zoom to fit"), this);
  zoomToWidthAction_->setShortcut(QKeySequence("Ctrl+8"));
  zoomToHeightAction_->setShortcut(QKeySequence("Ctrl+9"));
  zoomToImageAction_->setShortcut(QKeySequence("Ctrl+0"));

  openImageAction_ = new QAction(QIcon(":openImage.png"),
                                 tr("Open a new image"), this);
  openImageAction_->setShortcut(QKeySequence("Ctrl+o"));
  //  openImageAction_->setShortcut(QKeySequence(QKeySequence::Open));
  fileMenu_->addAction(openImageAction_);
  toolBar_->addAction(openImageAction_);

  openProjectAction_ = new QAction(QIcon(":openProject.png"),
                                   tr("Open a saved project"), this);
  openProjectAction_->setShortcut(QKeySequence("Ctrl+Shift+o"));
  fileMenu_->addAction(openProjectAction_);
  toolBar_->addAction(openProjectAction_);

  saveAsAction_ = new QAction(QIcon(":saveProjectAs.png"),
                              tr("Save project as"), this);
  saveAsAction_->setShortcut(QKeySequence("Ctrl+Shift+s"));
  fileMenu_->addAction(saveAsAction_);
  saveAction_ = new QAction(QIcon(":saveProject.png"),
                            tr("Save project"), this);
  saveAction_->setShortcut(QKeySequence("Ctrl+s"));
  fileMenu_->addAction(saveAction_);
  toolBar_->addAction(saveAction_);

  quitAction_ = fileMenu_->addAction(tr("Quit"));
  quitAction_->setShortcut(QKeySequence("Ctrl+q"));

  imageInfoAction_ = new QAction(tr("Image information"), this);
  imageInfoAction_->setShortcut(QKeySequence("Ctrl+i"));

  quickHelpAction_ = new QAction(QIcon(":quickHelp.png"), tr("Quick Help"),
                           this);
  quickHelpAction_->setShortcut(Qt::Key_F1);
  helpAction_ = new QAction(QIcon(":help.png"), tr("Help"), this);
  helpAboutAction_ = new QAction(QIcon(":about.png"), tr("About"), this);
  helpMenu_->addAction(quickHelpAction_);
  helpMenu_->addAction(helpAction_);
  helpMenu_->addAction(helpAboutAction_);

  connect(openImageAction_, SIGNAL(triggered()),
          this, SLOT(open()));
  connect(saveAsAction_, SIGNAL(triggered()),
          windowManager_, SLOT(saveAs()));
  connect(saveAction_, SIGNAL(triggered()),
          windowManager_, SLOT(save()));
  connect(openProjectAction_, SIGNAL(triggered()),
          windowManager_, SLOT(openProject()));
  connect(originalSizeAction_, SIGNAL(triggered()),
          this, SLOT(originalSize()));
  connect(zoomInAction_, SIGNAL(triggered()),
          this, SLOT(zoomIn()));
  connect(zoomOutAction_, SIGNAL(triggered()),
          this, SLOT(zoomOut()));
  connect(zoomToWidthAction_, SIGNAL(triggered()),
          this, SLOT(zoomToWidth()));
  connect(zoomToHeightAction_, SIGNAL(triggered()),
          this, SLOT(zoomToHeight()));
  connect(zoomToImageAction_, SIGNAL(triggered()),
          this, SLOT(zoomToImage()));
  connect(quitAction_, SIGNAL(triggered()),
          this, SLOT(quit()));
  connect(imageInfoAction_, SIGNAL(triggered()),
          this, SLOT(displayImageInfo()));
  connect(quickHelpAction_, SIGNAL(triggered()),
          this, SLOT(showQuickHelp()));
  connect(helpAction_, SIGNAL(triggered()),
          this, SLOT(helpRequested()));
  connect(helpAboutAction_, SIGNAL(triggered()),
          this, SLOT(helpAbout()));
}

void imageZoomWindow::createStatusBar() {

  status_ = new QLabel(this);
  // 1 means normal status gets as much width as possible
  statusBar()->addWidget(status_, 1);
  permanentStatus_ = new QLabel(this);
  // 0 means permanent status gets no more width than needed
  statusBar()->addPermanentWidget(permanentStatus_, 0);
}

void imageZoomWindow::addWindowActions(const QList<QAction*>& windowActions,
                                       QMenu* windowMenu) {

  for (int i = 0, size = windowActions.size(); i < size; ++i) {
    toolBar_->addAction(windowActions[i]);
  }
  menuBar()->insertMenu(helpMenu_->menuAction(), windowMenu);
}

void imageZoomWindow::addRecentlyOpenedMenus(QMenu* imagesMenu,
                                             QMenu* projectsMenu) {

  fileMenu_->insertMenu(saveAsAction_, imagesMenu);
  fileMenu_->insertMenu(saveAsAction_, projectsMenu);
}

void imageZoomWindow::addZoomActionsToImageMenu() {

  imageMenu_->addAction(zoomToImageAction_);
  imageMenu_->addAction(zoomToWidthAction_);
  imageMenu_->addAction(zoomToHeightAction_);
  imageMenu_->addAction(zoomInAction_);
  imageMenu_->addAction(zoomOutAction_);
  imageMenu_->addAction(originalSizeAction_);
}

void imageZoomWindow::addToolbarZoomIcons() {

  toolBar_->addAction(zoomInAction_);
  toolBar_->addAction(zoomOutAction_);
  toolBar_->addAction(zoomToWidthAction_);
  toolBar_->addAction(zoomToHeightAction_);
}

void imageZoomWindow::setZoomActionsEnabled(bool b) {

  zoomToImageAction_->setEnabled(b);
  zoomToWidthAction_->setEnabled(b);
  zoomToHeightAction_->setEnabled(b);
  zoomInAction_->setEnabled(b);
  zoomOutAction_->setEnabled(b);
  originalSizeAction_->setEnabled(b);
}

bool imageZoomWindow::quit() {

  const int returnCode = QMessageBox::warning(this, "Cstitch",
    tr("All work will be lost if you continue; are you sure you want to quit?"),
                                QMessageBox::Cancel | QMessageBox::Ok,
                                QMessageBox::Ok);

  if (returnCode == QMessageBox::Ok) {
    windowManager_->quit();
    exit(0);
  }
  else {
    return false;
  }
}

void imageZoomWindow::open() {

  windowManager_->openNewImage();
}

const QImage& imageZoomWindow::originalImage() const {

  return winManager()->originalImage();
}

void imageZoomWindow::displayOriginalImageInfo(int width, int height) {

  if (!imageMenu_->isEnabled()) {
    return;
  }
  const int colorCount = windowManager_->getOriginalImageColorCount();
  QMessageBox::information(this,
                           tr("Original Image"),
                           //: In English the singular version would be "color"
                           //: and the plural would be "unique colors" - add
                           //: "unique" in the plural case when possible
                           tr("The original image currently has dimensions "
                              "%1x%2 and contains %n color(s).", "", colorCount)
                           .arg(::itoqs(width))
                           .arg(::itoqs(height)));
}

void imageZoomWindow::setStatus(const QString& status) {

  status_->setText(status);
  status_->setToolTip(status);
}

void imageZoomWindow::setPermanentStatus(const QString& status) {

  permanentStatus_->setText("<b>" + status + "</b>");
  permanentStatus_->setToolTip(status);
}

void imageZoomWindow::showTemporaryStatusMessage(const QString& status,
                                                 int duration) {

  statusBar()->showMessage(status, duration);
}

int imageZoomWindow::imageNameToIndex(const QString& imageName) const {

  if (imageName == Original_) {
    return 0;
  }
  else {
    QRegExp regularExpression("Image (\\d+)");
    regularExpression.indexIn(imageName);
    const QString indexString = regularExpression.cap(1);

    return indexString.isNull() ? -1 : indexString.toInt();
  }
}

QString imageZoomWindow::imageNameFromIndex(int index) const {

  if (index == 0) {
    return Original_;
  }
  else {
    return QString("Image ") + ::itoqs(index);
  }
}

void imageZoomWindow::closeEvent(QCloseEvent* event) {

  const bool reallyQuit = quit();
  if (!reallyQuit) {
    event->ignore();
  }
}

void imageZoomWindow::showEvent(QShowEvent* ) {

  if (constructedAndShown_) {
    return;
  }
  else {
    constructedAndShown_ = true;
    processFirstShow();
  }
}

bool imageZoomWindow::eventFilter(QObject* watched, QEvent* event) {

  if (event->type() == QEvent::Wheel) {
    QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
    if (wheelEvent->modifiers() == Qt::ControlModifier) {
      const int yDelta = wheelEvent->angleDelta().y();
      bool zoomed = false;
      if (yDelta > 0) {
        zoomIn();
        zoomed = true;
      }
      else if (yDelta < 0) {
        zoomOut();
        zoomed = true;
      }
      if (zoomed) {
        wheelEvent->accept();
        return true;
      }
    }
    else if (wheelEvent->modifiers() == Qt::ShiftModifier) {
      return horizontalWheelScrollEvent(watched, wheelEvent);
    }
  }
  return QMainWindow::eventFilter(watched, event);
}

void imageZoomWindow::helpAbout() {

  QMessageBox aboutBox(this);
  aboutBox.setWindowTitle(tr("About"));
  const QString version = winManager()->getProgramVersion();
  aboutBox.setText("<b>Cstitch</b><br />" + 
                   tr("Version: %1<br />"
                      "Tom Klein<br />email: "
                      "tomklein@users.sourceforge.net<br />"
                      "http://cstitch.sourceforge.net/").arg(version));
  aboutBox.setIconPixmap(QPixmap(":aboutIcon.png"));
  aboutBox.exec();
}

void imageZoomWindow::helpRequested() const {

  helpBrowser::loadHelp(getHelpMode());
}

void imageZoomWindow::showQuickHelp(bool show) {

  if (show) {
    quickHelp::loadQuickHelp(getHelpMode(), this);
  }
  else {
    quickHelp::closeCurrentWindow();
  }
}

void imageZoomWindow::processFirstShow() {

  // this may be occuring before the actual show, so clear the event
  // queue to make sure that show has taken place
  clearEventQueue();
}


//// otherwise inline functions defined here to avoid includes - none of
//// these are performance sensitive
void imageZoomWindow::addQuickHelp(QAction* autoShowQuickHelp) {
  helpMenu_->insertAction(quickHelpAction_, autoShowQuickHelp);
}

void imageZoomWindow::showListDock() { dockHolder_->show(); }

void imageZoomWindow::hideListDock() { dockHolder_->hide(); }

bool imageZoomWindow::listDockIsVisible() const {
  return dockHolder_->isVisible(); 
}

void imageZoomWindow::setListDockEnabled(bool b) {
  dockHolder_->setEnabled(b); 
}

void imageZoomWindow::setListDockWidget(QWidget* widget) {
  dockHolder_->setWidget(widget);
  addDockWidget(Qt::RightDockWidgetArea, dockHolder_);
}

void imageZoomWindow::setListDockTitle(const QString& title) {
  dockHolder_->setWindowTitle(title);
}

void imageZoomWindow::addToolbarAction(QAction* action) {
  toolBar_->addAction(action);
}

void imageZoomWindow::addToolbarWidget(QWidget* widget) {
  toolBar_->addWidget(widget);
}

void imageZoomWindow::addToolbarSeparator() {toolBar_->addSeparator(); }

void imageZoomWindow::setToolbarEnabled(bool b) { toolBar_->setEnabled(b); }

void imageZoomWindow::setSaveActionsEnabled(bool b) {
  saveAction_->setEnabled(b);
  saveAsAction_->setEnabled(b);
}

void imageZoomWindow::setPermanentStatusEnabled(bool b) {
  permanentStatus_->setEnabled(b);
}

QString imageZoomWindow::imageInfoFlossString(flossType type) const {

  const QString flossTypeText = type.shortText();
  if (flossTypeText != "") {
    //: %1 is "DMC" or "Anchor" or ...
    //: this string will get added to the end of a sentence, as in:
    //: "This image contains only DMC colors"
    return tr("contains only %1 colors").arg(flossTypeText);
  }
  else {
    return "";
  }
}
  
