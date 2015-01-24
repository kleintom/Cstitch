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

#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include <QtWidgets/QWidget>

class imageLabelBase : public QWidget {

  Q_OBJECT

 public:
  explicit imageLabelBase(QWidget* parent) : QWidget(parent) {}
  // the image's current size
  virtual QSize size() const = 0;
  // the image's current width
  virtual int width() const = 0;
  // the image's current height
  virtual int height() const = 0;
  virtual int originalWidth() const = 0;
  virtual int originalHeight() const = 0;
  virtual void setImageSize(const QSize& size) = 0;
  virtual void setImageWidth(int width) = 0;
  virtual void setImageHeight(int height) = 0;
  // probably returns the scaled image size
  QSize sizeHint() const { return size(); }

 private:
  void mousePressEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  // draws the image at the current size setting
  virtual void paintEvent(QPaintEvent* event) = 0;

 signals:
  void announceImageClick(QMouseEvent* event);
  void mouseMoved(QMouseEvent* event);
  void mouseReleased(QMouseEvent* event);
};

// An imageLabel is used to display an image and handle mouse tracking
// over that image.
//
//// Implementation notes
//
// imageLabel holds its image as a QPixmap for quick display.
// The image is actually painted on the widget (not displayed on a QLabel).
// If mouseTracking is on then this widget emits signals for mouse presses,
// releases, and moves.
//
class imageLabel : public imageLabelBase {

  Q_OBJECT

 public:
  explicit imageLabel(QWidget* parent);
  imageLabel(const QPixmap& image, QWidget* parent);
  bool imageIsNull() const { return originalImage_.isNull(); }
  // the current scaled width
  int width() const { return scaledImage_.width(); }
  // the current scaled height
  int height() const { return scaledImage_.height(); }
  QSize size() const { return scaledImage_.size(); }
  int originalWidth() const { return originalImage_.width(); }
  int originalHeight() const { return originalImage_.height(); }
  void setMouseTracking(bool b) { QWidget::setMouseTracking(b); }
  // set a new <image> and redraw the <rectangle> portion of the widget,
  // but don't change the current image size settings
  void updateImage(const QPixmap& image, const QRect& rectangle = QRect());
  // sets a new image and resets the size settings to <image>
  void setImageAndSize(const QPixmap& image);
  // change the displayed image width to <width> and set height to maintain
  // the aspect ratio
  void setImageWidth(int width);
  void setImageHeight(int height);
  void setImageSize(const QSize& size);

 protected:
  // draws the image at the current size setting
  virtual void paintEvent(QPaintEvent* event);

 private:
  QPixmap originalImage_;
  QPixmap scaledImage_;
};

#endif

