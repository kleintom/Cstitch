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

#ifndef COLORCHOOSER_H
#define COLORCHOOSER_H

#include "imageZoomWindow.h"
#include "colorChooserProcessModes.h"

class imageLabel;
class dockListWidget;
class dockListSwatchWidget;
class windowManager;
class triC;
class colorCompareSaver;
class helpMode;
class QDomElement;
class QScrollArea;
class QPushButton;
class QComboBox;
class QSpinBox;

// class colorChooser
//
// colorChooser is the first of the four main application windows
// (see main.h for more information on the main windows).
// This is the first window the user sees on application startup; its
// primary purpose is to allow the user to choose and load an image from
// the filesystem (in any of the image formats supported by Qt) and then
// to choose the colors they wish to use to turn the image into a pattern.
// The user can load an image at any time, but a new image will delete
// all work on the previous image application-wide.
//
// There are five color selection modes for the user to choose from.
//
// DMC mode will create a new image by replacing each color in
// the original image with the closest matching DMC color.  This
// program is aware of 428 dmc color names, so depending on
// the variety of colors in theoriginal image, this mode will create a
// new image with anywhere from 1 to 428 colors.
//
// Num colors mode lets the user choose the number of colors they would
// like the final pattern to have (note: later processing may reduce
// the number of colors).  The actual colors are chosen based on the
// frequency with which colors in the original image appear.
//
// Num colors to DMC mode is similar to Num colors mode, except that the
// colors chosen are DMC colors and so may not actually be colors in the
// original image.
//
// Color List mode allows the user to choose colors by clicking on colors
// in the image.
//
// Color List to DMC mode works the same as the Color List mode,
// except that each clicked color from the original image is transformed
// to the closest matching DMC color.
//
// Once the user is satisfied with their color mode and selection they
// click a button to move to the next stage, the colorCompareWindow.

class colorChooser : public imageZoomWindow {

  Q_OBJECT

 public:
  explicit colorChooser(windowManager* winMgr);
  // reset colorChooser to use the new <image>
  void setNewImage(const QImage& image);
  // recreate a colorCompare image using the data in <saver> as part of a
  // project restore
  int recreateImage(const colorCompareSaver& saver);
  // append "global" xml settings to <appendee> for this widget
  void appendCurrentSettings(QDomDocument* doc, QDomElement* appendee) const;
  // restore "global" settings for this widget from <xml>
  void updateCurrentSettings(const QDomElement& xml);

 private:
  // constructor helper
  void constructMenuObjects();
  // constructor helper - construct and display widgets supporting process
  // mode selection
  void constructProcessingObjects();
  // In non-active state only the open file action is enabled
  void setWidgetActive(bool active);
  // Create and show the color list dock
  void popDock();
  // zoom in or out on the image by the given pixel amount
  void zoom(int zoomIncrement);
  // reset the image to its original size
  void originalSize();
  // set the widget's image
  void setLabelImage(const QImage& image);
  void setModeBox(const QString& mode);
  // return the helpMode enum value for this mode
  helpMode getHelpMode() const;

 private slots:
  // process a "Process" button: create a new image based on the current
  // processing mode and (if applicable) the current color list.  Pass
  // the result to windowManager for display in the colorCompareWindow
  void processProcessing();
  // Use the input mouseEvent to retrieve the image color at the
  // coordinates of the mouse event and send it to the current
  // processing mode for possible addition to the image list
  void processColorAdd(QMouseEvent* event);
  // Process the user's new mode selection by updating the widget
  // accordingly.
  // boxIndex is the new index of processModeBox_.
  void processProcessChange(int boxIndex);
  // update the color swatch in the dock list to reflect the color
  // currently under the mouse
  void processMouseMove(QMouseEvent* event);
  // remove the given color from the color list for the current mode;
  void removeColor(const triC& color);
  // clear the color list for the current mode
  void clearList();
  // pop a dialog with useful information on the current image
  void displayImageInfo();
  void zoomToWidth();
  void zoomToHeight();
  void zoomToImage();

 private:
  processModeGroup processMode_;

  // allow the user to clear the current color list
  QAction* clearListAction_;

  // the image's scroll area
  QScrollArea* imageScroll_;
  // the image
  imageLabel* imageLabel_;
  // the color list dock widget
  dockListSwatchWidget* clickedDock_;
  QDockWidget* generatedDockHolder_;
  dockListWidget* generatedDock_;

  // clicked to proceed to create a new image and proceed to colorCompare
  QPushButton* processButton_;
  // the list of processing modes for the user to choose from
  QComboBox* processModeBox_;
  // some modes let the user choose how many colors they want to use
  QSpinBox* numColorsBox_;
};

#endif
