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

#ifndef LEFTRIGHTACCESSORS_H
#define LEFTRIGHTACCESSORS_H

#include "imageCompareBase.h"

//
// leftRightAccessors are initialized as either "left" or "right", and
// then provide a left/right-free interface to the pointers they serve up
// (from an imageCompareBase).
// The user is provided the same interface to all left pointers or all
// right pointers (depending on how the accessor was initialized) without
// having to know if they're dealing with left or right.
//
//// Implementation notes
//
// leftRightAccessor provides the abstract interface; all fetches are done
// through delegation to a pointer to an imageCompareBase, which is the
// sole piece of data stored.
// leftAccessor and rightAccessor provide the left or right implementation.
// The user passes a "left" or "right" to the leftRightAccessor constructor,
// which is an RAII wrapper for the appropriate left/rightAccessor.
//
class leftRightAccessor;
class baseAccessor {
 // allow access to parent for assignment
  friend class leftRightAccessor;
 public:
  explicit baseAccessor(const imageCompareBase* parent) : parent_(parent) {}
  virtual QScrollArea* scroll() const = 0;
  virtual QScrollArea* oppositeScroll() const = 0;
  virtual stateLabel* toolbarLabel() const = 0;
  virtual QAction* showHideAction() const = 0;
  virtual QAction* deleteAction() const = 0;
  virtual imageLabelBase* label() const = 0;
  virtual imagePtr image() const = 0;
  virtual imagePtr oppositeImage() const = 0;
  virtual QMenu* imageMenu() const = 0;
  virtual comboBox* imageListBox() const = 0;
  virtual QAction* focusAction() const = 0;
 protected:
  const imageCompareBase* parent() const { return parent_; }
  virtual imageCompareBase::icLR side() const = 0;
 private:
  const imageCompareBase* parent_;
};

class leftAccessor : public baseAccessor {

 public:
  explicit leftAccessor(const imageCompareBase* parent)
    : baseAccessor(parent) {}
  QScrollArea* scroll() const { return parent()->leftScroll_; }
  QScrollArea* oppositeScroll() const { return parent()->rightScroll_; }
  stateLabel* toolbarLabel() const { return parent()->LLabel_; }
  QAction* showHideAction() const { return parent()->leftShowHide_; }
  QAction* deleteAction() const { return parent()->leftDelete_; }
  imageLabelBase* label() const { return parent()->leftLabel(); }
  imagePtr image() const { return parent()->leftImage(); }
  imagePtr oppositeImage() const { return parent()->rightImage(); }
  QMenu* imageMenu() const { return parent()->leftImageMenu_; }
  comboBox* imageListBox() const { return parent()->leftImageListBox_; }
  QAction* focusAction() const { return parent()->leftFocusAction_; }
 private:
  imageCompareBase::icLR side() const { return imageCompareBase::IC_LEFT; }
};

class rightAccessor : public baseAccessor {

 public:
  explicit rightAccessor(const imageCompareBase* parent)
    : baseAccessor(parent) {}
  QScrollArea* scroll() const { return parent()->rightScroll_; }
  QScrollArea* oppositeScroll() const { return parent()->leftScroll_; }
  stateLabel* toolbarLabel() const { return parent()->RLabel_; }
  QAction* showHideAction() const { return parent()->rightShowHide_; }
  QAction* deleteAction() const { return parent()->rightDelete_; }
  imageLabelBase* label() const { return parent()->rightLabel(); }
  imagePtr image() const { return parent()->rightImage(); }
  imagePtr oppositeImage() const { return parent()->leftImage(); }
  QMenu* imageMenu() const { return parent()->rightImageMenu_; }
  comboBox* imageListBox() const { return parent()->rightImageListBox_; }
  QAction* focusAction() const { return parent()->rightFocusAction_; }
 private:
  imageCompareBase::icLR side() const { return imageCompareBase::IC_RIGHT; }
};

class leftRightAccessor {

 public:
  leftRightAccessor(const imageCompareBase* parent, imageCompareBase::icLR side)
    : baseAccessor_(NULL) {

    if (side == imageCompareBase::IC_LEFT) {
      baseAccessor_ = new leftAccessor(parent);
    }
    else {
      baseAccessor_ = new rightAccessor(parent);
    }
  }
  leftRightAccessor(const imageCompareBase* parent,
                    const imagePtr container)
    : baseAccessor_(NULL) {

    if (container == parent->leftImage()) {
      baseAccessor_ = new leftAccessor(parent);
    }
    else if (container == parent->rightImage()) {
      baseAccessor_ = new rightAccessor(parent);
    }
    else {
      // oops; this probably won't end well
      baseAccessor_ = new rightAccessor(parent);
      qWarning() << "Parent/container mismatch in leftRightAccessor" <<
        parent->leftImage().constData() << parent->rightImage().constData() <<
        container.constData();
    }
  }
  leftRightAccessor& operator=(const leftRightAccessor& other) {

    if (this == &other) {
      return *this;
    }
    delete baseAccessor_;
    if (other.baseAccessor_->side() == imageCompareBase::IC_LEFT) {
      baseAccessor_ = new leftAccessor(other.baseAccessor_->parent());
    }
    else {
      baseAccessor_ = new rightAccessor(other.baseAccessor_->parent());
    }
    return *this;
  }
  ~leftRightAccessor() { delete baseAccessor_; }
  bool valid() const { return baseAccessor_ != NULL; }
  QScrollArea* scroll() const { return baseAccessor_->scroll(); }
  QScrollArea* oppositeScroll() const {
    return baseAccessor_->oppositeScroll();
  }
  stateLabel* toolbarLabel() const { return baseAccessor_->toolbarLabel(); }
  QAction* showHideAction() const { return baseAccessor_->showHideAction(); }
  QAction* deleteAction() const { return baseAccessor_->deleteAction(); }
  imageLabelBase* label() const { return baseAccessor_->label(); }
  imagePtr image() const { return baseAccessor_->image(); }
  imagePtr oppositeImage() const {
    return baseAccessor_->oppositeImage();
  }
  QMenu* imageMenu() const { return baseAccessor_->imageMenu(); }
  comboBox* imageListBox() const { return baseAccessor_->imageListBox(); }
  QAction* focusAction() const { return baseAccessor_->focusAction(); }

 private:
  // prohibit copy
  leftRightAccessor(const leftRightAccessor& lra);

 private:
  baseAccessor* baseAccessor_;
};

#endif
