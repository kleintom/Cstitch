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

#include "squareToolDock.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QStyle>
#include <QtWidgets/QComboBox>

#include "colorLists.h"
#include "mousePressLabel.h"
#include "imageProcessing.h"
#include "squareDockTools.h"
#include "detailToolDock.h"

squareToolDock::squareToolDock(QWidget* parent)
  : constWidthDock(parent), detailDock_(NULL) {

  dockLayout_ = new QVBoxLayout(this);
  setLayout(dockLayout_);

  toolLayout_ = new QGridLayout();

  // toolLayout_ uses smaller icons by default
  // apparently every button has to be set individually...
  const int iconDimension = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
  const QSize iconSize(iconDimension, iconDimension);
  constructTools(iconSize);

  toolColorLabel_ = new mousePressLabel(this);
  toolColorLabel_->setFixedSize(swatchSize());
  toolColorLabel_->setFrameStyle(QFrame::Panel | QFrame::Raised);
  toolColorLabel_->setLineWidth(3);
  toolColorLabel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  toolColorLabel_->setAlignment(Qt::AlignCenter);
  QPixmap qpm = QPixmap(swatchSize());
  qpm.fill(Qt::black);
  toolColorLabel_->setPixmap(qpm);
  toolColorLabel_->setEnabled(false);
  connect(toolColorLabel_, SIGNAL(mousePressed()),
          this, SLOT(processToolLabelMousePressed()));

  toolLabelLayout_ = new QHBoxLayout();
  dockLayout_->addLayout(toolLabelLayout_);
  toolLabelLayout_->addWidget(toolColorLabel_);

  dockLayout_->addLayout(toolLayout_);

  flossTypeBox_ = new QComboBox(this);
  connect(flossTypeBox_, SIGNAL(currentIndexChanged(int )),
          this, SLOT(processFlossTypeBoxChange(int )));
  const QList<flossType> types = flossType::flossTypes();
  for (int i = 0, size = types.size(); i < size; ++i) {
    flossTypeBox_->addItem(types[i].text(),
                           QVariant::fromValue(types[i]));
  }
  dockLayout_->addWidget(flossTypeBox_);

  setMinimumSize(sizeHint());
  setMaximumSize(sizeHint());
}

void squareToolDock::constructTools(const QSize& iconSize) {

  // noop mouse pointer (default mouse/window behavior)
  noopButton_ = new noopToolButton(this, tr("Select"), QIcon(":cursor.png"),
                                   iconSize);
  connect(noopButton_, SIGNAL(toolButtonClicked(squareDockToolButton* )),
          this, SLOT(processToolChange(squareDockToolButton*)));
  noopButton_->setDown(true);

  // change all (all pixels with the same color that is)
  changeAllButton_ = new changeAllToolButton(this, tr("Change all"),
                                             QIcon(":changeAll.png"),
                                             iconSize);
  connect(changeAllButton_, SIGNAL(toolButtonClicked(squareDockToolButton* )),
          this, SLOT(processToolChange(squareDockToolButton*)));
  connect(changeAllButton_, SIGNAL(setToolLabelColor(QRgb)),
          this, SLOT(setToolLabelColor(QRgb)));


  // change one color (one pixel that is)
  changeOneButton_ = new changeOneToolButton(this, tr("Change one"),
                                             QIcon(":draw.png"), iconSize);
  connect(changeOneButton_, SIGNAL(toolButtonClicked(squareDockToolButton* )),
          this, SLOT(processToolChange(squareDockToolButton*)));
  connect(changeOneButton_, SIGNAL(setToolLabelColor(QRgb)),
          this, SLOT(setToolLabelColor(QRgb)));

  // fill region
  fillRegionButton_ = new fillToolButton(this, tr("Fill region"),
                                         QIcon(":fill.png"), iconSize);
  connect(fillRegionButton_, SIGNAL(toolButtonClicked(squareDockToolButton* )),
          this, SLOT(processToolChange(squareDockToolButton*)));
  connect(fillRegionButton_, SIGNAL(setToolLabelColor(QRgb)),
          this, SLOT(setToolLabelColor(QRgb)));

  // detail squares
  detailButton_ = new detailToolButton(this, tr("Detail squares"),
                                       QIcon(":detail.png"), iconSize);
  connect(detailButton_, SIGNAL(toolButtonClicked(squareDockToolButton* )),
          this, SLOT(processToolChange(squareDockToolButton*)));

  toolLayout_->addWidget(noopButton_, 1, 1);
  toolLayout_->addWidget(changeAllButton_, 1, 2);
  toolLayout_->addWidget(changeOneButton_, 1, 3);
  toolLayout_->addWidget(fillRegionButton_, 1, 4);
  toolLayout_->addWidget(detailButton_, 1, 5);

  curToolButton_ = noopButton_;
}

void squareToolDock::processToolChange(squareDockToolButton* button) {

  if (curToolButton_ != button) {
    const toolChangeData deactivationData = curToolButton_->deactivate();
    processToolChange(T_DEACTIVATE, deactivationData);
    const toolChangeData activationData = button->activate();
    processToolChange(T_ACTIVATE, activationData);
    curToolButton_ = button;
    emit announceToolChanged(curToolButton_->toolIndex());
  }
  else {
    // the button toggles up/down each time it gets pressed, so undo that
    curToolButton_->setDown(true);
  }
}

void squareToolDock::processToolChange(toolChangeDirection direction,
                                       const toolChangeData& data) {
  if (direction == T_ACTIVATE) {
    data.toolButton()->setDown(true);
    if (data.contextAction()) {
      emit addContextAction(data.contextAction());
    }
  }
  else {
    data.toolButton()->setDown(false);
    if (data.contextAction()) {
      emit removeContextAction(data.contextAction());
    }
  }
  if (data.enableToolLabel() != triNoop) {
    const bool b = (data.enableToolLabel() == triTrue) ? true : false;
    toolColorLabel_->setEnabled(b);
  }
  if (data.showDetailDock() != triNoop) {
    const bool b = (data.showDetailDock() == triTrue) ? true : false;
    showDetailDock(b);
  }
}

void squareToolDock::processToolLabelMousePressed() {

  emit toolLabelColorRequested(getToolLabelColor().color(), getFlossType());
}

flossColor squareToolDock::getToolLabelColor() const {

  const QRgb color = toolColorLabel_->pixmap()->toImage().pixel(1, 1);
  return flossColor(color, getFlossType());
}

void squareToolDock::setToolLabelColor(const flossColor& color) {

  fillToolColorSwatch(color.qrgb());
}

void squareToolDock::setToolLabelColor(QRgb color) {

  fillToolColorSwatch(color);
}

void squareToolDock::fillToolColorSwatch(QRgb color) {

  QPixmap newLabel = QPixmap(swatchSize());
  newLabel.fill(::transformColor(color, getFlossType()).qc());
  toolColorLabel_->setPixmap(newLabel);
}

bool squareToolDock::event(QEvent* e) {

  // handle a toolDock widget enabled event
  if (e->type() == QEvent::EnabledChange && isEnabled()) {
    toolChangeData toolData = curToolButton_->activate();
    toolData.toolButton()->setDown(true);
    return true;
  }
  return QWidget::event(e);
}  

void squareToolDock::showDetailDock(bool show) {

  if (show) {
    if (detailDock_ == NULL) {
      detailDock_ = new detailToolDock(this);
      connect(detailDock_, SIGNAL(detailCalled(int)),
              this, SLOT(processDetailCall(int)));
      connect(detailDock_, SIGNAL(clearCalled()),
              this, SLOT(processDetailClearCall()));
      dockLayout_->addWidget(detailDock_);
    }
    detailDock_->detailListIsEmpty(true);
    detailDock_->show();
    setFixedSize(sizeHint());
  }
  else if (detailDock_) {
    detailDock_->hide();
    setFixedSize(sizeHint());
  }
}

QSize squareToolDock::sizeHint() const {

  const QFontMetrics fontMetric(font());
  int height = swatchSize().height() +
    style()->pixelMetric(QStyle::PM_ToolBarIconSize) +
    fontMetric.boundingRect("D").height() + 40;
  if (detailDock_ && detailDock_->isVisible()) {
    height += detailDock_->height();
  }
  return QSize(dockWidth(), height);
}

void squareToolDock::setToolToNoop() {

  processToolChange(noopButton_);
}

void squareToolDock::setFlossType(flossType type) {

  // make sure the current swatch color is valid for <type>
  const flossType oldSwatchType = getFlossType();
  if (!(oldSwatchType <= type)) { // incompatible types
    setToolLabelColor(Qt::black);
  }
  flossTypeBox_->setCurrentIndex(flossTypeBox_->findText(type.text()));
}

flossType squareToolDock::getFlossType() const {

  return flossTypeBox_->itemData(flossTypeBox_->currentIndex()).value<flossType>();
}

void squareToolDock::processFlossTypeBoxChange(int ) {

  const flossType newFlossType = getFlossType();
  if (newFlossType != flossVariable) {
    // old and new floss types are probably incompatible (we can't know
    // for sure unless we save the old floss type, which we currently don't)
    setToolLabelColor(Qt::black);
  }
  emit toolFlossTypeChanged(newFlossType);
}
