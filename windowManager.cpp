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

#include "windowManager.h"

#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QDateTime>
#include <QtConcurrent/QtConcurrentRun>

#include <QtWidgets/QMenu>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QImageWriter>

#include "colorChooser.h"
#include "colorCompare.h"
#include "fileListMenu.h"
#include "imageUtility.h"
#include "patternWindow.h"
#include "squareWindow.h"
#include "versionProcessing.h"
#include "xmlUtility.h"

windowManager* windowManager::winManager_ = NULL;

windowManager::windowManager() : originalImageColorCount_(0),
                                 projectFilename_(QString()),
                                 hideWindows_(false) {

  const QString chooserText(tr("Color Chooser (1/4)"));
  const QString compareText(tr("Color compare (2/4)"));
  const QString squareText(tr("Square compare (3/4)"));
  const QString patternText(tr("Pattern (4/4)"));
  colorChooserAction_ = new QAction(QIcon(":colorChooser.png"),
                                    chooserText, this);
  colorCompareAction_ = new QAction(QIcon(":colorCompare.png"),
                                    compareText, this);
  squareWindowAction_ = new QAction(QIcon(":square.png"),
                                     squareText, this);
  patternWindowAction_ = new QAction(QIcon(":pattern.png"),
                                     patternText, this);
  activeColorChooserAction_ = new QAction(QIcon(":colorChooser_active.png"),
                                          chooserText, this);
  activeColorCompareAction_ = new QAction(QIcon(":colorCompare_active.png"),
                                          compareText, this);
  activeSquareWindowAction_ = new QAction(QIcon(":square_active.png"),
                                           squareText, this);
  activePatternWindowAction_ = new QAction(QIcon(":pattern_active.png"),
                                           patternText, this);
  colorChooserAction_->setEnabled(false);
  colorChooserAction_->setShortcut(QKeySequence(QString(tr("Ctrl+1"))));
  colorCompareAction_->setEnabled(false);
  colorCompareAction_->setShortcut(QKeySequence(QString(tr("Ctrl+2"))));
  squareWindowAction_->setEnabled(false);
  squareWindowAction_->setShortcut(QKeySequence(QString(tr("Ctrl+3"))));
  patternWindowAction_->setEnabled(false);
  patternWindowAction_->setShortcut(QKeySequence(QString(tr("Ctrl+4"))));

  windowMenu_ = new QMenu(tr("&Windows"));
  connect(windowMenu_, SIGNAL(triggered(QAction*)),
          this, SLOT(displayActionWindow(QAction*)));
  windowMenu_->addAction(colorChooserAction_);
  windowMenu_->addSeparator();
  windowMenu_->addAction(colorCompareAction_);
  windowMenu_->addSeparator();
  windowMenu_->addAction(squareWindowAction_);
  windowMenu_->addSeparator();
  windowMenu_->addAction(patternWindowAction_);

  const QSettings settings("cstitch", "cstitch");
  const int recentFilesLength = 7;
  QStringList recentImages;
  if (settings.contains("recent_images")) {
    recentImages = 
      ::existingFiles(settings.value("recent_images").toStringList());
  }
  recentImagesMenu_ = new fileListMenu(tr("Recent images"),
                                       recentFilesLength,
                                       recentImages);
  connect(recentImagesMenu_, SIGNAL(triggered(const QString& )),
          this, SLOT(openRecentImage(const QString& )));
  if (recentImages.isEmpty()) {
    recentImagesMenu_->setEnabled(false);
  }
  QStringList recentProjects;
  if (settings.contains("recent_projects")) {
    recentProjects =
      ::existingFiles(settings.value("recent_projects").toStringList());
  }

  recentProjectsMenu_ = new fileListMenu(tr("Recent projects"),
                                         recentFilesLength,
                                         recentProjects);
  connect(recentProjectsMenu_, SIGNAL(triggered(const QString& )),
          this, SLOT(openRecentProject(const QString& )));
  if (recentProjects.isEmpty()) {
    recentProjectsMenu_->setEnabled(false);
  }

  autoShowQuickHelp_ = new QAction(tr("Auto display Quick Help windows"),
                                   this);
  autoShowQuickHelp_->setCheckable(true);
  // restore the autoShowQuickHelp_ setting if it exists
  if (settings.contains("auto_show_quick_help")) {
    const bool checked = settings.value("auto_show_quick_help").toBool();
    autoShowQuickHelp_->setChecked(checked);
  }
  else {
    autoShowQuickHelp_->setChecked(true);
  }
  // make this connection _after_ the initial checkbox has been set or
  // else quickHelp will be called on a non-existent active window!
  connect(autoShowQuickHelp_, SIGNAL(toggled(bool )),
          this, SLOT(autoShowQuickHelp(bool )));
}

void windowManager::configureNewWindow(imageZoomWindow* window,
                                       windowStage stage) {

  QAction* stage0 = (stage == CHOOSER) ?
    activeColorChooserAction_ : colorChooserAction_;
  QAction* stage1 = (stage == COMPARE) ?
    activeColorCompareAction_ : colorCompareAction_;
  QAction* stage2 = (stage == SQUARE) ?
    activeSquareWindowAction_ : squareWindowAction_;
  QAction* stage3 = (stage == PATTERN) ?
    activePatternWindowAction_ : patternWindowAction_;

  QList<QAction*> actions;
  actions << stage0 << stage1 << stage2 << stage3;

  window->addWindowActions(actions, windowMenu_);
  window->addRecentlyOpenedMenus(recentImagesMenu_, recentProjectsMenu_);
  window->setWindowTitle(getWindowTitle());
  window->setWindowIcon(QIcon(":cstitch.png"));
  window->addQuickHelp(autoShowQuickHelp_);
  window->showQuickHelp(false); // close any current quick help
  if (!hideWindows_) {
    window->showQuickHelp(autoShowQuickHelp_->isChecked());
  }

  window->installEventFilter(this);
}

void windowManager::addColorChooserWindow(colorChooser* window) {

  colorChooser_ =
    windowName<colorChooser*>("Color Chooser Window", window);
  QWidget* widgetPointer = window;
  colorChooserAction_->setData(QVariant::fromValue(widgetPointer));
  colorChooserAction_->setEnabled(true);
  // install event filter before show to capture the geometry change
  window->installEventFilter(this);
  window->showMaximized();
  // configure maybe displays the quickHelp window, so configure after
  // the showMaximized so that help displays above the main window
  configureNewWindow(window, CHOOSER);
}

void windowManager::addColorCompareImage(const QImage& image,
                                         const QVector<triC>& colors,
                                         flossType type,
                                         colorCompareSaver saver,
                                         int imageIndex) {

  if (imageIndex == -1) {
    imageIndex = colorCompareCount_();
    saver.setThisIndex(imageIndex);
  }
  else {
    colorCompareCount_.set(imageIndex + 1);
  }
  if (colorCompareWindow_.window() == NULL) {
    colorCompare* newColorCompare =
      new colorCompare(image, imageIndex, colors, type, this);
    colorCompareWindow_ =
      windowName<colorCompare*>(tr("Color compare"), newColorCompare);

    const colorChooser* colorChooser = colorChooser_.window();
    if (framePoint_.isNull()) {
//      framePoint_ = QPoint(colorChooser->geometry().x() - colorChooser->x(),
//                           colorChooser->geometry().y() - colorChooser->y());
//      
//      framePoint_ -= QPoint(1, 1);
      framePoint_ = QPoint(0, 0);
      frameSize_ =
        QSize(colorChooser->frameGeometry().width() - colorChooser->width(),
              colorChooser->frameGeometry().height() - colorChooser->height());
      currentGeometry_.moveTopLeft(currentGeometry_.topLeft() - framePoint_);
    }
    setNewWidgetGeometryAndRaise(newColorCompare);
    QWidget* widgetPointer = newColorCompare;
    colorCompareAction_->setData(QVariant::fromValue(widgetPointer));
    colorCompareAction_->setEnabled(true);
    configureNewWindow(newColorCompare, COMPARE);
  }
  else {
    colorCompareWindow_.window()->addImage(image, imageIndex, colors, type);
    setMenuEntry("Color compare", true);
    setNewWidgetGeometryAndRaise(colorCompareWindow_.window());
  }
  colorCompareAction_->setEnabled(true);
  colorCompareSavers_.push_back(saver);
}

void windowManager::addSquareWindow(const QImage& image, int dimension,
                                    const QVector<triC>& colors,
                                    flossType type,
                                    squareWindowSaver saver,
                                    int parentIndex,
                                    int imageIndex) {

  if (imageIndex == -1) {
    imageIndex = squareCount_();
    saver.setThisIndex(imageIndex);
  }
  else {
    squareCount_.set(imageIndex + 1);
  }
  if (squareWindow_.window() == NULL) {
    squareWindow* newSquareWindow =
      new squareWindow(image, imageIndex, dimension, colors, type, this);
    squareWindow_ = windowName<squareWindow*>("Square compare",
                                              newSquareWindow);
    setNewWidgetGeometryAndRaise(newSquareWindow);
    QWidget* widgetPointer = newSquareWindow;
    squareWindowAction_->setData(QVariant::fromValue(widgetPointer));
    squareWindowAction_->setEnabled(true);
    configureNewWindow(newSquareWindow, SQUARE);
  }
  else {
    squareWindow_.window()->addImage(image, dimension, colors, type,
                                     imageIndex);
    setMenuEntry("Square compare", true);
    setNewWidgetGeometryAndRaise(squareWindow_.window());
  }
  squareWindowAction_->setEnabled(true);
  squareWindowSavers_.push_back(saver);
  addChild(&colorCompareSavers_, parentIndex, imageIndex);
}

void windowManager::addPatternWindow(const QImage& image, int dimension,
                                     const QVector<flossColor>& colors,
                                     QRgb gridColor,
                                     patternWindowSaver saver,
                                     int parentIndex, int imageIndex) {

  if (imageIndex == -1) {
    imageIndex = patternCount_();
    saver.setThisIndex(imageIndex);
  }
  else {
    patternCount_.set(imageIndex + 1);
  }
  if (patternWindow_.window() == NULL) {
    patternWindow* newPatternWindow = new patternWindow(this);
    newPatternWindow->addImage(image, dimension, colors, gridColor,
                               imageIndex);
    setNewWidgetGeometryAndRaise(newPatternWindow);
    patternWindow_ =
      windowName<patternWindow* >("Pattern window", newPatternWindow);

    QWidget* widgetPointer = newPatternWindow;
    patternWindowAction_->setData(QVariant::fromValue(widgetPointer));
    patternWindowAction_->setEnabled(true);
    configureNewWindow(newPatternWindow, PATTERN);
  }
  else {
    patternWindow_.window()->addImage(image, dimension, colors, gridColor,
                                      imageIndex);
    setMenuEntry("Pattern Window", true);
    setNewWidgetGeometryAndRaise(patternWindow_.window());
  }
  patternWindowAction_->setEnabled(true);
  patternWindowSavers_.push_back(saver);
  addChild(&squareWindowSavers_, parentIndex, imageIndex);
}

void windowManager::save() {

  saveAs(projectFilename_);
}

void windowManager::saveAs(const QString projectFilename) {

  if (projectFilename.isNull()) {
    const QString fileString =
      QFileDialog::getSaveFileName(activeWindow(), tr("Save project"), ".",
                                   tr("Cstitch files (*.xst)\n"
                                      "All files (*.*)"));
    if (!fileString.isNull()) {
      projectFilename_ = fileString;
    }
    else {
      return;
    }
  }

  QDomDocument doc;
  QDomElement root = doc.createElement("cstitch");
  // version
  root.setAttribute("version", projectVersion_);
  doc.appendChild(root);
  ::appendTextElement(&doc, "warning",
                      tr("DIRE WARNING: DO NOT EDIT THIS FILE BY HAND! ! ! ! ! ! ! ! ! ! ! ! ! ! !"),
                      &root);
  ::appendTextElement(&doc, "warning",
                      "DIRE WARNING: DO NOT EDIT THIS FILE BY HAND! ! ! ! ! ! ! ! ! ! ! ! ! ! !",
                      &root);
  // date
  ::appendTextElement(&doc, "date", QDateTime::currentDateTime().toString(),
                      &root);
  // image name
  ::appendTextElement(&doc, "image_name", originalImageName_, &root);
  // image color count
  ::appendTextElement(&doc, "color_count",
                      ::itoqs(getOriginalImageColorCount()), &root);
  // first write settings that are independent of any particular image
  QDomElement globals(doc.createElement("global_settings"));
  // a disabled window is one that doesn't have any images other than the
  // original (and is therefore not currently being displayed)
  if (colorChooserAction_->isEnabled() && colorChooser_.window()) {
    colorChooser_.window()->appendCurrentSettings(&doc, &globals);
  }
  if (colorCompareAction_->isEnabled() && colorCompareWindow_.window()) {
    colorCompareWindow_.window()->appendCurrentSettings(&doc, &globals);
  }
  if (squareWindowAction_->isEnabled() && squareWindow_.window()) {
    squareWindow_.window()->appendCurrentSettings(&doc, &globals);
  }
  if (patternWindowAction_->isEnabled() && patternWindow_.window()) {
    patternWindow_.window()->appendCurrentSettings(&doc, &globals);
  }

  root.appendChild(globals);
  //// colorCompare
  if (!colorCompareSavers_.empty()) {
    QDomElement colorCompareElement = doc.createElement("color_compare");
    root.appendChild(colorCompareElement);
    for (int i = 0, size = colorCompareSavers_.size(); i < size; ++i) {
      const colorCompareSaver thisColorCompareSaver = colorCompareSavers_[i];
      QDomElement thisColorCompareImage(thisColorCompareSaver.toXml(&doc));
      colorCompareElement.appendChild(thisColorCompareImage);
      if (thisColorCompareSaver.hasChildren()) {
        //// squareWindow
        squareWindow* squareWindowObject = squareWindow_.window();
        QDomElement squareWindowElement = doc.createElement("square_window");
        thisColorCompareImage.appendChild(squareWindowElement);
        const QList<int> colorCompareChildren =
          thisColorCompareSaver.children();
        for (int ii = 0, iiSize = colorCompareChildren.size();
             ii < iiSize; ++ii) {
          const int thisCompareChild = colorCompareChildren[ii];
          const squareWindowSaver thisSquareWindowSaver =
            getSaverFromIndex(squareWindowSavers_, thisCompareChild);
          QDomElement thisSquareXml = thisSquareWindowSaver.toXml(&doc);
          squareWindowElement.appendChild(thisSquareXml);
          if (!thisSquareWindowSaver.hidden()) {
            // append the current history for this child
            squareWindowObject->writeCurrentHistory(&doc, &thisSquareXml,
                                                    thisCompareChild);
          }
          if (thisSquareWindowSaver.hasChildren()) {
            //// patternWindow
            patternWindow* patternWindowObject = patternWindow_.window();
            QDomElement patternWindowElement =
              doc.createElement("pattern_window");
            thisSquareXml.appendChild(patternWindowElement);
            const QList<int> squareWindowChildren =
              thisSquareWindowSaver.children();
            for (int iii = 0, iiiSize = squareWindowChildren.size();
                 iii < iiiSize; ++iii) {
              const int thisSquareChild = squareWindowChildren[iii];
              const patternWindowSaver thisPatternWindowSaver =
                getSaverFromIndex(patternWindowSavers_, thisSquareChild);
              QDomElement thisPatternXml = thisPatternWindowSaver.toXml(&doc);
              patternWindowElement.appendChild(thisPatternXml);
              // append the history for this child
              patternWindowObject->writeCurrentHistory(&doc, &thisPatternXml,
                                                       thisSquareChild);
            }
          }
        }
      }
    }
  }
  QString xmlString = doc.toString(2);

  // write the xml portion as text
  QFile outFile(projectFilename_);
  outFile.open(QIODevice::WriteOnly);
  QTextStream textStream(&outFile);
  textStream << xmlString << endl;
  textStream.flush();
  outFile.close();

  // append the image as binary
  outFile.open(QIODevice::Append);
  QDataStream dataStream(&outFile);
  dataStream << originalImageData_;
  outFile.close();
  setWindowTitles(QFileInfo(projectFilename_).fileName());
  activeWindow()->showTemporaryStatusMessage(tr("Saved project to %1")
                                             .arg(projectFilename_));
  updateRecentFiles(projectFilename_, recentProjectsMenu_);
}

void windowManager::openProject() {

  const QString fileString =
    QFileDialog::getOpenFileName(activeWindow(), tr("Open project"), ".",
                                 tr("Cstitch files (*.xst)"
                                    "\nAll files (*.*)"));

  if (!fileString.isEmpty()) {
    const bool success = openProject(fileString);
    if (success) {
      updateRecentFiles(fileString, recentProjectsMenu_);
    }
  }
}

bool windowManager::openProject(const QString& projectFile) {

  QFile inFile(projectFile);
  inFile.open(QIODevice::ReadOnly);
  // the save file starts with xml text, so read that first
  QTextStream textInStream(&inFile);
  QString inString;
  inString = textInStream.readLine() + "\n";
  // (A day after the initial release I changed the program name from
  // stitch to cstitch, so check for either...)
  QRegExp rx("^<(cstitch|stitch) version=");
  rx.indexIn(inString);
  const QString programName = rx.cap(1);
  if (programName != "cstitch" && programName != "stitch") { // uh oh
    QMessageBox::critical(NULL, tr("Bad project file"),
                          tr("Sorry, %1 is not a valid project file "
                             "(diagnostic: wrong first line)")
                          .arg(projectFile));
    return false;
  }

  int lineCount = 0;
  const int maxLineCount = 100000;
  const QString endTag = "</" + programName + ">";
  QString thisString = "";
  do {
    thisString = textInStream.readLine();
    inString += thisString + "\n";
    ++lineCount;
    if (lineCount > maxLineCount) {
      QMessageBox::critical(NULL, tr("Bad project file"),
                            tr("Sorry, %1 appears to be corrupted "
                               "(diagnostic: bad separator)")
                            .arg(projectFile));
      return false;
    }
  } while (thisString != endTag);

  QDomDocument doc;
  const bool xmlLoadSuccess = doc.setContent(inString);
  if (!xmlLoadSuccess) {
    QMessageBox::critical(NULL, tr("Bad project file"),
                          tr("Sorry, %1 appears to be corrupted "
                             "(diagnostic: parse failed)")
                          .arg(projectFile));
    return false;
  }
  // a blank line between the xml and the image
  inString += textInStream.readLine() + "\n";

  // hide everything except progress meters while we regenerate this project
  hideWindows_ = true;
  hideWindows();

  // how many total images are we restoring?
  int imageCount = 1; // one colorChooser image
  imageCount += doc.elementsByTagName("color_compare_image").size();
  imageCount += doc.elementsByTagName("square_window_image").size();
  imageCount += doc.elementsByTagName("pattern_window_image").size();
  groupProgressDialog progressMeter(imageCount);
  progressMeter.setMinimumDuration(2000);
  progressMeter.setWindowModality(Qt::WindowModal);
  progressMeter.show();
  progressMeter.bumpCount();
  altMeter::setGroupMeter(&progressMeter);

  // read the image file name
  const QString fileName = ::getElementText(doc, "image_name");
  // read the project version number
  setProjectVersion(::getElementAttribute(doc, "cstitch", "version"));
  // now read the binary data image
  inFile.seek(inString.length());
  QDataStream imageStream(&inFile);
  QImage newImage;
  imageStream >> newImage;
  inFile.seek(inString.length());
  QDataStream imageData(&inFile);
  QByteArray imageByteArray;
  imageData >> imageByteArray;
  reset(newImage, imageByteArray, fileName);

  //// colorChooser
  colorChooser_.window()->setNewImage(newImage);
  progressMeter.bumpCount();

  //// colorCompare
  QDomElement colorCompareElement =
    doc.elementsByTagName("color_compare").item(0).toElement();
  if (!colorCompareElement.isNull()) {
    colorChooser* colorChooserObject = colorChooser_.window();
    QDomNodeList compareImagesList(colorCompareElement.
                                   elementsByTagName("color_compare_image"));
    for (int i = 0, size = compareImagesList.size(); i < size; ++i) {
      QDomElement thisCompareElement(compareImagesList.item(i).toElement());
      const int hiddenColorCompareIndex = colorChooserObject->
        recreateImage(colorCompareSaver(thisCompareElement));
      progressMeter.bumpCount();
      //// squareWindow
      colorCompare* colorCompareObject = colorCompareWindow_.window();
      QDomNodeList squareImagesList(thisCompareElement.
                                    elementsByTagName("square_window_image"));
      if (!squareImagesList.isEmpty()) {
        for (int ii = 0, iiSize = squareImagesList.size(); ii < iiSize; ++ii) {
          QDomElement thisSquareElement(squareImagesList.item(ii).toElement());
          const int hiddenSquareImageIndex = colorCompareObject->
            recreateImage(squareWindowSaver(thisSquareElement));
          progressMeter.bumpCount();
          squareWindow* squareWindowObject = squareWindow_.window();
          //// patternWindow
          // restore this square image's children before restoring its history
          // (the children may have their own different histories)
          QDomNodeList
            patternImageList(thisSquareElement.
                             elementsByTagName("pattern_window_image"));
          if (!patternImageList.isEmpty()) {
            for (int iii = 0, iiiSize = patternImageList.size();
                 iii < iiiSize; ++iii) {
              QDomElement thisPatternElement(patternImageList.item(iii).
                                             toElement());
              squareWindowObject->
                recreatePatternImage(patternWindowSaver(thisPatternElement));
              patternWindow_.window()->updateHistory(thisPatternElement);
              progressMeter.bumpCount();
            }
          }
          squareWindowObject->updateImageHistory(thisSquareElement);
          // if this image was only created so that one of its children could be
          // recreated, remove it (its data has already been stored away)
          if (hiddenSquareImageIndex != -1) {
            squareWindowObject->removeImage(hiddenSquareImageIndex);
          }
        }
      }
      // if this image was only created so that one of its children could be
      // recreated, remove it (its data has already been stored away)
      if (hiddenColorCompareIndex != -1) {
        colorCompareObject->removeImage(hiddenColorCompareIndex);
      }
    }
  }

  //// restore window wide settings that are independent of a particular image
  QDomElement windowGlobals(doc.elementsByTagName("global_settings").
                            item(0).toElement());
  if (colorChooserAction_->isEnabled() && colorChooser_.window()) {
    colorChooser_.window()->updateCurrentSettings(windowGlobals);
  }
  if (colorCompareAction_->isEnabled() && colorCompareWindow_.window()) {
    colorCompareWindow_.window()->updateCurrentSettings(windowGlobals);
  }
  if (squareWindowAction_->isEnabled() && squareWindow_.window()) {
    squareWindow_.window()->updateCurrentSettings(windowGlobals);
    squareWindow_.window()->checkAllColorLists();
  }
  if (patternWindowAction_->isEnabled() && patternWindow_.window()) {
    patternWindow_.window()->updateCurrentSettings(windowGlobals);
  }
  // read the image number of colors (before the reset)
  const int colorCount = ::getElementText(doc, "color_count").toInt();
  originalImageColorCount_ = colorCount;
  // call while hideWindows_ (if colorCount wasn't 0 it won't be recalculated)
  startOriginalImageColorCount();
  hideWindows_ = false;
  altMeter::setGroupMeter(NULL);
  projectFilename_ = projectFile;
  setWindowTitles(QFileInfo(projectFilename_).fileName());
  setNewWidgetGeometryAndRaise(colorChooser_.window());
  return true;
}

void windowManager::reset(const QImage& image, const QByteArray& byteArray,
                          const QString& imageName) {

  projectFilename_ = QString();
  originalImage_ = image;
  originalImageData_ = byteArray;
  originalImageName_ = imageName;
  originalImageColorCount_ = 0;

  colorCompareCount_.reset();
  squareCount_.reset();
  patternCount_.reset();
  if (colorCompareWindow_.window()) {
    colorCompareAction_->setEnabled(false);
    colorCompareWindow_.clear();
  }
  if (squareWindow_.window()) {
    squareWindowAction_->setEnabled(false);
    squareWindow_.clear();
  }
  if (patternWindow_.window()) {
    patternWindowAction_->setEnabled(false);
    patternWindow_.clear();
  }
  colorCompareSavers_.clear();
  squareWindowSavers_.clear();
  patternWindowSavers_.clear();
}

void windowManager::startOriginalImageColorCount() {

  // start the computation of the number of colors in this image in a
  // separate thread
  // (it's possible an old thread is still running - you can't cancel
  // these threads or destroy the "old" future object (that gives a
  // segfault when its thread finishes), but the future objects are
  // ref counted, so they should gracefully destroy themselves after
  // their process finishes and their ref count is 0 - right?)
  if (originalImageColorCount_ == 0 || !hideWindows_) {
    colorCountComputation_ =
      QtConcurrent::run(::numberOfColors, originalImage_);
  }
  else {
    colorCountComputation_ = QFuture<int>();
  }
}

void windowManager::hideWindows() {

  const QList<imageZoomWindow*> widgets = constructedWidgets();
  for (int i = 0, size = widgets.size(); i < size; ++i) {
    widgets[i]->hide();
  }
}

void windowManager::setWindowTitles(const QString& title) {

  const QList<imageZoomWindow*> widgets = constructedWidgets();
  for (int i = 0, size = widgets.size(); i < size; ++i) {
    widgets[i]->setWindowTitle(title);
  }
}

void windowManager::displayActionWindow(QAction* action) {

  QWidget* actionWindow = action->data().value<QWidget*>();
  imageZoomWindow* curWindow = activeWindow();
  if (actionWindow == curWindow) { // we're already there
    return;
  }
  curWindow->showQuickHelp(false);
  setNewWidgetGeometryAndRaise(actionWindow);
}

void windowManager::colorCompareEmpty() {

  colorCompareAction_->setEnabled(false);
  showAndRaise(colorChooser_.window());
  setMenuEntry(colorCompareWindow_.name(), false);
}

void windowManager::squareWindowEmpty() {

  squareWindowAction_->setEnabled(false);
  if (windowIsActive(colorCompareWindow_.name())) {
    showAndRaise(colorCompareWindow_.window());
  }
  else {
    showAndRaise(colorChooser_.window());
  }
  setMenuEntry(squareWindow_.name(), false);
}

void windowManager::patternWindowEmpty() {

  patternWindowAction_->setEnabled(false);
  if (windowIsActive(squareWindow_.name())) {
    showAndRaise(squareWindow_.window());
  }
  else if (windowIsActive(colorCompareWindow_.name())) {
    showAndRaise(colorCompareWindow_.window());
  }
  else {
    showAndRaise(colorChooser_.window());
  }
  setMenuEntry(patternWindow_.name(), false);
}

void windowManager::setMenuEntry(const QString& entryName, bool enable) {

  QList<QAction*> actionList = windowMenu_->actions();
  for (QList<QAction*>::iterator it = actionList.begin(),
          end = actionList.end(); it != end; ++it) {
    if ((*it)->text() == entryName) {
      (*it)->setEnabled(enable);
      break;
    }
  }
}

bool windowManager::windowIsActive(const QString& windowName) {

  QList<QAction*> actionList = windowMenu_->actions();
  for (QList<QAction*>::iterator it = actionList.begin(),
          end = actionList.end(); it != end; ++it) {
    if ((*it)->text() == windowName) {
      return (*it)->isEnabled();
    }
  }
  return false;
}

imageZoomWindow* windowManager::activeWindow() const {

  const QList<imageZoomWindow*> widgets = constructedWidgets();
  for (int i = 0, size = widgets.size(); i < size; ++i) {
    if (widgets[i]->isVisible()) {
      return widgets[i];
    }
  }
  // oops, couldn't find an active window
  qWarning() << "No active window at windowManager::activeWindow()";
  showAndRaise(colorChooser_.window());
  return colorChooser_.window();
}

// for use in constructors that can't figure their own frame dimensions
void windowManager::frameWidthAndHeight(int* w, int* h) const {

  if (const colorChooser* colorChooser =
      colorChooser_.window()) {
    *w = colorChooser->frameGeometry().width() - colorChooser->width();
    *h = colorChooser->frameGeometry().height() - colorChooser->height();
  }
  else {
    *w = 0;
    *h = 0;
  }
}

bool windowManager::openNewImage(const QString& imageFile) {

  const QImage newImage(imageFile);
  if (newImage.isNull()) {
    const QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    QString supportedTypes;
    for (QList<QByteArray>::const_iterator it = formats.begin(),
           end = formats.end(); it != end; ++it) {
      supportedTypes += (*it).data() + QString(", ");
    }
    supportedTypes.chop(2);
    const QString errorString = "Unable to load " + imageFile +
      ";\nplease make sure " +
      "your image is one of the supported types:\n" + supportedTypes;
    QMessageBox::warning(NULL, "Image load failed", errorString);
    return false;
  }
  QFile fileDevice(imageFile);
  fileDevice.open(QIODevice::ReadOnly);
  QByteArray byteArray(fileDevice.readAll());
  const QString imageName = QFileInfo(imageFile).fileName();
  
  reset(newImage, byteArray, imageName);
  setProjectVersion(programVersion_);
  ::showAndRaise(colorChooser_.window());
  colorChooser_.window()->setNewImage(newImage);
  colorChooser_.window()->setWindowTitle(getWindowTitle());
  startOriginalImageColorCount();
  return true;
}

void windowManager::openNewImage() {

  const bool warnToSave = !originalImage_.isNull();
  const QString file = ::getNewImageFileName(activeWindow(), warnToSave);
  if (!file.isEmpty()) {
    const bool success = openNewImage(file);
    if (success) {
      updateRecentFiles(file, recentImagesMenu_);
    }
  }
}

void windowManager::setNewSize(const QSize& size) {

  if (hideWindows_) {
    return;
  }
  currentGeometry_.setSize(size);
}

void windowManager::setNewPosition(const QPoint& position) {

  if (hideWindows_) {
    return;
  }
  currentGeometry_.moveTopLeft(position - framePoint_);
}

void windowManager::setNewWidgetGeometryAndRaise(QWidget* widget) {

  if (hideWindows_) {
    return;
  }
  hideWindows();
  widget->move(currentGeometry_.topLeft());
  widget->resize(currentGeometry_.size());
  ::showAndRaise(widget);
}

bool windowManager::eventFilter(QObject* target, QEvent* event) {

  //    qDebug() << "e" << event->type();
  if (event->type() == QEvent::Resize) {
    QResizeEvent* resize = static_cast<QResizeEvent*>(event);
    setNewSize(resize->size());
    return true;
  }

  if (event->type() == QEvent::Move) {
    setNewPosition(static_cast<QWidget*>(target)->pos());
    return true;
  }
  return QObject::eventFilter(target, event);
}

template<class T> parentChildren
windowManager::deleteIndexedImageFromList(QList<T>* list, int index) {

  for (int i = 0, size = list->size(); i < size; ++i) {
    T& thisItem = (*list)[i]; // a modifiable reference
    if (thisItem.index() == index) {
      if (thisItem.hasChildren()) {
        thisItem.setHidden(true);
        return parentChildren(-1, -1);
      }
      else {
        const int parent = thisItem.parent();
        list->removeAt(i);
        return parentChildren(index, parent);
      }
    }
  }
  return parentChildren(-1, -1);
}

template<class T> parentChildren
windowManager::childDeletedFromList(QList<T>* list, int thisIndex,
                                    int childIndex) {

  for (int i = 0, size = list->size(); i < size; ++i) {
    T& thisItem = (*list)[i]; // a modifiable reference
    if (thisItem.index() == thisIndex) {
      thisItem.removeChild(childIndex);
      if (!thisItem.hasChildren() && thisItem.hidden()) {
        list->removeAt(i);
        const int parent = thisItem.parent();
        return parentChildren(thisIndex, parent);
      }
      else {
        return parentChildren(-1, -1);
      }
    }
  }
  return parentChildren(-1, -1);
}

template<class T> T
windowManager::getSaverFromIndex(const QList<T>& list, int index) {

  for (int i = 0, size = list.size(); i < size; ++i) {
    if (list[i].index() == index) {
      return list[i];
    }
  }
  return T();
}

template<class T> void windowManager::addChild(QList<T>* list,
                                               int thisIndex,
                                               int childIndex) {

  for (int i = 0, size = list->size(); i < size; ++i) {
    T& thisItem = (*list)[i]; // a modifiable reference
    if (thisItem.index() == thisIndex) {
      thisItem.addChild(childIndex);
      return;
    }
  }
}

void windowManager::colorCompareImageDeleted(int imageIndex) {

  deleteIndexedImageFromList(&colorCompareSavers_, imageIndex);
}

void windowManager::squareWindowImageDeleted(int imageIndex) {

  const parentChildren propogate =
    deleteIndexedImageFromList(&squareWindowSavers_, imageIndex);
  if (propogate.parentIndex() != -1) {
    childDeletedFromList(&colorCompareSavers_, propogate.parentIndex(),
                         propogate.thisIndex());
  }
}

void windowManager::patternWindowImageDeleted(int imageIndex) {

  const parentChildren propogate =
    deleteIndexedImageFromList(&patternWindowSavers_, imageIndex);
  if (propogate.parentIndex() != -1) {
    const parentChildren propogateAgain =
      childDeletedFromList(&squareWindowSavers_, propogate.parentIndex(),
                           propogate.thisIndex());
    if (propogateAgain.parentIndex() != -1) {
      childDeletedFromList(&colorCompareSavers_, propogate.parentIndex(),
                           propogate.thisIndex());
    }
  }
}

void windowManager::autoShowQuickHelp(bool show) {

  QSettings settings("cstitch", "cstitch");
  settings.setValue("auto_show_quick_help", show);
  activeWindow()->showQuickHelp(show);
}

QList<imageZoomWindow*> windowManager::constructedWidgets() const {

  QList<imageZoomWindow*> returnList;
  if (colorChooser_.window()) {
    returnList.push_back(colorChooser_.window());
  }
  if (colorCompareWindow_.window()) {
    returnList.push_back(colorCompareWindow_.window());
  }
  if (squareWindow_.window()) {
    returnList.push_back(squareWindow_.window());
  }
  if (patternWindow_.window()) {
    returnList.push_back(patternWindow_.window());
  }
  return returnList;
}

int windowManager::getOriginalImageColorCount() {

  // if this is a restore and colorCount_ is 0, too bad, we're not going to
  // count now [especially since that should never happen!]
  if (originalImageColorCount_ == 0 && !hideWindows_) {
    // [isCanceled() is the unofficial way to tell if a QFuture is empty
    // this should never happen ...]
    if (colorCountComputation_.isCanceled()) {
      colorCountComputation_ = QtConcurrent::run(::numberOfColors,
                                                 originalImage_);
    }
    // this call blocks until the thread finishes
    originalImageColorCount_ = colorCountComputation_.result();
  }
  return originalImageColorCount_;
}

QString windowManager::getWindowTitle() const {

  if (!projectFilename_.isNull()) {
    return QFileInfo(projectFilename_).fileName();
  }
  else {
    return originalImageName_.isNull() ? "Cstitch" :
      originalImageName_;
  }
}

void windowManager::setProjectVersion(const QString& projectVersion) {

  projectVersion_ = projectVersion;
  versionProcessor::setProcessor(projectVersion_);
  colorMatcher::initializeIntensitySpreads();
}

void windowManager::updateRecentFiles(const QString& file,
                                      fileListMenu* menu) {

  menu->prependFile(file);
  menu->setEnabled(true);
}

void windowManager::quit() {

  QSettings settings("cstitch", "cstitch");
  settings.setValue("recent_images", recentImagesMenu_->files());
  settings.setValue("recent_projects", recentProjectsMenu_->files());
}

void windowManager::openRecentImage(const QString& imageFile) {

  const QFile fileCheck(imageFile);
  if (fileCheck.exists()) {
    openNewImage(imageFile);
    updateRecentFiles(imageFile, recentImagesMenu_);
  }
  else { // remove <imageFile> from the recent images list
    openRecentFailure(recentImagesMenu_, imageFile);
  }
}

void windowManager::openRecentProject(const QString& projectFile) {

  const QFile fileCheck(projectFile);
  if (fileCheck.exists()) {
    openProject(projectFile);
    updateRecentFiles(projectFile, recentProjectsMenu_);
  }
  else { // remove <projectFile> from the recent images list
    openRecentFailure(recentProjectsMenu_, projectFile);
  }
}

void windowManager::openRecentFailure(fileListMenu* menu,
                                      const QString& file) {

  menu->remove(file);
  if (menu->isEmpty()) {
    menu->setEnabled(false);
  }
  const QString errorString = tr("The file %1 no longer exists.").arg(file);
  QMessageBox::warning(NULL, tr("Load failed"), errorString);
}
