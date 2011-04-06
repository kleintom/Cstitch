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

#ifndef DOCKIMAGE_H
#define DOCKIMAGE_H

#include <QtCore/QRect>

#include <QtGui/QWidget>
#include <QtGui/QPixmap>

// the dockImage appears in a dock widget and displays one of two images
// at a time: an original image or another image (which in practice is a
// squared version of the original); a right click switches between the
// two.
// The widget displays a "viewport" red outlined rectangle that is
// meant to represent the portion of the image currently visible in
// the main window.  The user can click and/or drag anywhere in the
// dock image to move the "viewport" and, via emitted signals, the view
// in the main window.  Conversely, the main window can call
// updateViewport to make the viewport respond to changes in the view
// of the image on the main window.
class dockImage : public QWidget {

  Q_OBJECT

 public:
  dockImage(const QImage& originalImage, QWidget* parent);
  // set the non-original <image>, scaled to fit the dock width, and
  // create a new original image cropped from the upper left corner to
  // the size of <image>
  void setImage(const QImage& image);
  // move the viewport rectangle so that the left edge is
  // <xRatio1>*width along the image, etc.
  void updateViewport(qreal xRatio1, qreal xRatio2,
                      qreal yRatio1, qreal yRatio2);

 private:
  void paintEvent(QPaintEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  // move the viewport to be centered at <newCenter>, moved back onto the
  // image if necessary
  void moveViewport(const QPoint& newCenter);
  // reset the viewport if necessary so that it's entirely within the image
  void maybeCorrectViewport();

 signals:
  // <rightEdge> if the right side of the viewport is on the right side
  // of the image (the percentages aren't accurate enough to get that right
  // on their own
  void viewportUpdated(qreal xPercentage, qreal yPercentage,
                       bool rightEdge, bool bottomEdge);

 private:
  QPixmap image_; // the non-original image
  // the portion of the original image that we show may depend on the size
  // of image_, so keep a ref to the unaltered version
  const QImage& originalImageRef_;
  QPixmap originalImage_;
  bool showingOriginal_; // true if the original image is currently shown
  bool dragging_; // true if mouse is being drug in this widget
  QRect viewport_;
};

#endif
