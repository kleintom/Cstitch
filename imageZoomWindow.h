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

#ifndef IMAGEZOOMWINDOW_H
#define IMAGEZOOMWINDOW_H

#include <QtWidgets/QMainWindow>

class windowManager;
class helpMode;
class QDomDocument;
class QDomElement;
class QString;
class QLabel;
class QMenu;
class flossType;

extern const int ZOOM_INCREMENT;

// class imageZoomWindow
//
// imageZoomWindow serves as the core base of common functionality for the
// four main windows (colorChooser, colorCompare, squareWindow, and
// patternWindow).  It provides interfaces and sometimes functionality
// for the following: zooming, common menus, toolbar controls, list dock,
// common actions used by all windows, the status bar, the window
// manager object used by each window, and the help browser.
//
class imageZoomWindow : public QMainWindow {

  Q_OBJECT

 public:
  // dockName is the title for the dock list widget for this object
  imageZoomWindow(const QString& dockName, windowManager* winMgr);
  // window manager manages all of the "change window" actions; we just
  // add them and forget them (promise)
  void addWindowActions(const QList<QAction*>& windowActions,
                        QMenu* windowMenu);
  // windowManager manages the action to turn auto show of quick help
  // on/off (common to all windows), we just put it on our help menu and
  // forget about it (promise)
  void addQuickHelp(QAction* autoShowQuickHelp);
  // windowManager manages these menus, we just display them (promise)
  void addRecentlyOpenedMenus(QMenu* imagesMenu, QMenu* projectsMenu);
  // show <status> in the normal (left) part of the status bar for
  // <duration> milliseconds
  void showTemporaryStatusMessage(const QString& status,
                                  int duration = 3000);
  // append "global" xml settings to <appendee> for this widget
  virtual void appendCurrentSettings(QDomDocument* doc,
                                     QDomElement* appendee) const = 0;
  // restore "global" settings for this widget from <xml>; return an error
  // message if something goes wrong
  virtual QString updateCurrentSettings(const QDomElement& xml) = 0;

 public slots:
  // provide the user some quick (brief) help for this widget, or close
  // the quick help window
  void showQuickHelp(bool show = true);

 protected:
  void showListDock();
  void hideListDock();
  bool listDockIsVisible() const;
  void setListDockEnabled(bool b);
  // make <widget> the dock's widget and add the dock to the window,
  // making it visible and enabled
  void setListDockWidget(QWidget* widget);
  void setListDockTitle(const QString& title);
  // append
  void addToolbarAction(QAction* action);
  // append
  void addToolbarWidget(QWidget* widget);
  // append
  void addToolbarSeparator();
  void setToolbarEnabled(bool b);
  void setZoomActionsEnabled(bool b);
  void addZoomActionsToImageMenu();
  QMenu* fileMenu() { return fileMenu_; }
  QMenu* imageMenu() { return imageMenu_; }
  QMenu* helpMenu() { return helpMenu_; }
  QAction* zoomInAction() { return zoomInAction_; }
  QAction* zoomOutAction() { return zoomOutAction_; }
  QAction* originalSizeAction() { return originalSizeAction_; }
  QAction* imageInfoAction() { return imageInfoAction_; }
  QAction* quitAction() { return quitAction_; }
  void setSaveActionsEnabled(bool b);
  windowManager* winManager() { return windowManager_; }
  const windowManager* winManager() const { return windowManager_; }
  // return the name used for the original image in all windows
  const QString& Original() const { return Original_; }
  const QImage& originalImage() const;
  // pop up a dialog showing useful information about the original image
  void displayOriginalImageInfo(int width, int height);
  // call to append the zoom in, zoom out, and original size buttons to
  // the toolbar
  void addToolbarZoomIcons();
  // set the text of the normal status message (the left part of the bar)
  void setStatus(const QString& status);
  // set the text of the permanent status message (the right part of the
  // bar)
  void setPermanentStatus(const QString& status);
  void setPermanentStatusEnabled(bool b);
  // convert the given <imageName> to its index
  // "Image n" returns n, "Original image" returns 0, else returns -1
  int imageNameToIndex(const QString& imageName) const;
  // returns Original_ if <index> == 0, else returns "Image <index>"
  QString imageNameFromIndex(int index) const;
  virtual void showEvent(QShowEvent* event);
  virtual void closeEvent(QCloseEvent* event);
  virtual bool eventFilter(QObject* watched, QEvent* event);
  // [must keep track of <watched> for derived classes with more than
  // one scroll windows]
  virtual bool horizontalWheelScrollEvent(QObject* watched,
                                          QWheelEvent* event) const= 0;
  // the widget is being shown for the first time; this is used for
  // processing that requires geometry knowledge (which is unavailable
  // until the widget is literally visible on the screen)
  virtual void processFirstShow();
  // Return a text string for image info display on colors of type <type>.
  QString imageInfoFlossString(flossType type) const;

 protected slots:
  // reset image(s) to original size
  virtual void originalSize() = 0;
  // make an image "ZOOM_INCREMENT pixels larger"
  void zoomIn() { zoom(ZOOM_INCREMENT); }
  // make an image "ZOOM_INCREMENT pixels smaller"
  void zoomOut() { zoom(-ZOOM_INCREMENT); }
  // zoom something (positive to zoom in, negative to zoom out)
  virtual void zoom(int zoomIncrement) = 0;
  virtual void zoomToWidth() = 0;
  virtual void zoomToHeight() = 0;
  virtual void zoomToImage() = 0;
  // display useful information about an image
  virtual void displayImageInfo() = 0;
  // quit the application
  bool quit();
  // provide the user some help information for this widget
  void helpRequested() const;

 private slots:
  // display "about" information for this application
  void helpAbout();
  // open a new image
  void open();

 private:
  // create the list dock widget, using the given <dockName> as title
  // this doesn't cause the dock to become visible (see setDockWidget()
  // for that)
  void createDock(const QString& dockName);
  // create and add the file, image, dock, and help menus to the menu bar,
  // in that order
  void createMenus();
  // createMenus() MUST be called first
  // create the toolbar and populate the toolbar and menus with the common
  // actions shared by all windows
  void createToolbarAndMenuActions();
  // create "normal" and "permanent" status bars and add them to the widget
  void createStatusBar();
  // return the helpMode enum const for this widget
  virtual helpMode getHelpMode() const = 0;

 private:
  // the string used for the original image in all windows
  const QString Original_;

  QToolBar* toolBar_;
  // the widget that holds the dock list widget
  QDockWidget* dockHolder_;
  //// toolbar/menubar actions
  // open a new image for processing
  QAction* openImageAction_;
  // save the project as
  QAction* saveAsAction_;
  // save the project
  QAction* saveAction_;
  // open a saved project
  QAction* openProjectAction_;
  // quit the application
  QAction* quitAction_;
  QAction* zoomInAction_;
  QAction* zoomOutAction_;
  QAction* zoomToWidthAction_;
  QAction* zoomToHeightAction_;
  QAction* zoomToImageAction_;
  // set image(s) to original size
  QAction* originalSizeAction_;
  // pop up some useful information about an image
  QAction* imageInfoAction_;
  // pop up some quick (brief) help on the current stage
  QAction* quickHelpAction_;
  // pop up some help information on the application
  QAction* helpAction_;
  // pop up about information for the application
  QAction* helpAboutAction_;

  // standard menus
  QMenu* fileMenu_;
  QMenu* imageMenu_;
  QMenu* helpMenu_;

  // status bar labels
  // status_ is what QStatusBar refers to as "normal", permanentStatus_ is
  // "permanent" - normal appears to the left or permanent, and normal
  // can be temporarily overwritten by a temporary status message while
  // permanent is never overwritten (unless it's changed!)
  QLabel* status_;
  QLabel* permanentStatus_;

  // [yuk] Most of our widgets need to make some constructor decisions
  // based on the size of the widget, but the widget size isn't known
  // until the widget is literally visible (it's not enough to call show!),
  // so we set this to true on the first show event
  bool constructedAndShown_;

  //// winManager keeps track of the windows menu
  //// and shares it amongst all windows, amongst other things
  windowManager* windowManager_;
};

#endif
