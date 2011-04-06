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

#ifndef COLORCHOOSERPROCESSMODES_H
#define COLORCHOOSERPROCESSMODES_H

#include <QtCore/QSharedData>
#include <QtCore/QVector>

#include "triC.h"

class triState;
class colorChooserProcessMode;
class QImage;
class QDomDocument;
class QDomElement;
template<class T> class QExplicitlySharedDataPointer;
class QString;
template<class T1, class T2> struct QPair;

typedef QPair<QString, QString> QStringPair;
typedef QExplicitlySharedDataPointer<colorChooserProcessMode> processModePtr;

// Used to indicate what needs updating when the colorChooser process
// is changed.
class processChange {
 public:
  // mouseTracking is if the image tracks mouse moves over it (for updating
  // the color swatch on the color list);
  // listRemoved is whether or not the user can right click on a color in
  // the color list to choose to remove it from the list;
  // <colors> are the colors that go on the color list for the new mode
  processChange(bool mouseTracking, bool numColorsBoxEnabled,
                bool listRemoveEnabled, const QString& dockTitle,
                const QVector<triC>& clickedColors,
                const QVector<triC>& generatedColors)
    : mouseTracking_(mouseTracking), numColorsBoxEnabled_(numColorsBoxEnabled),
    listRemoveEnabled_(listRemoveEnabled), dockTitle_(dockTitle),
    clickedColors_(clickedColors), generatedColors_(generatedColors) {}
  bool mouseTracking() const { return mouseTracking_; }
  bool numColorsBoxEnabled() const { return numColorsBoxEnabled_; }
  bool listRemoveEnabled() const { return listRemoveEnabled_; }
  QString dockTitle() const { return dockTitle_; }
  const QVector<triC>& clickedColors() const { return clickedColors_; }
  const QVector<triC>& generatedColors() const { return generatedColors_; }

 private:
  bool mouseTracking_;
  bool numColorsBoxEnabled_;
  bool listRemoveEnabled_;
  QString dockTitle_;
  const QVector<triC>& clickedColors_;
  const QVector<triC>& generatedColors_;
};

// colorChooserProcessMode represents a colorChooser processing mode
// and provides that mode's processing capabilities.  It is an abstract
// class derived by the actual process modes.
// This class maintains the color list for the mode.
class colorChooserProcessMode : public QSharedData {

 public:
  // input the color list for this mode
  colorChooserProcessMode(const QVector<triC>& colors = QVector<triC>())
   : clickedColors_(colors) {}
  const QVector<triC>& clickedColorList() const { return clickedColors_; }
  const QVector<triC>& generatedColorList() const {
    return generatedColors_;
  }
  virtual QVector<triC> colorList() const;
  void setClickedColorList(const QVector<triC>& colorList) {
    clickedColors_ = colorList;
  }
  void setGeneratedColorList(const QVector<triC>& colorList) {
    generatedColors_ = colorList;
  }
  // return true if the color list was cleared
  virtual bool resetColorList() {
    clickedColors_.clear();
    generatedColors_.clear();
    return true;
  }
  // returns the color that would be added if it isn't already on one
  // of the lists (note that in certain modes, the input color may be
  // transformed before being added to the list); <added> set to true if
  // the color was actually added
  virtual triC addColor(const triC& color, bool* added);
  // return true if the color list is empty after the remove
  bool removeColor(const triC& color);
  // return the updates needed for switching to this mode
  virtual processChange makeProcessChange() const = 0;
  // perform this mode's processing directly on <image>, using the mode's
  // color list and <numColors> (for those modes that need it) and
  // <numImageColors>, the number of colors in <image>
  // return triNoop if the user cancels processing, triTrue if the color
  // list was updated by completed processing, and triFalse if processing
  // completed but the color list doesn't need updating
  virtual triState performProcessing(QImage* image, int numColors,
                                     int numImageColors) = 0;
  virtual bool modeIsDmcOnly() const = 0;
  // return the mode description for this mode
  virtual QString modeText() const = 0;
  // return a status message for this mode
  virtual QString statusHint() const = 0;
  // return a tool tip for this mode
  virtual QString toolTip() const = 0;
  // return true if the colorChooser "number of colors to choose" box
  // should be enabled
  virtual bool numColorsBoxActive() const { return false; }
  virtual void appendColorList(QDomDocument* doc,
                               QDomElement* appendee) = 0;
  // process <originalImage> using <colors>
  void restoreSavedImage(QImage* originalImage,
                         const QVector<triC>& colors,
                         int numImageColors);

 private:
  // clicked colors are the ones the user chose from the image
  QVector<triC> clickedColors_;
  // generated colors are the ones we generated for the user
  QVector<triC> generatedColors_;
};

// a group of processing modes supporting the common process mode interface
// on whichever mode is current
class processModeGroup {

 public:
  processModeGroup();
  // return the mode strings the user sees for the modes in this group
  QList<QStringPair> modeStrings() const;
  void setNewMode(const QString& mode);
  // clear the color lists of all modes in this group
  void clearColorLists();
  bool modeIsDmcOnly() const { return curMode_->modeIsDmcOnly(); }
  //// methods below delegate to curMode_
  QString modeText() const { return curMode_->modeText(); }
  processChange makeProcessChange() const {
    return curMode_->makeProcessChange();
  }
  // defined in .cpp just so we don't need an include for triState
  triState performProcessing(QImage* image, int numColors,
                             int numImageColors);
  QString statusHint() const { return curMode_->statusHint(); }
  QVector<triC> colorList() const { return curMode_->colorList(); }
  const QVector<triC>& clickedColorList() const {
    return curMode_->clickedColorList();
  }
  triC addColor(const triC& color, bool* added) {
    return curMode_->addColor(color, added);
  }
  bool removeColor(const triC& color) {
    return curMode_->removeColor(color);
  }
  bool resetColorList() { return curMode_->resetColorList(); }
  bool numColorsBoxActive() const {
    return curMode_->numColorsBoxActive();
  }
  void restoreSavedImage(QImage* originalImage,
                         const QVector<triC>& colors,
                         int numImageColors) {
    curMode_->restoreSavedImage(originalImage, colors, numImageColors);
  }
  void appendColorLists(QDomDocument* doc, QDomElement* appendee) const;
  void setColorLists(const QDomElement& element);
  const QVector<triC>& generatedColorList() const {
    return curMode_->generatedColorList();
  }
  QString toolTip(const QString& modeText) const;
  bool userColorsExist() const {
    return !curMode_->clickedColorList().isEmpty();
  }

 private:
  // the current processing mode; non-null after construction, points to
  // one of the modes below (all derived from colorChooserProcessMode)
  // all of the processing and color list behavior is encoded in this
  // mode, so widget behavior is largely determined by its current value
  processModePtr curMode_;
  QList<processModePtr> activeModes_;
};

// base for num_colors and num_colors_to_dmc modes
class numColorsBaseModes : public colorChooserProcessMode {
 public:
  bool removeColor(const triC& ) { return true; }
  processChange makeProcessChange() const {
    return processChange(true, true, true, QObject::tr("Clicked colors"),
                         clickedColorList(), generatedColorList());
  }
  triState performProcessing(QImage* image, int numColors,
                             int numImageColors);
  QString statusHint() const {
    return QObject::tr("Select the number of colors to be chosen from the number box and/or click on a color on the image to add it");
  }
  bool processButtonActive() const { return true; }
  bool numColorsBoxActive() const { return true; }
  void appendColorList(QDomDocument* doc, QDomElement* appendee);
};

class numberOfColorsMode : public numColorsBaseModes {
 public:
  bool modeIsDmcOnly() const { return false; }
  QString modeText() const { return QObject::tr("Num Colors"); }
  QString toolTip() const {
    return QObject::tr("Click on colors and/or let the program pick a specified number of colors");
  }
};

class numberOfColorsToDmcMode : public numColorsBaseModes {
 public:
  triC addColor(const triC& color, bool* added);
  bool modeIsDmcOnly() const { return true; }
  QString modeText() const { return QObject::tr("Num Colors to DMC"); }
  QString toolTip() const {
    return QObject::tr("Click on colors and/or let the program pick a specified number of DMC colors");
  }
};

class dmcMode : public colorChooserProcessMode {
 public:
  dmcMode();
  QVector<triC> colorList() const { return generatedColorList(); }
  bool resetColorList() { return false; }
  bool removeColor(const triC& ) { return true; }
  processChange makeProcessChange() const {
    return processChange(false, false, false, QObject::tr("DMC colors"),
                         clickedColorList(), generatedColorList());
  }
  triState performProcessing(QImage* image, int numColors,
                             int numImageColors);
  bool modeIsDmcOnly() const { return true; }
  QString modeText() const { return QObject::tr("DMC"); }
  QString statusHint() const {
    return QObject::tr("Colors will be chosen from the displayed list of DMC colors");
  }
  QString toolTip() const {
    return QObject::tr("Let the program choose colors from the DMC color list");
  }
  void appendColorList(QDomDocument* , QDomElement* ) { return; }
};

#endif
