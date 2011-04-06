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

#include "utility.h"

#include <QtCore/qmath.h>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtGui/QImageWriter>

extern const int PROGRESS_X_COORDINATE;
extern const int PROGRESS_Y_COORDINATE;

groupProgressDialog* altMeter::groupDialog_ = NULL;

QString getNewImageFileName(QWidget* activeWindow, bool displayWarning) {

  int returnCode = QMessageBox::Ok;
  if (displayWarning) {
    returnCode = QMessageBox::warning(activeWindow, QObject::tr("Stitch"),
                                      "All previous work will be lost if you continue.",
                                      QMessageBox::Cancel | QMessageBox::Ok,
                                      QMessageBox::Ok);
  }
  if (returnCode == QMessageBox::Ok) {
    const QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    QString extensions;
    for (QList<QByteArray>::const_iterator it = formats.begin(),
           end = formats.end(); it != end; ++it) {
      extensions += QString("*.") + (*it).data() + QString(" ");
    }
    return QFileDialog::getOpenFileName(activeWindow, QObject::tr("Open image"), ".",
                                        "Image files (" + extensions +
                                        ")\nAll (*.*)");
  }
  return QString();
}

QString readTextFile(const QString& fileName) {

  QFile file(fileName);
  // using QIODevice::Text turns all newlines into \n
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream stream(&file);
    const QString fileString(stream.readAll());
    file.close();
    return fileString;
  }
  return QString();
}

groupProgressDialog::groupProgressDialog(int numDialogs)
  : QProgressDialog("", QString(), 0, 100),
    count_(0), numDialogs_(numDialogs) {

  setMinimumDuration(0);
  setWindowModality(Qt::WindowModal);
  setAutoClose(false);
  setAutoReset(false);
  move(PROGRESS_X_COORDINATE, PROGRESS_Y_COORDINATE);
  setWindowTitle("Progress");
}

altMeter::altMeter(const QString& labelText, const QString& cancelButtonText,
                   int minimum, int maximum) {

  if (!groupDialog_) {
    dialog_ = new QProgressDialog;
    dialog_->setLabelText(labelText);
    dialog_->setCancelButtonText(cancelButtonText);
    dialog_->setRange(minimum, maximum);
    dialog_->move(PROGRESS_X_COORDINATE, PROGRESS_Y_COORDINATE);
    dialog_->setWindowTitle("Progress");
    dialog_->setWindowModality(Qt::WindowModal);
  }
  else {
    groupDialog_->reset(minimum, maximum);
    dialog_ = groupDialog_;
  }
}

int averageCharacterWidth(const QFont& font) {

  const QFontMetrics metrics(font);
  int totalWidth = 0;
  for (int i = 32; i < 128; ++i) {
    totalWidth += metrics.width(QChar(i));
  }
  return qCeil(totalWidth/97.);
}
