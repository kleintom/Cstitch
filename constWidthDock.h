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

#ifndef CONSTWIDTHDOCK_H
#define CONSTWIDTHDOCK_H

#include <QtWidgets/QWidget>
#include <QFontMetrics>
#include <QtWidgets/QStyle>

//
// constWidthDock serves as the base for dock widgets.  It enforces a
// constant fixed width (provided by dockWidth()). It also provides the
// swatch size for swatches showing the color under the mouse, as well
// as the icon size for color icons in color list widgets.
//
class constWidthDock : public QWidget {

 public:
  constWidthDock(QWidget* parent) : QWidget(parent) {

    QFont widgetFont = font();
    widgetFont.setFamily("monaco");
    widgetFont.setFixedPitch(true);
    const QFontMetrics metric(widgetFont);
    iconSize_ = QSize(metric.height() - 2, metric.height() - 2);
    // The width chosen here is based primarily on how many dmc color names will
    // fit without needing to horizontal scroll or check the tooltip.  Only 18%
    // of dmc names are longer than 16 characters, and the longest is 25
    // characters (which is just too wide), so this seems like a good
    // compromise.
    const int leftPadding = 20;
    const int rightPadding = 8;
    const int partsSpacing = 5;
    const int scrollBarWidth = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    width_ = leftPadding +
      /* icon */ partsSpacing + iconSize_.width() + partsSpacing +
      /* dmc text */ metric.width("1234567890123456") + partsSpacing +
      scrollBarWidth + rightPadding;
    setFixedWidth(width_);
  }

 protected:
  int dockWidth() const { return width_; }
  QSize swatchSize() const { return QSize(32, 32); }
  QSize iconSize() const { return iconSize_; }

 private:
  int width_;
  QSize iconSize_;
};

#endif
