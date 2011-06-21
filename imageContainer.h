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

#ifndef IMAGECONTAINER_H
#define IMAGECONTAINER_H

#include <QtCore/QSharedData>
#include <QtCore/QMetaType>
#include <QtCore/QSize>

#include <QtGui/QImage>

#include "triC.h"
#include "floss.h"

extern const int ZOOM_INCREMENT;

class imageContainer;
class squareImageContainer;

typedef QExplicitlySharedDataPointer<imageContainer> imagePtr;

//
// imageContainer holds an image along with associated data, including
// the image's name, its scaled width, its floss color class, and the
// color list for the image (implemented by derived classes).  Derived
// classes must hold and return the image and hold and return the
// color list.
//
// squareContainer() was added as a workaround for QT's lack of template
// support (cf. imageCompareBase.h doc) and as an alternative to
// dynamic_casting, which requires an extra dll for mingw on Windows
//
class imageContainer : public QSharedData {

 public:
  // <dmc> if the initial image is DMC colors only
  imageContainer(const QString& imageName, const QSize initialImageSize,
                 flossType type)
   : name_(imageName), scaledSize_(initialImageSize), flossType_(type) {}
  virtual const squareImageContainer* squareContainer() const { return NULL; }
  virtual squareImageContainer* squareContainer() { return NULL; }
  QString name() const { return name_; }
  QSize scaledSize() const { return scaledSize_; }
  int scaledWidth() const { return scaledSize_.width(); }
  int scaledHeight() const { return scaledSize_.height(); }
  // this used to be setScaledWidth, but QPixmap::scaledTo[Width|Height]
  // can give bad results if you feed it say a height, then read the
  // width and then scale to that width (bad meaning you sometimes don't get
  // the original height back!).
  // Return the actual size set
  virtual QSize setScaledSize(const QSize& size) {
    scaledSize_ = size;
    return scaledSize_;
  }
  virtual QSize zoom(bool zoomIn) {
    const int zoomIncrement = zoomIn ? ZOOM_INCREMENT : -ZOOM_INCREMENT;
    const int newWidth = scaledSize_.width() + zoomIncrement;
    if (newWidth < 1) { //
      return scaledSize_;
    }
    const int newHeight =
      qRound(originalHeight() * static_cast<qreal>(newWidth)/originalWidth());
    scaledSize_ = QSize(newWidth, newHeight);
    return scaledSize_;
  }
  // Return true if this container is holding the original image.
  virtual bool isOriginal() const = 0;
  // Return the floss type of the initial color list.
  flossType flossMode() const { return flossType_; }
  virtual const QImage& image() const = 0;
  virtual QImage scaledImage() const {
    return image().scaled(scaledSize_, Qt::IgnoreAspectRatio);
  }
  int originalWidth() const { return image().width(); }
  int originalHeight() const { return image().height(); }
  QSize originalSize() const { return image().size(); }
  virtual QVector<triC> colors() const = 0;
  virtual QVector<flossColor> flossColors() const = 0;

 private:
  QString name_;
  QSize scaledSize_; // current scaled size
  // are the colors for this image all of one floss type?
  const flossType flossType_;

};
// make imagePtr known to QVariant
Q_DECLARE_METATYPE(imagePtr)

// mutableImageContainer copies in its image and makes no guarantees about the
// constness of that image (although in this case it is const).
class mutableImageContainer : public imageContainer {

 public:
  mutableImageContainer(const QString& imageName, const QImage& image,
                        const QVector<triC>& colors, flossType type)
    : imageContainer(imageName, image.size(), type), image_(image),
    colors_(colors) {}
  const QImage& image() const { return image_; }
  QVector<triC> colors() const { return colors_; }
  QVector<flossColor> flossColors() const {
    QVector<flossColor> returnVector;
    const flossType type = flossMode();
    for (int i = 0, size = colors_.size(); i < size; ++i) {
      returnVector.push_back(flossColor(colors_[i], type));
    }
    return returnVector;
  }
  bool isOriginal() const { return false; }

 private:
  const QImage image_;
  QVector<triC> colors_;
};

// immutableImageContainer copies a _const_ _reference_ to its image and
// guarantees that the reference image will not be altered.
class immutableImageContainer : public imageContainer {

 public:
 immutableImageContainer(const QString& imageName, const QImage& image)
   : imageContainer(imageName, image.size(), flossVariable),
     image_(image) {}
  const QImage& image() const { return image_; }
  QVector<triC> colors() const { return QVector<triC>(); }
  QVector<flossColor> flossColors() const { return QVector<flossColor>(); }
  bool isOriginal() const { return true; }

 private:
  const QImage& image_;
};

#endif
