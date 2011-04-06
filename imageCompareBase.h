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

#ifndef IMAGECOMPAREBASE_H
#define IMAGECOMPAREBASE_H

#include <QtGui/QLabel>

#include "imageSaverWindow.h"
#include "utility.h"
#include "triC.h"
#include "imageContainer.h"

class comboBox;
class imageLabelBase;
class imageLabel;
class windowManager;
class leftRightAccessor;
class imageCompareBase;
class QScrollArea;
class QSplitter;

// a text label with knowledge of a "buddy" label and three states:
// off (disabled), on (enabled), and current (enabled, bold, and
// underlined).  Only one of the two buddies can be current at a time.
// Each label in the pair has to call setBuddy to establish the buddy
// connection.
// Note: the label text will be appended with ':'
class stateLabel : public QLabel {
 public:
  explicit stateLabel(const QString& label) : label_(label) {
    setText(label_ + ":");
    setEnabled(false);
    setFixedWidth(sWidth(label_ + ":"));
  }
  // enable the label
  void on() { setText(label_ + ":"); setEnabled(true); on_ = true; }
  // disable the label
  void off() { setText(label_ + ":"); setEnabled(false); on_ = false; }
  // enable, underline, and bold the label
  // if the buddy is currently cur, it will become just enabled, otherwise
  // it stays as is
  void cur() {
    setEnabled(true); on_ = true;
    setText("<b><u><font color='green'>" + label_ + "</font></u>:</b>");
    buddy_->buddyIsCur();
  }
  // save a pointer to the buddy label for this label
  void setBuddy(stateLabel* buddyLabel) { buddy_ = buddyLabel; }
  // called for notification that this label's buddy is now cur, i.e.
  // we're not cur, our buddy is
  // we drop to simply enabled if we were cur
  void buddyIsCur() { if ( on_ ) { on(); } }
 private:
  QString label_;
  // we're on if we're on() or cur(); there's no way to tell which one
  // we're in
  bool on_;
  stateLabel* buddy_;
};

//// As a protest against C++ function pointer yuckiness, I've decided to
//// cut off my nose to spite my face (or something like that)
class zoomHelperFunction {
 public:
  explicit zoomHelperFunction(imageCompareBase* parent) : parent_(parent) {}
  virtual void operator()(const leftRightAccessor& lra,
                          const QSize& scrollSize) const = 0;
 protected:
  imageCompareBase* parent() const { return parent_; }
 private:
  imageCompareBase* parent_;
};
class zoomToWidthHelperFunction : public zoomHelperFunction {
 public:
  explicit zoomToWidthHelperFunction(imageCompareBase* parent)
    : zoomHelperFunction(parent) {}
  inline void operator()(const leftRightAccessor& lra,
                         const QSize& scrollSize) const;
};
class zoomToHeightHelperFunction  : public zoomHelperFunction {
 public:
  explicit zoomToHeightHelperFunction(imageCompareBase* parent)
    : zoomHelperFunction(parent) {}
  inline void operator()(const leftRightAccessor& lra,
                         const QSize& scrollSize) const;
};
class zoomToImageHelperFunction  : public zoomHelperFunction {
 public:
  explicit zoomToImageHelperFunction(imageCompareBase* parent)
    : zoomHelperFunction(parent) {}
  inline void operator()(const leftRightAccessor& lra,
                         const QSize& scrollSize) const;
};
class zoomToOriginalHelperFunction  : public zoomHelperFunction {
 public:
  explicit zoomToOriginalHelperFunction(imageCompareBase* parent)
    : zoomHelperFunction(parent) {}
  inline void operator()(const leftRightAccessor& lra,
                         const QSize& scrollSize) const;
};

// class imageCompareBase
//
// imageCompareBase provides common functionality for the widgets that
// provide side by side image splitter capabilities (colorCompare and
// squareWindow).  One or two images can be viewed at a time.  The two
// sides can never show the same image at once.  The user is provided
// a left and a right image menu from which to choose the image to show,
// and to hide a given side or delete the image in a given side.  Images
// can also be chosen from pull down combo boxes on the toolbar.
//
// Several user-generated actions are context dependent (originalSize(),
// for example, but see derived functionality for many other examples).
// Context is either left side or right side, and is determined (usually)
// by a mouse click on the desired side or clicking a left or right side
// button on the toolbar or by shift-left or -right arrow.  Current
// context is indicated by a black frame around the current side as well
// as via a highlighted "L:" or "R:" on the toolbar.
//
// By default, both sides scroll and zoom together so that each side
// is showing the same part of the image at all times (up to scaling),
// but the user can turn off dual scrolling and/or zooming from the Image
// menu.
//
////
// Implementation Notes
//
// The sides come from a QSplitter, which has two sides, each holding a
// scroll area.  Each scroll area holds an imageLabel, which maintains
// the actual image at the current scale.
// This class tracks which image is on which side using leftImage_,
// rightImage_, and curImage_ - curImage_ points to either leftImage_ or
// rightImage_, whichever is the active image.
//
// After construction there is always a curImage_.
// after construction, there is always a leftImage_ and a rightImage_
// (with the exception that if all images except original are deleted
// then there is only a left or a right, but the widget is not visible,
// and will not become visible again until a new image is added).
//
// The image pointers actually point to imageContainers which contain
// extra information and functionality for the image.  Since the desired
// functionality depends on what type of image is being stored (and
// access to that functionality requires knowledge of the type), the
// image container type should have been templated into this class.
// Unfortunately Qt does not support templated QObject classes
// http://doc.trolltech.com/qq/qq15-academic.html
// The workaround used here is as follows:
//
// 1) This class provides virtual pointer-to-image-container
// setters which MUST be used to assign leftImage_, rightImage_,
// and curImage_ so that derived classes can handle such changes
// as appropriate.
//
// 2) This class NEVER creates its own imageContainers - only derived
// classes create imageContainers (so that they can actually use
// derivations of imageContainers).
//
// 3) The same situation applies to left and right imageLabels (users of
// this class will want extra functionality for their labels depending on
// what type of image it is they're putting in their label).  The labels
// are only created at construction time, BY DERIVED CLASSES, so this is
// less of an issue than with imageContainers.
//
// In practice, colorCompare actually does use imageContainers and
// imageLabels, while squareWindow uses squareImageContainers and
// squareImageLabels, which are derived types.
//
// All of the left and right processing code is identical, so to avoid
// code duplication for left and right functions, left and right
// processor functions simply create a leftRightAccessor object for the
// given side which hides the explicit side but provides access to any
// desired pointers for that side, and then pass the accessor and other
// required non-side-specific information to a common processing function
// for that path (see, for example, hideLeft and hideRight, which both
// call hideLR ("LR" for Left/Right) to do the actual processing).
//
class imageCompareBase : public imageSaverWindow {
  // the accessor classes (only leftRightAccessor is directly used here)
  // simply provide access to "left" or "right" pointers (such as
  // leftImage, leftLabel, leftScroll, etc.) without their user needing
  // to know which side they actually came from
  friend class baseAccessor;
  friend class leftAccessor;
  friend class rightAccessor;
  friend class leftRightAccessor;
  // implementing polymorphic functors for calling private member functions
  // requires friendship
  friend class zoomToOriginalHelperFunction;
  friend class zoomToWidthHelperFunction;
  friend class zoomToHeightHelperFunction;
  friend class zoomToImageHelperFunction;

  Q_OBJECT

 protected:
  // the active scroll area gets an extra highlight frame of width
  // ACTIVE_LINE_WIDTH
  enum {ACTIVE_LINE_WIDTH = 3, INACTIVE_LINE_WIDTH = 0};
  // a "left" or "right" enum for passing around sidedness
  enum icLR {IC_LEFT, IC_RIGHT};

 public:
  explicit imageCompareBase(windowManager* windowMgr);
  // delete the image with index <imageIndex>
  void removeImage(int imageIndex);

 protected:
  // set the current image to <container> and update subwidgets to reflect
  // the new active side
  virtual void setCur(imagePtr container);
  // update the active image label to view curImage's image
  virtual void updateImageLabelImage() = 0;
  virtual imageLabelBase* leftLabel() const = 0;
  virtual imageLabelBase* rightLabel() const = 0;
  virtual void setLeftImage(imagePtr container) = 0;
  virtual imagePtr leftImage() const = 0;
  virtual void setRightImage(imagePtr container) = 0;
  virtual imagePtr rightImage() const = 0;
  virtual void setCurImage(imagePtr container) = 0;
  virtual imagePtr curImage() const = 0;
  // return a list of all current images
  QList<imagePtr> images() const;
  QScrollArea* leftScroll() const { return leftScroll_; }
  QScrollArea* rightScroll() const { return rightScroll_; }
  // add an image to the left image menu
  void addLeftImageMenuAction(QAction* action);
  // add an image to the right image menu
  void addRightImageMenuAction(QAction* action);
  // if <b> then enable actions that operate on both sides of the splitter
  void bothScrollsVisible(bool b);
  // update the left and right image menus to reflect the current left
  // and right images - the left image appears grayed out on the right
  // menu and vice versa
  // may update show/hide/delete options as well
  void updateImageLists();
  // set the left side image to Original and make it cur
  void setLeftToOriginalImage();
  imageLabelBase* activeLabel() {
    return (curImage() == leftImage()) ? leftLabel() : rightLabel();
  }
  imageLabelBase* inactiveLabel() {
    return (curImage() == leftImage()) ? rightLabel() : leftLabel();
  }
  QScrollArea* activeScroll() {
    return (curImage() == leftImage()) ? leftScroll_ : rightScroll_;
  }
  virtual bool eventFilter(QObject* watched, QEvent* event);
  virtual void keyPressEvent(QKeyEvent* event);
  virtual bool filterKeyEvent(QKeyEvent* event);
  // add the left and right toolbar image boxes and labels
  // intended for use by derived classes
  void addLeftRightToolBarBoxes();
  // add the left and right toolbar focus buttons
  // intended for use by derived classes
  void addLeftRightFocusButtons();
  // return the imageContainer corresponding to the given <imageName>;
  // returns NULL if image isn't found
  imagePtr getImageFromName(const QString& imageName) const;
  // return the imageContainer corresponding to the given <index>,
  // where <index> is as in the image name "Image <index>" or the original
  // image if index is 0;
  // returns NULL if image isn't found
  imagePtr getImageFromIndex(int index) const;
  // return the scroll value for a scroll with max <newMax> corresponding
  // to a scroll with <oldMax> and <oldValue>
  int convertScrollValue(int oldMax, int newMax, int oldValue) {
    return qRound(qreal(newMax)/oldMax * oldValue); // newValue
  }
  //// base methods for both Left and Right slot processing
  // delete the image referenced by <lra> and replace it with another image
  // (or notify windowManager if no other is available)
  void deleteLR(const leftRightAccessor& lra);
  void hideLR(const leftRightAccessor& lra);
  void showLR(const leftRightAccessor& lra);
  void focusLR(const leftRightAccessor& lra);
  // process a selection of the image <name> by the user from an image
  // list box
  void processBoxLR(const leftRightAccessor& lra, const QString& name);
  // process a selection of <action> by the user from an image menu list
  void processImageMenuTriggerLR(QAction* action,
                                 const leftRightAccessor& lra);
  // update the image lists to reflect selection of lra.image()
  void updateImageListsLR(const leftRightAccessor& lra);
  // let it be known that the given image (index) has been deleted
  virtual void imageDeleted(int imageIndex) = 0;
  bool dualZoomingActive() const { return dualZoomingAction_->isChecked(); }
  // zoom setting changes can change both sides, so update data that track
  // both sides
  void updateZoomValues() {
    setSavedImageSize();
    updateScrollValues();
  }
  // generic zoom to helper function that implements common zoom
  // to functionality using <helperFunction>
  virtual void zoomToHelper(const zoomHelperFunction& helperFunction);

 protected slots:
  // process selection of <imageName> from the left image list box
  void processLeftBox(const QString& imageName);
  void processRightBox(const QString& imageName);
  // process selection of <action> from the left image menu list
  void processLeftImageMenuTrigger(QAction* action);
  void processRightImageMenuTrigger(QAction* action);
  void focusLeft();
  void focusRight();
  void showHideLeft(bool show);
  void showHideRight(bool show);
  void deleteLeft();
  void deleteRight();
  // switch the left and right images (if there are both left and right)
  void switchSides();
  // center the splitter so that the left and right images have the same
  // width
  void centerSplitter();
  // zoom in or out on the image by the given pixel amount
  virtual void zoom(int zoomIncrement);
  // make the current image its original size
  void originalSize();
  void zoomToWidth();
  void zoomToHeight();
  void zoomToImage();
  // do any processing required for a dual zoom setting change
  virtual void updateDualZooming(bool dualZoomOn);
  /* the following are for dual scrolling */
  // the Left Horizontal Scroll Bar moved, so move the right as well
  void processLHSBsliderMoved(int value);
  // the Left Horizontal Scroll Bar value changed, so change the right as
  // well
  void processLHSBValue(int value);
  void processLVSBsliderMoved(int value);
  void processLVSBValue(int value);
  void processRHSBsliderMoved(int value);
  void processRHSBValue(int value);
  void processRVSBsliderMoved(int value);
  void processRVSBValue(int value);
  // the dual scrolling checkbox has been changed
  void processDualScrollingChange(bool dualScrollingOn);

 private:
  // construction helper
  void constructScrolling();
  // construction helper - setup signal/slot connections
  void setConnections();
  void setScrollingConnections(bool checked);
  // return the scaled size of the current image
  // implements imageSaverWindow::
  QSize curImageViewSize() const;
  // set the <label> and the <image> to <size>, and update anything that
  // needs to know about the changes
  void setScaledImageSize(const QSize size, imageLabelBase* label,
                          imagePtr image);
  virtual void setScaledImageWidth(int width, imageLabelBase* label,
                                   imagePtr image);
  virtual void setScaledImageHeight(int height, imageLabelBase* label,
                                    imagePtr image);
  void originalSizeHelper(const leftRightAccessor& lra,
                          const QSize& scrollSize);
  void zoomToWidthHelper(const leftRightAccessor& lra,
                         const QSize& scrollSize);
  void zoomToHeightHelper(const leftRightAccessor& lra,
                          const QSize& scrollSize);
  void zoomToImageHelper(const leftRightAccessor& lra,
                         const QSize& scrollSize);
  imagePtr inactiveImage() {
    return (curImage() == leftImage()) ? rightImage() : leftImage();
  }
  // save the sizes of the current left and right side images
  void setSavedImageSize();
  // return the stored last image size for the current side, using
  // <newImage> to seed it if it hasn't already been set
  QSize getSavedImageSize(const imagePtr newImage);
  // let it be known that the only image left is the original
  // this MUST result in this window being hidden until a new image gets
  // added
  virtual void imageListEmpty() = 0;
  // delete the menu actions associated with the given image and call
  // imageDeleted to notify super classes
  void deleteImageActions(const QString& imageName);

  // if we're doing dual scrolling then synchronize the scroll values
  void updateScrollValues();
  // implements imageZoomWindow::processFirstShow
  void processFirstShow();

 private:
  QScrollArea* leftScroll_;
  QScrollArea* rightScroll_;
  QSplitter* splitter_;

  // a label showing "L:" in the toolbar that indicates the state of the
  // left side
  stateLabel* LLabel_;
  QMenu* leftImageMenu_;
  // a combo box for the toolbar listing possible left images
  comboBox* leftImageListBox_;
  // checkable - checked means show
  QAction* leftShowHide_;
  QAction* leftDelete_;
  // remember the size of the most current image on this side
  QSize leftImageSavedSize_;

  stateLabel* RLabel_;
  QMenu* rightImageMenu_;
  comboBox* rightImageListBox_;
  QAction* rightShowHide_;
  QAction* rightDelete_;
  QSize rightImageSavedSize_;

  // let the user switch the images
  QAction* switchAction_;
  // let the user equalize the image widths in the splitter
  QAction* centerSplitterAction_;
  // let the user turn on/off dual scrolling for left and right images
  QAction* dualScrollingAction_;
  // let the user turn on/off dual zooming for left and right iamges
  QAction* dualZoomingAction_;
  // let the user click a toolbar button to set focus to the left
  QAction* leftFocusAction_;
  QAction* rightFocusAction_;
};

void zoomToWidthHelperFunction::operator()(const leftRightAccessor& lra,
                                           const QSize& scrollSize) const {
  parent()->zoomToWidthHelper(lra, scrollSize);
}

void zoomToHeightHelperFunction::operator()(const leftRightAccessor& lra,
                                            const QSize& scrollSize) const {
  parent()->zoomToHeightHelper(lra, scrollSize);
}

void zoomToImageHelperFunction::operator()(const leftRightAccessor& lra,
                                           const QSize& scrollSize) const {
  parent()->zoomToImageHelper(lra, scrollSize);
}

void 
zoomToOriginalHelperFunction::operator()(const leftRightAccessor& lra,
                                         const QSize& scrollSize) const {
  parent()->originalSizeHelper(lra, scrollSize);
}

#endif
