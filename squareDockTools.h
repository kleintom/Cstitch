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

#ifndef SQUAREDOCKTOOLS_H
#define SQUAREDOCKTOOLS_H

#include <QtGui/QAction>
#include <QtGui/QToolButton>

#include "utility.h"

class squareToolDock;
class triState;

enum squareToolCode {T_NOOP, T_CHANGE_ALL, T_CHANGE_ONE, T_FILL_REGION,
                     T_DETAIL};

// an action activated by calling trigger: users must implement trigger
// and should emit a (unique to the derived class) signal with the given
// color from trigger
class contextColorAction : public QAction {
 public:
  contextColorAction(const QString& actionName, QObject* parent)
    : QAction(actionName, parent) {}
  contextColorAction(const QIcon& icon, const QString& actionName,
                    QObject* parent)
    : QAction(icon, actionName, parent) {}
  virtual void trigger(const QColor& color) const = 0;
};

class changeAllAction : public contextColorAction {

  Q_OBJECT

 public:
  changeAllAction(const QString& actionName, QObject* parent)
    : contextColorAction(actionName, parent) { }
  void trigger(const QColor& color) const {
    emit changeAllColorContextTrigger(color);
  }
 signals:
  void changeAllColorContextTrigger(const QColor& color) const;
};

class changeOneAction : public contextColorAction {

  Q_OBJECT

 public:
  changeOneAction(const QString& actionName, QObject* parent)
    : contextColorAction(actionName, parent) { }
  void trigger(const QColor& color) const {
    emit changeOneColorContextTrigger(color);
  }
 signals:
  void changeOneColorContextTrigger(const QColor& color) const;
};

class fillRegionAction : public contextColorAction {

  Q_OBJECT

 public:
  fillRegionAction(const QString& actionName, QObject* parent)
    : contextColorAction(actionName, parent) { }
  void trigger(const QColor& color) const {
    emit fillRegionContextTrigger(color);
  }
 signals:
  void fillRegionContextTrigger(const QColor& color) const;
};

// data providing information on how to update widgets after a given
// tool change
class toolChangeData {
 public:
  toolChangeData(QToolButton* toolButton, triState enableToolLabel,
                 contextColorAction* contextAction,
                 triState showDetailDock)
  : toolButton_(toolButton), enableToolLabel_(enableToolLabel),
    contextAction_(contextAction), showDetailDock_(showDetailDock) {}
  QToolButton* toolButton() const { return toolButton_; }
  triState enableToolLabel() const { return enableToolLabel_; }
  contextColorAction* contextAction() const { return contextAction_; }
  triState showDetailDock() const { return showDetailDock_; }
 private:
  QToolButton* toolButton_;
  triState enableToolLabel_;
  contextColorAction* contextAction_;
  triState showDetailDock_;
};

//// Tools
// squareDockToolButton provides the abstract interface for a dock tool
// button
class squareDockToolButton : public QToolButton {

  Q_OBJECT

 public:
  squareDockToolButton(QWidget* parent, const QString& toolTip,
                       const QIcon& icon, const QSize& iconSize);
  // returns how to update when this tool is activated
  virtual toolChangeData activate() = 0;
  // returns how to update when this tool is deactivated
  virtual toolChangeData deactivate() = 0;
  // return this tool's id
  virtual squareToolCode toolIndex() const = 0;
 private slots:
  void buttonClicked() {
    emit toolButtonClicked(this);
  }
 signals:
  void toolButtonClicked(squareDockToolButton* clickedTool);
};
// make squareDockTool* known to QVariant
Q_DECLARE_METATYPE(squareDockToolButton*);

// the noop tool just provides the standard mouse behavior
class noopToolButton : public squareDockToolButton {
 public:
  noopToolButton(QWidget* parent, const QString& toolTip,
                 const QIcon& icon, const QSize& iconSize)
    : squareDockToolButton(parent, toolTip, icon, iconSize) {}
  toolChangeData activate();
  toolChangeData deactivate();
  squareToolCode toolIndex() const { return T_NOOP; }
};

class changeAllToolButton : public squareDockToolButton {

  Q_OBJECT

 public:
  changeAllToolButton(QWidget* parent, const QString& toolTip,
                      const QIcon& icon, const QSize& iconSize);
  toolChangeData activate();
  toolChangeData deactivate();
  squareToolCode toolIndex() const { return T_CHANGE_ALL; }

 private slots:
  void processChangeAllContextAction(const QColor& color);

 signals:
  void setToolLabelColor(QRgb color);

 private:
  contextColorAction* changeAllColorContextAction_;
};

class changeOneToolButton : public squareDockToolButton {

  Q_OBJECT

 public:
  changeOneToolButton(QWidget* parent, const QString& toolTip,
                      const QIcon& icon, const QSize& iconSize);
  toolChangeData activate();
  toolChangeData deactivate();
  squareToolCode toolIndex() const { return T_CHANGE_ONE; }

 private slots:
  void processChangeOneContextAction(const QColor& color);

 signals:
  void setToolLabelColor(QRgb color);

 private:
  contextColorAction* changeOneColorContextAction_;
};

class fillToolButton : public squareDockToolButton {

  Q_OBJECT

 public:
  fillToolButton(QWidget* parent, const QString& toolTip,
                 const QIcon& icon, const QSize& iconSize);
  toolChangeData activate();
  toolChangeData deactivate();
  squareToolCode toolIndex() const { return T_FILL_REGION; }

 private slots:
  void processFillRegionContextAction(const QColor& color);

 signals:
  void setToolLabelColor(QRgb color);

 private:
  contextColorAction* fillRegionContextAction_;
};

class detailToolButton : public squareDockToolButton {
 public:
  detailToolButton(QWidget* parent, const QString& toolTip,
                   const QIcon& icon, const QSize& iconSize);
  toolChangeData activate();
  toolChangeData deactivate();
  squareToolCode toolIndex() const { return T_DETAIL; }
};

#endif
