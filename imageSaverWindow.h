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

#ifndef IMAGESAVERWINDOW_H
#define IMAGESAVERWINDOW_H

#include "imageZoomWindow.h"

// class imageSaverWindow
//
// This class is inherited by the three main windows that provide
// saving capability (colorCompare, squareWindow, and patternWindow).
// The capability provided is to save a currently viewed image at its
// current zoom size as either a pdf or any one of the image formats
// supported by Qt (determined by save file extension).
//
// The pure virtual function curImageForSaving() is used to
// fetch whatever the derived widget considers to be the current image.
//
class imageSaverWindow : public imageZoomWindow {

  Q_OBJECT

 public:
  // dockName and winMgr are passed on to imageZoomWindow
  imageSaverWindow(const QString& dockName, windowManager* winMgr);

 protected:
  void setSaveImageActionEnabled(bool b);

 protected slots:
  // get a filename from the user, ensuring that it has a supported
  // extension (either pdf or a Qt-supported image format)
  // warn the user if the image to be saved is "large"
  // save the current image (see curImageForSaving()) to the user's file
  void processSaveImage();

 private:
  // return the current image to be saved
  virtual QImage curImageForSaving() const = 0;
  // return the size of the current image
  virtual QSize curImageViewSize() const = 0;
  // save <image> as a pdf to <outputFile> with default margins plus the
  // current image size (so not necessarily 8.5x11)
  void doPdfSave(const QString& outputFile, const QImage& image);
  // save <image> to <outputFile>, using <outputFile>'s extension as the
  // image format to save in - the extension must have already been
  // vetted as one supported by Qt (see getValidSaveFile).  If the format
  // supports compression then a popup is used to let the user select
  // compression/quality on a scale of 0 to 100.
  void doNonPdfSave(const QString& outputFile, const QImage& image);
  // get a file name from the user that ends in one of the extensions
  // listed in <formats>.  Loop until the user either enters a valid
  // filename or cancels.
  // Returns a valid file name or the empty string
  QString getValidSaveFile(const QList<QByteArray>& formats);

 private:
  // let the user save an image
  QAction* saveImageAction_;
};

#endif
