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

#include "imageSaverWindow.h"

#include <QtWidgets/QMenu>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtPrintSupport/QPrinter>
#include <QPainter>
#include <QImageWriter>
#include <QImageReader>

#include "sliderSpinBoxDialog.h"
#include "utility.h"
#include "windowManager.h"

imageSaverWindow::imageSaverWindow(const QString& dockName,
                                   windowManager* winMgr)
  : imageZoomWindow(dockName, winMgr) {

  saveImageAction_ = new QAction(tr("Save Image"), this);
  saveImageAction_->setShortcut(QKeySequence("Alt+shift+s"));
  saveImageAction_->setIcon(QIcon(":saveImage.png"));
  connect(saveImageAction_, SIGNAL(triggered()),
          this, SLOT(processSaveImage()));

  fileMenu()->insertAction(quitAction(), saveImageAction_);
  addToolbarAction(saveImageAction_);
}

QString imageSaverWindow::getValidSaveFile(const QList<QByteArray>& formats) {

  QString extensions;
  QString warningExtensions;
  for (QList<QByteArray>::const_iterator it = formats.begin(),
         end = formats.end(); it != end; ++it) {
    extensions += QString("*.") + (*it).data() + QString(" ");
    warningExtensions += (*it).data() + QString(", ");
  }
  extensions.chop(1); // remove the last space
  extensions += "\nAll (*.*)";
  warningExtensions.chop(2);

  QString returnString;
  while (true) { // get a filename with a valid extension
    returnString =
      QFileDialog::getSaveFileName(this, tr("Save image"), ".", extensions);
    // return if the user canceled or the extension is valid
    if (returnString.isEmpty() ||
        formats.contains(::extension(returnString).toLatin1())) {
      return returnString;
    }
    QMessageBox messageBox;
    messageBox.setText("Please use one of the following filename extensions: " +
                       warningExtensions);
    messageBox.setIcon(QMessageBox::Warning);
    messageBox.exec();
  }
}

void imageSaverWindow::doPdfSave(const QString& outputFile,
                                 const QImage& image) {

  QPrinter printer;
  printer.setOutputFormat(QPrinter::PdfFormat);  // actually default
  printer.setOutputFileName(outputFile);
  const int xMargins =
    printer.paperRect().width() - printer.pageRect().width();
  const int yMargins =
    printer.paperRect().height() - printer.pageRect().height();
  printer.setPaperSize(QSizeF(image.width() + xMargins,
                              image.height() + yMargins),
                       QPrinter::DevicePixel);
  QPainter painter(&printer);
  painter.drawImage(QPoint(0, 0), image);
  painter.end(); // this does the qprinter write
}

void imageSaverWindow::doNonPdfSave(const QString& outputFile,
                                    const QImage& image) {

  QImageWriter writer(outputFile);
  // ask the user for the compression factor if the image format supports it
  if (writer.supportsOption(QImageIOHandler::Quality)) {
    int quality = -1;
    if (::extension(outputFile) != "png") {
      quality =
        sliderSpinBoxDialog::getSliderSpinBoxValue(tr("Set image quality:"),
                                                   tr("Image quality"),
                                                   0, 100, this);
    }
    else {
      // for png, quality means compression, and 100 means least compressed
      quality =
        sliderSpinBoxDialog::getSliderSpinBoxValue(tr("Set image compression (100 for most compressed):"),
                                                   tr("Image compression"),
                                                   0, 100, this);
      quality = -quality + 100;
    }
    if (quality >= 0 && quality <= 100) {
      writer.setQuality(quality);
      writer.write(image);
    }
    else if (quality == -1) { // user hit cancel
      return;
    }
    else {
      qWarning() << "Bad quality in nonPdfSave:" << quality << outputFile;
    }
  }
  else {
    writer.write(image);
  }
}

void imageSaverWindow::processSaveImage() {

  //  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  QList<QByteArray> formats = QImageReader::supportedImageFormats();
  formats.insert(formats.begin(), "pdf");
  const QString outputFile = getValidSaveFile(formats);
  if (outputFile.isEmpty()) {
    return;
  }

  const QSize curViewingSize = curImageViewSize();
  // 6000x6000 is "big"
  // (36,000,000 pixels * 4 bytes/pixel = 144,000,000 bytes;
  //  1048576 bytes/mb -> 137 mb)
  const qint64 bytes = static_cast<qint64>(curViewingSize.width()) *
    static_cast<qint64>(curViewingSize.height() * 4);
  if (bytes > 36000000) {
    const int megabytes = (bytes/1048576) + 1;
    QString size = ::itoqs(megabytes) + " megabytes";
    const QString warning(tr("The image you are requesting to save is ") +
                          ::itoqs(curViewingSize.width()) + tr(" by ") +
                          ::itoqs(curViewingSize.height()) +
                          tr(", which would require ") + size +
                          tr(" of memory to process.") +
                          tr("  Depending on your system resources, ") +
                          tr("processing could take anywhere from a ") +
                          tr("couple seconds to tens of minutes, and could ") +
                          tr("bog down your computer for that time.\n") +
                          tr("You may want to cancel and save your project ") +
                          tr("before attempting this operation.\n\n") +
                          tr("Do you want to continue?"));
    const int answer = QMessageBox::warning(this, tr("Memory usage warning"),
                                            warning,
                                            QMessageBox::Yes|QMessageBox::No);
    if (answer == QMessageBox::No) {
      return;
    }
  }

  const QImage saveImage = curImageForSaving();
  if (saveImage.isNull()) {
    const QString error(tr("Sorry, there wasn't enough memory to save ") +
                        tr("at the current image size.  Try reducing ") +
                        tr("the image's size before saving again."));
    QMessageBox::warning(this, tr("Image is too large to save"),
                         error);
    return;
  }
  if (::extension(outputFile) == "pdf") {
    doPdfSave(outputFile, saveImage);
  }
  else {
    doNonPdfSave(outputFile, saveImage);
  }
}

void imageSaverWindow::setSaveImageActionEnabled(bool b) {

  saveImageAction_->setEnabled(b);
}
