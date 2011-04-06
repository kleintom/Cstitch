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

#include <QtGui/QWidget>
#include <QtGui/QCheckBox>

#include "squareDockTools.h"
#include "detailToolDock.h"

class mousePressLabel;
class triC;
class QGridLayout;

//
// squareToolDock is the dock widget for displaying the square editing tools; the widget
// also displays a color swatch (tool loabel color) for those tools that
// use a user selected color and a "dmc only" box that when checked means
// the user should only be allowed to choose and use dmc colors for tools.
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
class squareToolDock : public QWidget {

  Q_OBJECT

  enum toolChangeDirection {T_ACTIVATE, T_DEACTIVATE};

 public:
  explicit squareToolDock(QWidget* parent);
  QRgb getToolLabelColor() const;
  void setDmcOnly(bool b) { dmcBox_->setChecked(b); }
  bool dmcOnly() const { return dmcBox_->isChecked(); }
  void setToolToNoop();
  void detailListIsEmpty(bool b) { detailDock_->detailListIsEmpty(b); }

 public slots:
  // make the tool color swatch <color>
  void setToolLabelColor(QRgb color);

 private:
  // constructor helper: make the tool objects, using <iconSize> for icons
  void constructTools(const QSize& iconSize);
  bool event(QEvent* e);
  void showDetailDock(bool show);
  QSize sizeHint() const;
  // perform a tool activation or deactivation, as specified by <direction>
  // for the tool specified by <data>: (de)activate the tool's button,
  // signal the (re)moval of the tool's context action, (un)show the
  // tool color label, and (un)show the detail tool widget
  void processToolChange(toolChangeDirection direction,
                         const toolChangeData& data);

 private slots:
  // deactivate the old tool (curToolButton_) and activate the new tool
  // (<action>)
  void processToolChange(squareDockToolButton* button);
  // the user clicked on the color swatch - pop up a color choose dialog
  // for the user to choose a new color
  void processToolLabelMousePressed();
  // the user clicked the detail button, pass it on
  void processDetailCall(int numColors) const {
    emit detailCall(numColors);
  }
  // the user clicked the detail clear button, pass it on
  void processDetailClearCall() const {
    emit detailClearCall();
  }

 signals:
  void announceToolChanged(squareToolCode currentTool) const;
  void addContextAction(contextColorAction* action) const;
  void removeContextAction(contextColorAction* action) const;
  void detailCall(int numColors) const;
  void detailClearCall() const;
  void toolLabelColorRequested(const triC& currentColor, bool dmcOnly);

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

  // translate all chosen colors to DMC or no
  QCheckBox* dmcBox_;
};

#endif
