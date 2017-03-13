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

#ifndef SQUARETOOLS_H
#define SQUARETOOLS_H

#include <QtCore/QSet>
#include <QCoreApplication>

#include "imageUtility.h"

class colorDialog;
class squareWindow;
class QMouseEvent;

// raw data structure for caching of data needed during a drawing drag
struct changeOneDragCache {
  changeOneDragCache() :labelWidth(0), labelHeight(0), imageWidth(0),
    imageHeight(0), squareDim(0), cacheIsActive(false) {}
  void clear() {
    squaresVisited.clear();
    cacheIsActive = false;
  }
  flossColor newColor; // the drawing color
  int labelWidth, labelHeight, imageWidth, imageHeight, squareDim;
  bool cacheIsActive; // true if this cache is currently being used
  QSet<pairOfInts> squaresVisited; // squares we've already drawn on
  QMatrix matrix; // scaling matrix between displayed and original images
};

// raw data structure for caching of data needed during a detailing drag
struct detailDragCache {
  detailDragCache() :labelWidth(0), labelHeight(0), imageWidth(0),
    imageHeight(0), squareDim(0), cacheIsActive(false) {}
  void clear() {
    squaresVisited.clear();
    cacheIsActive = false;
  }
  int labelWidth, labelHeight, imageWidth, imageHeight, squareDim;
  bool cacheIsActive; // true if this cache is currently being used
  QSet<pixel> squaresVisited; // squares we've already drawn on
};

//
// class squareTool
//
// squareTool is the abstract interface for a square image tool
// each tool is a friend of squareWindow and performs the
// interface specified actions directly on the window's objects
//
class squareTool {

 public:
  explicit squareTool(squareWindow* parent) : parent_(parent) {}
  // do what this tool does when the user clicks on the active image
  virtual void activeImageClick(QMouseEvent* event) = 0;
  // do what this tool does when the user clicks on the inactive image
  virtual void inactiveImageClick(QMouseEvent* event) = 0;
  // do what this tool does when the user moves the mouse on the active
  // image
  virtual void activeMouseMove(QMouseEvent* event) = 0;
  // do what this tool does when the user releases the mouse
  virtual void mouseRelease() = 0;
  // set up colorDialog connections for this tool
  virtual void connectChangeColorDialog(colorDialog* dialog) = 0;
  // set the status bar text for how to use this tool
  virtual void setMouseHint() = 0;
 protected:
  squareWindow* parent() const { return parent_; }
  // Update the tool dock tool color swatch and highlight the color in the color
  // list.
  void updateToolColor(QRgb newColor) const;
 private:
  squareWindow* parent_;
};

class inactiveImageClickBase : public squareTool {

 public:
  explicit inactiveImageClickBase(squareWindow* parent) : squareTool(parent) {}
  void inactiveImageClick(QMouseEvent* event);
  void connectChangeColorDialog(colorDialog* dialog);
};

class nullTool : public squareTool {

 public:
  explicit nullTool(squareWindow* parent) : squareTool(parent) {}
  void activeImageClick(QMouseEvent* ) {}
  void inactiveImageClick(QMouseEvent* ) {}
  void activeMouseMove(QMouseEvent* event) {
    squareTool::activeMouseMove(event);
  }
  void mouseRelease() {}
  void connectChangeColorDialog(colorDialog* ) {}
  void setMouseHint();
};

class changeAllTool : public inactiveImageClickBase {

 public:
  explicit changeAllTool(squareWindow* parent)
    : inactiveImageClickBase(parent) {}
  void activeImageClick(QMouseEvent* event);
  void activeMouseMove(QMouseEvent* event) {
    squareTool::activeMouseMove(event);
  }
  void mouseRelease() {}
  void setMouseHint();
};

class changeOneTool  : public inactiveImageClickBase {

 public:
  explicit changeOneTool(squareWindow* parent)
    : inactiveImageClickBase(parent) {}
  void activeImageClick(QMouseEvent* event);
  void activeMouseMove(QMouseEvent* event);
  void mouseRelease();
  void setMouseHint();
 private:
  // cached values when the mouse is being dragged
  changeOneDragCache dragCache_;
};

class fillTool  : public inactiveImageClickBase {

 public:
  explicit fillTool(squareWindow* parent) : inactiveImageClickBase(parent) {}
  void activeImageClick(QMouseEvent* event);
  void activeMouseMove(QMouseEvent* event) {
    squareTool::activeMouseMove(event);
  }
  void mouseRelease() {}
  void setMouseHint();
};

class detailTool : public squareTool {

 public:
  explicit detailTool(squareWindow* parent) : squareTool(parent) {}
  void activeImageClick(QMouseEvent* event);
  void inactiveImageClick(QMouseEvent* ) {}
  void activeMouseMove(QMouseEvent* event);
  void mouseRelease();
  void connectChangeColorDialog(colorDialog* ) {}
  void clearCache() {
    dragCache_.clear();
  }
  QList<pixel> coordinates() const {
    return dragCache_.squaresVisited.toList();
  }
  void setMouseHint();
 private:
  void processDetailEvent(int boxX, int boxY, Qt::MouseButtons buttons);
 private:
  detailDragCache dragCache_; // cached values when the mouse is being dragged
};

#endif
