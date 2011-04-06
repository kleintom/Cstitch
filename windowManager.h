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

#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QtCore/QString>
#include <QtCore/QFuture>

#include <QtGui/QWidget>
#include <QtGui/QAction>

#include "windowSavers.h"

class triC;
class colorChooser;
class squareWindow;
class patternWindow;
class colorCompare;
class imageZoomWindow;

// a simple class for keeping track of a count that starts at 1 and
// increments on each call of ()
class countIterator {
 public:
  countIterator() { count_ = 1; }
  void reset() { count_ = 1; }
  void set(int newCount) {count_ = newCount; }
  int last() const { return count_ - 1; }
  int operator()() { return count_++; }
 private:
  int count_;
};

// functor for finding an action with data a pointer to a QWidget
class actionWidgetEqual {
 public:
  explicit actionWidgetEqual(QWidget* widget) : widget_(widget) {}
  bool operator()(QAction* action) const {
    return action->data().value<QWidget*>() == widget_;
  }
 private:
  QWidget* widget_;
};

template<class T> class windowName;
// windowName<T*> holds a pointer to T and a name associated with that
// pointer
// T* is assumed to be dynamic and windowName takes ownership
// T MUST be derived from QObject (for deleteLater(), which is the
// "right" way to delete a widget while it's in use)
template<class T> class windowName<T*> { // specialize to pointers
 public:
  windowName(const QString& name, T* window)
    : name_(name), window_(window) {}
  explicit windowName(T* window) : window_(window) {}
  windowName() : window_(NULL) {}
  QString name() const { return name_; }
  T* window() const { return window_; }
  void clear() { window_->deleteLater(); window_ = NULL; name_ = ""; }
  bool operator==(const windowName& window) const { // compare pointers
    return window_ == window.window_;
  }
 private:
  QString name_;
  T* window_;
};

// functor for comparing a windowName<T*> with a string - returns true
// if the other window's name _starts with_ name_
template<class T> class windowNameStartsWithName {
 public:
  explicit windowNameStartsWithName(const QString& name) : name_(name) {}
  bool operator()(const windowName<T*>& otherWindowName) const {
    return otherWindowName.name().startsWith(name_);
  }
 private:
  QString name_;
};

// functor for comparing a windowName<T*> with a T*
template<class T> class windowNameEqualsPointer {
 public:
  explicit windowNameEqualsPointer(T* window) : window_(window) {}
  bool operator()(const windowName<T*>& otherWindowName) const {
    return otherWindowName.window() == window_;
  }
 private:
  T* window_;
};

//
// windowManager is used by the four main windows (colorChooser,
// colorCompare, squareWindow, and patternWindow) to keep track of which
// of the windows is active.  The colorChooser object must be created
// elsewhere and passed in (with addColorChooserWindow), but windowManager
// will create the others as needed, and has destruction responsibilities
// for all four.  It is also responsible for keeping the common
// window menu list (windowMenu()) displayed by each of the four main
// windows updated. The window menu list lists the four main windows: those
// that are active are displayed when selected, those that are inactive are
// grayed out and cannot be selected.
//
// Only one of the four main windows is ever displayed at any given time,
// but they all share the same geometry (windowManager uses event filters
// on the main windows to track geometry changes).
//
// originalImage() returns a const reference to the the image last opened
// by the user (only one image can be active at a time - if the user
// opens a new image, all work on the previous image is deleted), so that
// windowManager is the only place the actual image is stored.
//
// frameWidthAndHeight() is an unfortunate hack necessary because in some
// environments (all?) windows can't know their frame geometry until after
// they've been fully painted (which of course is too late if we want the
// window to have a certain geometry including its frame!).
//
class windowManager : public QObject {

  Q_OBJECT

 public:
  windowManager();
  // add a new colorChooser
  void addColorChooserWindow(colorChooser* window);
  // add <image> with <colors> (all <dmc> or no) and image number <index>
  // (incremented from the highest past value if -1) to the colorCompare
  // object (creating colorCompare first if necessary)
  void addColorCompareImage(const QImage& image,
                            const QVector<triC>& colors, bool dmc,
                            colorCompareSaver saver, int imageIndex = -1);
  // the color compare image with index <imageIndex> was deleted
  void colorCompareImageDeleted(int imageIndex);
  // add <image> with <colors> (all <dmc> or no) and square dimension
  // <dimension> to the squareWindow object (creating squareWindow first if
  // necessary)
  void addSquareWindow(const QImage& image, int dimension,
                       const QVector<triC>& colors, bool dmc,
                       squareWindowSaver saver,
                       int parentIndex, int imageIndex = -1);
  // the square compare image with index <imageIndex> was deleted
  void squareWindowImageDeleted(int imageIndex);
  // add <image> with <colors>, square dimension <dimension>, <gridColor>,
  // and history <xmlHistory> to the patternWindow object (creating
  // patternWindow first if necessary)
  void addPatternWindow(const QImage& image, int dimension,
                        const QVector<triC>& colors, QRgb gridColor,
                        patternWindowSaver saver,
                        int parentIndex, int imageIndex = -1);
  // the pattern image with index <imageIndex> was deleted
  void patternWindowImageDeleted(int imageIndex);
  // should be called when only the original image is left in
  // colorCompare (the others having been deleted) - colorChooser is
  // displayed instead and the colorCompare window menu entry is
  // deactivated
  void colorCompareEmpty();
  // should be called when only the original image is left in
  // squareWindow (the others having been deleted) - colorCompare is
  // displayed instead if it's not empty, otherwise colorChooser is
  // displayed; the squareWindow window menu entry is deactivated
  void squareWindowEmpty();
  // should be called when all images in patternWindow have been deleted -
  // the first of squareWindow, colorCompare, colorChooser that's non-empty
  // is displayed instead, and the squareWindow window menu entry is
  // deactivated
  void patternWindowEmpty();
  // ask the user for a new image file - if one's given, reset and then
  // set it as the new image in colorChooser
  void openNewImage();
  // a new <image> with name <imageName> has been opened, so reset/delete
  // all old data to prepare for new data
  void reset(const QImage& image, const QByteArray& byteArray,
             const QString& imageName);
  // there is no non-const access to the original image
  const QImage& originalImage() const { return originalImage_; }
  int getOriginalImageColorCount();
  // sets *w and *h to the width and height of the frame of windows in the
  // current environment (or 0s if the colorChooser object doesn't exist
  // yet)
  void frameWidthAndHeight(int* w, int* h) const;
  // set the application version
  void setVersion(const QString& version) { version_ = version; }
  QString getVersion() const { return version_; }

 public slots:
  // save all current data to file
  void save();
  void saveAs(const QString projectFilename = QString());
  // reload a saved project from file specified by user
  void openProject();

 private:
  enum windowStage {CHOOSER, COMPARE, SQUARE, PATTERN};
  // set the common menu list entry for <imageName> to <enabled>
  void setMenuEntry(const QString& imageName, bool enabled);
  // set the geometry of widget to the saved geometry (currentGeometry_)
  // and show the window
  void setNewWidgetGeometryAndRaise(QWidget* widget);
  // return true if the window for <windowName> from the common window
  // menu is active
  bool windowIsActive(const QString& windowName);
  // return a pointer to the main window currently active
  // (or colorChooser if none is active)
  imageZoomWindow* activeWindow() const;
  // track geometry changes on the main windows
  bool eventFilter(QObject* target, QEvent* event);
  // update the common <size> used by all main windows
  void setNewSize(const QSize& size);
  // update the common <position> used by all main windows
  void setNewPosition(const QPoint& position);
  // remove the image with index <index> from the <list> of saved images
  // of that type
  // returns an invalid parentChildren if the removed image has children,
  // else returns the parentChildren info for the removed image
  template<class T> parentChildren
    deleteIndexedImageFromList(QList<T>* list, int index);
  // remove the child with index <childIndex> from the image with index
  // <thisIndex> on the <list>
  // return an invalid parentChildren if thisIndex still has children
  // or isn't hidden (in other words, if it shouldn't be deleted yet),
  // else return the parentChildren info for thisIndex
  template<class T> parentChildren
    childDeletedFromList(QList<T>* list, int thisIndex, int childIndex);
  // return the saver object for the image with index <index> from <list>
  template<class T> T getSaverFromIndex(const QList<T>& list, int index);
  // add the child with index <childIndex> to the item on <list> with
  // index <thisIndex>
  template<class T> void addChild(QList<T>* list, int thisIndex,
                                  int childIndex);
  // hide any currently visible main window
  void hideWindows();
  // return the string that should be used for a main window titlebar title
  QString getWindowTitle() const;
  // set the titlebar title of any constructed main window to <title>
  void setWindowTitles(const QString& title);
  // return the list of main windows that have been constructed
  // list is ordered the same as the main processing stages
  QList<imageZoomWindow*> constructedWidgets() const;
  // common setup code for a new <stage> main window
  void configureNewWindow(imageZoomWindow* window, windowStage stage);

 private slots:
  void autoShowQuickHelp(bool show);
  // hide the current main window and display the main window contained
  // in <action>'s data
  void displayActionWindow(QAction* action);
  // start the computation of the number of colors in the original image
  // in a separate thread if the current count is 0 or we're not 
  // currently in a project restore, else just clear the current
  // computation
  void startOriginalImageColorCount();

 private:
  QByteArray originalImageData_; // the user's original image (as raw data)
  QImage originalImage_; // the user's original image (as a QImage)
  // the filename of the original image (excluding the path)
  QString originalImageName_;
  // WARNING never read this value directly - colorCountComputation_
  // may be in the process of computing it; use getOriginalImageColorCount
  // instead
  int originalImageColorCount_; // # of colors in the original image
  // the result of a "future" computation in a separate thread
  QFuture<int> colorCountComputation_;
  QString projectFilename_;

  // all main windows share the same geometry
  QRect currentGeometry_;
  // a relative point: if the top left corner of the frame of a widget is
  // (0,0), then framePoint_ is the location of the top left corner of the
  // widget without its frame
  QPoint framePoint_;
  // widgets don't know their frame size until they're literally visible
  // on the screen, so sometimes we need to help them out
  QSize frameSize_; // app wide

  windowName<colorChooser*> colorChooser_;
  // colorChooserAction is displayed in the common window menu and on
  // the toolbar list of non-colorChooser windows
  QAction* colorChooserAction_;
  // activeColorChooserAciton_ is displayed on the toolbar window list
  // of colorChooser as the current active window (and nowhere else)
  QAction* activeColorChooserAction_;

  windowName<colorCompare*> colorCompareWindow_;
  QAction* colorCompareAction_;
  QAction* activeColorCompareAction_;

  windowName<squareWindow*> squareWindow_;
  QAction* squareWindowAction_;
  QAction* activeSquareWindowAction_;

  windowName<patternWindow*> patternWindow_;
  QAction* patternWindowAction_;
  QAction* activePatternWindowAction_;

  // the common window menu shared by the main windows, managed by us
  QMenu* windowMenu_;

  // the checkbox common to all windows that determines whether quick
  // help is auto shown or not
  QAction* autoShowQuickHelp_;

  // true if we don't want any of the main windows visible
  // ONLY used during restore
  bool hideWindows_;

  // the counts are used to name new images sequentially: "Image 1",
  // "Image 2", etc.
  countIterator colorCompareCount_;
  countIterator squareCount_;
  countIterator patternCount_;

  QList<colorCompareSaver> colorCompareSavers_;
  QList<squareWindowSaver> squareWindowSavers_;
  QList<patternWindowSaver> patternWindowSavers_;

  QString version_;
};

#endif
