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

#ifndef SQUARETOOLDOCK_H
#define SQUARETOOLDOCK_H

#include "squareDockTools.h"
#include "detailToolDock.h"
#include "constWidthDock.h"
// it seems signal parameter types can't be forward declared, although I
// can't find that documented anywhere
#include "floss.h"

class mousePressLabel;
class triC;
class QGridLayout;
class QComboBox;

//
// squareToolDock is the dock widget for displaying the square editing
// tools; the widget also displays a color swatch (tool loabel color)
// for those tools that use a user selected color and a "dmc only" box
// that when checked means the user should only be allowed to choose
// and use dmc colors for tools.
//
//// Implementation notes
//
// The tools are represented by objects derived from squareDockTool and
// are stored in their corresponding QAction.
// The detail tool has its own sub widget that displays the extra info
// required by the detail tool.
// Some tools have context actions connected with the color list that get
// added/removed when the tool is selected/deselected (achieved by passing
// signals back and forth)
//
class squareToolDock : public constWidthDock {

  Q_OBJECT

  enum toolChangeDirection {T_ACTIVATE, T_DEACTIVATE};

 public:
  explicit squareToolDock(QWidget* parent);
  // Set the tool swatch to flossColor <color> if <color>'s floss type is
  // at most as permissive as getFlossType()'s, otherwise transform
  // <color> to getFlossType()'s type and set the swatch to that color.
  void setToolLabelColor(const flossColor& color);
  flossColor getToolLabelColor() const;
  // Set the floss type in the floss type combo box.
  void setFlossType(flossType type);
  // Get the floss type in the floss type combo box.
  flossType getFlossType() const;
  void setToolToNoop();
  void detailListIsEmpty(bool b) { detailDock_->detailListIsEmpty(b); }

 public slots:
  // Make the tool color swatch <color> (transformed to be of color type
  // getFlossType()).
  void setToolLabelColor(QRgb color);

 private:
  // Constructor helper: make the tool objects, using <iconSize> for icons.
  void constructTools(const QSize& iconSize);
  // Fill the color swatch with <color> transformed to be of color type
  // getFlossType().
  void fillToolColorSwatch(QRgb color);
  bool event(QEvent* e);
  void showDetailDock(bool show);
  QSize sizeHint() const;
  // Perform a tool activation or deactivation, as specified by <direction>
  // for the tool specified by <data>: (de)activate the tool's button,
  // signal the (re)moval of the tool's context action, (un)show the
  // tool color label, and (un)show the detail tool widget.
  void processToolChange(toolChangeDirection direction,
                         const toolChangeData& data);

 private slots:
  // Deactivate the old tool (curToolButton_) and activate the new tool
  // determined by <button>.
  void processToolChange(squareDockToolButton* button);
  // The user clicked on the color swatch, pass it on.
  void processToolLabelMousePressed();
  // The user clicked the detail button, pass it on.
  void processDetailCall(int numColors) const {
    emit detailCall(numColors);
  }
  // The user clicked the detail clear button, pass it on.
  void processDetailClearCall() const {
    emit detailClearCall();
  }
  // The user changed the tool floss type, pass it on and update the
  // tool swatch flossColor if necessary.
  void processFlossTypeBoxChange(int index);

 signals:
  void announceToolChanged(squareToolCode currentTool) const;
  void addContextAction(contextColorAction* action) const;
  void removeContextAction(contextColorAction* action) const;
  void detailCall(int numColors) const;
  void detailClearCall() const;
  void toolLabelColorRequested(const triC& currentColor, flossType type);
  void toolFlossTypeChanged(flossType type) const;

 private:
  QVBoxLayout* dockLayout_;
  QGridLayout* toolLayout_;

  // tool buttons
  mousePressLabel* toolColorLabel_; // shared by all tools that use it
  QHBoxLayout* toolLabelLayout_;

  // the currently depressed tool button
  squareDockToolButton* curToolButton_;
  squareDockToolButton* noopButton_;
  squareDockToolButton* changeAllButton_;
  squareDockToolButton* changeOneButton_;
  squareDockToolButton* fillRegionButton_;
  squareDockToolButton* detailButton_;

  // the settings widget for the detail tool
  detailToolDock* detailDock_;

  // determines what type of colors can be set
  QComboBox* flossTypeBox_;
};

#endif
