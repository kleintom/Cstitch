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

#include "squareDockTools.h"

#include "squareToolDock.h"
#include "mousePressLabel.h"

squareDockToolButton::squareDockToolButton(QWidget* parent,
                                           const QString& toolTip,
                                           const QIcon& icon,
                                           const QSize& iconSize)
  : QToolButton(parent) {
  setToolTip(toolTip);
  setIcon(icon);
  setIconSize(iconSize);
  setDown(false);
  setAutoRaise(true);
  connect(this, SIGNAL(clicked()), this, SLOT(buttonClicked()));
}

changeAllToolButton::changeAllToolButton(QWidget* parent,
                                         const QString& toolTip,
                                         const QIcon& icon,
                                         const QSize& iconSize)
  : squareDockToolButton(parent, toolTip, icon, iconSize) {

  changeAllColorContextAction_ =
    new changeAllAction(tr("Set as tool color"), this);
  connect(changeAllColorContextAction_,
          SIGNAL(changeAllColorContextTrigger(const QColor& )),
          this, SLOT(processChangeAllContextAction(const QColor& )));
}

changeOneToolButton::changeOneToolButton(QWidget* parent,
                                         const QString& toolTip,
                                         const QIcon& icon,
                                         const QSize& iconSize)
  : squareDockToolButton(parent, toolTip, icon, iconSize) {

  changeOneColorContextAction_ =
    new changeOneAction(tr("Set as tool color"), this);
  connect(changeOneColorContextAction_,
          SIGNAL(changeOneColorContextTrigger(const QColor& )),
          this, SLOT(processChangeOneContextAction(const QColor& )));
}

fillToolButton::fillToolButton(QWidget* parent, const QString& toolTip,
                               const QIcon& icon, const QSize& iconSize)
  : squareDockToolButton(parent, toolTip, icon, iconSize) {

  fillRegionContextAction_ =
    new fillRegionAction(tr("Set as tool color"), this);
  connect(fillRegionContextAction_,
          SIGNAL(fillRegionContextTrigger(const QColor& )),
          this, SLOT(processFillRegionContextAction(const QColor& )));
}

detailToolButton::detailToolButton(QWidget* parent, const QString& toolTip,
                                   const QIcon& icon, const QSize& iconSize)
  : squareDockToolButton(parent, toolTip, icon, iconSize) { }

toolChangeData noopToolButton::activate() {

  return toolChangeData(this, triFalse, NULL, triNoop);
}

toolChangeData changeAllToolButton::activate() {

  return toolChangeData(this, triTrue,
                        changeAllColorContextAction_, triNoop);
}

toolChangeData changeOneToolButton::activate() {

  return toolChangeData(this, triTrue,
                        changeOneColorContextAction_, triNoop);
}

toolChangeData fillToolButton::activate() {

  return toolChangeData(this, triTrue, fillRegionContextAction_,
                        triNoop);
}

toolChangeData detailToolButton::activate() {

  return toolChangeData(this, triFalse, NULL, triTrue);
}


toolChangeData noopToolButton::deactivate() {

  return toolChangeData(this, triNoop, NULL, triNoop);
}

toolChangeData changeAllToolButton::deactivate() {

  return toolChangeData(this, triNoop,
                        changeAllColorContextAction_, triNoop);
}

toolChangeData changeOneToolButton::deactivate() {

  return toolChangeData(this, triNoop,
                        changeOneColorContextAction_, triNoop);
}

toolChangeData fillToolButton::deactivate() {

  return toolChangeData(this, triNoop,
                        fillRegionContextAction_, triNoop);
}

toolChangeData detailToolButton::deactivate() {

  return toolChangeData(this, triNoop, NULL, triFalse);
}

void changeAllToolButton::processChangeAllContextAction(const QColor& color) {

  emit setToolLabelColor(color.rgb());
}

void changeOneToolButton::processChangeOneContextAction(const QColor& color) {

  emit setToolLabelColor(color.rgb());
}

void fillToolButton::processFillRegionContextAction(const QColor& color) {

  emit setToolLabelColor(color.rgb());
}
