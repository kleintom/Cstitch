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

#include "squareTools.h"

#include "colorDialog.h"
#include "squareDockWidget.h"
#include "squareToolDock.h"
#include "squareWindow.h"

void squareTool::activeMouseMove(QMouseEvent* event) {

  const squareImageLabel* label = parent()->activeSquareLabel();
  const squareImagePtr curImage = parent()->curImage_;
  const int px = (event->x()*curImage->originalWidth())/label->width();
  const int py = (event->y()*curImage->originalHeight())/label->height();
  parent()->colorListDock_->updateColorSwatch(curImage->image().pixel(px, py));
}

void inactiveImageClickBase::inactiveImageClick(QMouseEvent* event) {

  const squareImageLabel* label = parent()->inactiveSquareLabel();
  const int w = label->width();
  const int h = label->height();
  const int x = event->x();
  const int y = event->y();
  if (event->button() == Qt::MidButton) {
    const QImage& image = parent()->curImage_->image();
    const int originalX = (x*image.width())/w;
    const int originalY = (y*image.height())/h;
    // note we use the color from the _active_ image
    QRgb oldColor;
    if (originalX < image.width() && originalY < image.height()) {
      oldColor = ::colorFromScaledImageCoords(x, y, w, h, image);
    }
    else { // the inactive image is larger than the active, so the clicked
      // point doesn't exist on active - fall back to inactive
      const QImage& inactiveImage = parent()->inactiveImage()->image();
      oldColor = ::colorFromScaledImageCoords(x, y, w, h, inactiveImage);
    }
    // the _real_ original coordinates
    const int realOriginalX = (x*parent()->originalImage().width())/w;
    const int realOriginalY = (y*parent()->originalImage().height())/h;
    parent()->requestSquareColor(realOriginalX, realOriginalY,
                                 parent()->curImage_->originalDimension(),
                                 oldColor);
  }
  else if (event->button() == Qt::RightButton) {
    const QImage& image = parent()->inactiveImage()->image();
    const int originalX = (x*image.width())/w;
    const int originalY = (y*image.height())/h;
    const QRgb newColor = image.pixel(originalX, originalY);
    parent()->toolDock_->setToolLabelColor(newColor);
  }
}

void inactiveImageClickBase::connectChangeColorDialog(colorDialog* dialog) {

  QObject::connect(dialog, SIGNAL(finished(int,
                                           const triC& , const triC& )),
                   parent(), SLOT(changeColorDialogFinished(int,
                                                            const triC& ,
                                                            const triC& )));
}

void changeOneTool::activeImageClick(QMouseEvent* event) {

  const int x = event->x();
  const int y = event->y();
  const int originalImageWidth = parent()->curImage_->originalWidth();
  const int originalImageHeight = parent()->curImage_->originalHeight();
  squareImageLabel* label = parent()->activeSquareLabel();
  const int labelWidth = label->width();
  const int labelHeight = label->height();
  int originalX = (x * originalImageWidth)/labelWidth;
  int originalY = (y * originalImageHeight)/labelHeight;
  const int originalDimension = parent()->curImage_->originalDimension();
  const int boxX = originalX/originalDimension;
  const int boxY = originalY/originalDimension;
  const QRgb oldColor =
    ::colorFromScaledImageCoords(x, y, labelWidth, labelHeight,
                                 parent()->curImage_->image());
  const Qt::MouseButton mouseButton = event->button();
  if (mouseButton == Qt::LeftButton) {
    dragCache_.cacheIsActive = true;
    dragCache_.labelWidth = labelWidth;
    dragCache_.labelHeight = labelHeight;
    dragCache_.imageWidth = originalImageWidth;
    dragCache_.imageHeight = originalImageHeight;
    dragCache_.squareDim = originalDimension;
    // this won't be used until we actually drag
    QMatrix matrix;
    matrix.scale(static_cast<qreal>(labelWidth)/originalImageWidth,
                 static_cast<qreal>(labelHeight)/originalImageHeight);
    matrix = QImage::trueMatrix(matrix,
                                originalImageWidth, originalImageHeight);
    dragCache_.matrix = matrix;
    const pairOfInts boxCoordinates(boxX, boxY);
    const QRgb newColor = parent()->toolDock_->getToolLabelColor();
    dragCache_.newColor = newColor;
    dragCache_.squaresVisited.insert(boxCoordinates);
    label->setSquaresColor(newColor);
    label->addSquare(boxCoordinates);
    const int d = parent()->roughCurDim();
    label->update(x-d, y-d, 2*d, 2*d);
  }
  else if (mouseButton == Qt::MidButton) {
    // make originals upper left corner of the square clicked
    originalX = boxX * originalDimension;
    originalY = boxY * originalDimension;
    parent()->requestSquareColor(originalX, originalY,
                                 originalDimension, oldColor);
  }
  else if (mouseButton == Qt::RightButton) {
    // make the clicked color the tool color
    parent()->toolDock_->setToolLabelColor(oldColor);
  }
}

void detailTool::activeImageClick(QMouseEvent* event) {

  const int x = event->x();
  const int y = event->y();
  const int originalImageWidth = parent()->curImage_->originalWidth();
  const int originalImageHeight = parent()->curImage_->originalHeight();
  squareImageLabel* label = parent()->activeSquareLabel();
  const int labelWidth = label->width();
  const int labelHeight = label->height();
  const int originalX = (x * originalImageWidth)/labelWidth;
  const int originalY = (y * originalImageHeight)/labelHeight;
  const int originalDimension = parent()->curImage_->originalDimension();
  const int boxX = originalX/originalDimension;
  const int boxY = originalY/originalDimension;
  dragCache_.cacheIsActive = true;
  dragCache_.labelWidth = labelWidth;
  dragCache_.labelHeight = labelHeight;
  dragCache_.imageWidth = originalImageWidth;
  dragCache_.imageHeight = originalImageHeight;
  dragCache_.squareDim = originalDimension;
  label->startDrawingHashes();
  processDetailEvent(boxX, boxY, event->buttons());
  const int d = parent()->roughCurDim();
  label->update(x-d, y-d, 2*d, 2*d);
  if (event->buttons() & Qt::LeftButton) {
    parent()->toolDock_->detailListIsEmpty(false);
  }
}

void changeAllTool::activeImageClick(QMouseEvent* event) {

  const int x = event->x();
  const int y = event->y();
  squareImageLabel* label = parent()->activeSquareLabel();
  const int labelWidth = label->width();
  const int labelHeight = label->height();
  const QRgb oldColor =
    ::colorFromScaledImageCoords(x, y, labelWidth, labelHeight,
                                 parent()->curImage_->image());
  const Qt::MouseButton mouseButton = event->button();
  if (mouseButton == Qt::LeftButton) {
    parent()->processChangeAll(oldColor,
                               parent()->toolDock_->getToolLabelColor());
  }
  else if (mouseButton == Qt::MidButton) {
    const int originalImageWidth = parent()->curImage_->originalWidth();
    const int originalImageHeight = parent()->curImage_->originalHeight();
    const int originalX = (x * originalImageWidth)/labelWidth;
    const int originalY = (y * originalImageHeight)/labelHeight;
    const int originalDimension = parent()->curImage_->originalDimension();
    parent()->requestSquareColor(originalX, originalY,
                                 originalDimension, oldColor);
  }
  else if (mouseButton == Qt::RightButton) {
    // make the clicked color the tool color
    parent()->toolDock_->setToolLabelColor(oldColor);
  }
}

void fillTool::activeImageClick(QMouseEvent* event) {

  const int x = event->x();
  const int y = event->y();
  const int originalImageWidth = parent()->curImage_->originalWidth();
  const int originalImageHeight = parent()->curImage_->originalHeight();
  squareImageLabel* label = parent()->activeSquareLabel();
  const int labelWidth = label->width();
  const int labelHeight = label->height();
  const QRgb oldColor =
    ::colorFromScaledImageCoords(x, y, labelWidth, labelHeight,
                                 parent()->curImage_->image());
  const int originalX = (x * originalImageWidth)/labelWidth;
  const int originalY = (y * originalImageHeight)/labelHeight;
  const Qt::MouseButton mouseButton = event->button();
  if (mouseButton == Qt::LeftButton) {
    const QRgb newColor = parent()->toolDock_->getToolLabelColor();
    const dockListUpdate update =
      parent()->curImage_->fillRegion(originalX, originalY, newColor);
    parent()->curImageUpdated(update);
  }
  else if (mouseButton == Qt::MidButton) {
    const int originalDimension = parent()->curImage_->originalDimension();
    parent()->requestSquareColor(originalX, originalY,
                                 originalDimension, oldColor);
  }
  else if (mouseButton == Qt::RightButton) {
    // make the clicked color the tool color
    parent()->toolDock_->setToolLabelColor(oldColor);
  }
}

void changeOneTool::activeMouseMove(QMouseEvent* event) {

  // update the dock color swatch if we're not dragging
  if (!dragCache_.cacheIsActive) {
    squareTool::activeMouseMove(event);
  }
  // note: event->button is always nobutton, so have to use buttons()
  if (event->buttons() & Qt::LeftButton) {
    const int labelWidth = dragCache_.labelWidth;
    const int labelHeight = dragCache_.labelHeight;
    const int originalImageWidth = dragCache_.imageWidth;
    const int originalImageHeight = dragCache_.imageHeight;
    const int x = event->x();
    const int y = event->y();
    // to speed things up, we'll only update and paint the label
    // image; we'll update the container image when the mouse is
    // released and then redraw everything from the container
    int originalX = (x*originalImageWidth)/labelWidth;
    int originalY = (y*originalImageHeight)/labelHeight;
    // convert originalX and originalY to box coordinates boxX, boxY
    const int squareDim = dragCache_.squareDim;
    const int boxX = originalX/squareDim;
    const int boxY = originalY/squareDim;
    // only process if we haven't already changed this square
    const pairOfInts boxCoordinates(boxX, boxY);
    if (!dragCache_.squaresVisited.contains(boxCoordinates)) {
      dragCache_.squaresVisited.insert(boxCoordinates);
      squareImageLabel* label = parent()->activeSquareLabel();
      label->addSquare(boxCoordinates);
      const int xstart = boxX*squareDim;
      const int ystart = boxY*squareDim;
      int sxstart, sxend, systart, syend;
      const QMatrix& matrix = dragCache_.matrix;
      matrix.map(xstart, ystart, &sxstart, &systart);
      matrix.map(xstart + squareDim, ystart + squareDim, &sxend, &syend);
      const int dx = sxend - sxstart;
      const int dy = syend - systart;
      label->update(x - dx, y - dy, 2*dx, 2*dy);
    }
  }
}

void detailTool::activeMouseMove(QMouseEvent* event) {

  // update the dock color swatch if we're not dragging
  if (!dragCache_.cacheIsActive) {
    squareTool::activeMouseMove(event);
  }

  if (event->buttons() & (Qt::LeftButton | Qt::RightButton)) {
    const int x = event->x();
    const int y = event->y();
    const int originalX = (x*dragCache_.imageWidth)/dragCache_.labelWidth;
    const int originalY = (y*dragCache_.imageHeight)/dragCache_.labelHeight;
    // convert originalX and originalY to box coordinates boxX, boxY
    const int squareDim = dragCache_.squareDim;
    const int boxX = originalX/squareDim;
    const int boxY = originalY/squareDim;
    processDetailEvent(boxX, boxY, event->buttons());
    const int d = parent()->roughCurDim();
    parent()->activeSquareLabel()->update(x-d, y-d, 2*d, 2*d);
  }
}

void changeOneTool::mouseRelease() {

  if (dragCache_.cacheIsActive) {
    // we just finished a drag event, so clean up
    parent()->activeSquareLabel()->clearSquares();
    const dockListUpdate update =
      parent()->curImage_->commitChangeOneDrag(dragCache_.squaresVisited,
                                               dragCache_.newColor);
    parent()->curImageUpdated(update);
    dragCache_.clear();
  }
}

void detailTool::mouseRelease() {

  if (dragCache_.cacheIsActive) {
    parent()->activeSquareLabel()->stopDrawingHashes();
  }
}

void detailTool::processDetailEvent(int boxX, int boxY,
                                    Qt::MouseButtons buttons) {

  const pairOfInts boxCoordinates(boxX, boxY);
  if (buttons & Qt::LeftButton) { // add
    if (!dragCache_.squaresVisited.contains(pixel(boxCoordinates))) {
      const int squareDim = parent()->curImage_->originalDimension();
      QRgb squareColor =
        parent()->curImage_->image().pixel(boxX*squareDim, boxY*squareDim);
      QRgb oppositeColor = triC(squareColor).opposite().qrgb();
      parent()->activeSquareLabel()->addHashSquare(pixel(oppositeColor,
                                                         pairOfInts(boxX,
                                                                    boxY)));
      dragCache_.squaresVisited.insert(pixel(squareColor, boxCoordinates));
    }
  }
  else if (buttons & Qt::RightButton) { // remove
    if (dragCache_.squaresVisited.remove(pixel(boxCoordinates))) {
      const int squareDim = parent()->curImage_->originalDimension();
      QRgb squareColor =
        parent()->curImage_->image().pixel(boxX*squareDim, boxY*squareDim);
      parent()->activeSquareLabel()->removeHashSquare(pixel(squareColor,
                                                            boxCoordinates));
      if (dragCache_.squaresVisited.isEmpty()) {
        parent()->toolDock_->detailListIsEmpty(true);
      }
    }
  }
}

void nullTool::setMouseHint() { parent()->setStatus(QString()); }

void changeAllTool::setMouseHint() {
  parent()->setStatus(QObject::tr("lb: change color under mouse; mb: change swatch color; rb: set swatch color"));
}

void changeOneTool::setMouseHint() {
  parent()->setStatus(QObject::tr("lb: paint with swatch color; mb: change swatch color; rb: set swatch color"));
}

void fillTool::setMouseHint() {
  parent()->setStatus(QObject::tr("lb: fill with swatch color; mb: change swatch color; rb: set swatch color"));
}

void detailTool::setMouseHint() {
  parent()->setStatus(QObject::tr("lb: add for detailing; rb: remove from detailing"));
}
