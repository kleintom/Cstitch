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

#ifndef VERSIONPROCESSOR_H
#define VERSIONPROCESSOR_H

#include <QtCore/QSharedPointer>

#include "imageProcessing.h"

// Projects save only the original image and recreate all other images from
// the original image and saved information on the options used to create
// those other images in the first place.  As a result the image processing
// methods we need to use are strongly tied to the particular version of
// the project file being restored.  The classes here take care of choosing
// the correct methods based on the project version.

class CVersion {

 public:
  CVersion(const QString& versionString) : version_(versionString) {}
  bool operator>(const CVersion& otherVersion) const;
  bool operator>(const QString& otherVersion) const {
    return operator>(CVersion(otherVersion));
  }

 private:
  const QString version_;
};

// A version transformer for chooseColorsFromList
class chooseColorsVersion {

 public:
  virtual ~chooseColorsVersion() {}
  virtual QRgb transform(const colorTransformerPtr& transformer,
                         QRgb color) const = 0;
};
typedef QSharedPointer<chooseColorsVersion> chooseColorsVersionPtr;

class chooseColorsVersion_current : public chooseColorsVersion {

 public:
  QRgb transform(const colorTransformerPtr& transformer, QRgb color) const {
    return transformer->transform(color);
  }
};

class chooseColorsVersion_0_9_2_16 : public chooseColorsVersion {

 public:
  QRgb transform(const colorTransformerPtr& , QRgb color) const { 
    return color;
  }
};

// A version transformer for squareImageContainer::addColor
class addSquareColorVersion {

 public:
  virtual ~addSquareColorVersion() {}
  virtual flossColor transform(const flossColor& color) const = 0;
};
typedef QSharedPointer<addSquareColorVersion> addSquareColorVersionPtr;

class addSquareColorVersion_current : public addSquareColorVersion {

 public:
  flossColor transform(const flossColor& color) const {
    return color;
  }
};

class addSquareColorVersion_0_9_2_16 : public addSquareColorVersion {

 public:
  flossColor transform(const flossColor& color) const;
};

// versionProcessor provides the abstract interface for specific version
// processors.
class versionProcessor {

 public:
  virtual ~versionProcessor() {}
  static void setProcessor(const QString& version);
  static const versionProcessor* processor() { return processor_; }
  virtual chooseColorsVersionPtr chooseColors() const = 0;
  virtual addSquareColorVersionPtr addSquareColor() const = 0;

 private:
  static versionProcessor* processor_;
};

class versionProcessor_current : public versionProcessor {

 public:
  chooseColorsVersionPtr chooseColors() const {
    return chooseColorsVersionPtr(new chooseColorsVersion_current);
  }
  addSquareColorVersionPtr addSquareColor() const {
    return addSquareColorVersionPtr(new addSquareColorVersion_current);
  }
};

class versionProcessor_0_9_2_16 : public versionProcessor {

 public:
  chooseColorsVersionPtr chooseColors() const {
    return chooseColorsVersionPtr(new chooseColorsVersion_0_9_2_16);
  }
  addSquareColorVersionPtr addSquareColor() const {
    return addSquareColorVersionPtr(new addSquareColorVersion_0_9_2_16);
  }
};

#endif
