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

#include "patternMetadata.h"

#include <QtCore/QSettings>
#include <QtCore/QDate>

#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QPainter>

#include "imageUtility.h"
#include "symbolChooser.h"

// bounds for allowed symbol sizes (too large and file sizes are ridiculous,
// too small and symbols can't be distinguished)
extern const int MAX_SYMBOL_SIZE;
extern const int MIN_SYMBOL_SIZE;

patternMetadata::patternMetadata(int pdfWidth, int titleFontSize,
                                 int patternByFontSize, int photoByFontSize,
                                 QWidget* parent)
  : cancelAcceptDialogBase(parent),
    widgetLayout_(new QVBoxLayout),
    metadataBox_(new QGroupBox(tr("Pattern information (all fields are optional, all text will be centered in the pdf)"))),
    metadataLayout_(new QVBoxLayout),
    titleLabel_(new QLabel(tr("<u><b>Pattern title</b></u> (only the first line will be used):"))),
    titleEdit_(new fixedWidthTextEdit(pdfWidth, 1, this)),
    titleSettingsKey_("pattern_title"),
    titleFontSize_(titleFontSize),
    linesToKeep_(4),
    patternByLabel_(new QLabel(tr("<u><b>Pattern information</b></u> (only the first four lines will be used):"))),
    patternByEdit_(new fixedWidthTextEdit(pdfWidth, linesToKeep_, this)),
    patternBySettingsKey_("pattern_by"),
    patternByFontSize_(patternByFontSize),
    patternByLicenseLayout_(new QHBoxLayout),
    patternByLicenseButton_(new QPushButton(tr("Insert"))),
    patternByLicenses_(new QComboBox),
    photoByLabel_(new QLabel(tr("<u><b>Photo information</b></u> (only the first four lines will be used):"))),
    photoByEdit_(new fixedWidthTextEdit(pdfWidth, linesToKeep_, this)),
    photoBySettingsKey_("photo_by"),
    photoByFontSize_(photoByFontSize),
    photoByLicenseLayout_(new QHBoxLayout),
    photoByLicenseButton_(new QPushButton(tr("Insert"))),
    photoByLicenses_(new QComboBox),
    clearMetadataButton_(new QPushButton(tr("Clear all information"))),
    symbolSizeBox_(new QGroupBox(tr("Pdf symbol size"))),
    symbolSizeLayout_(new QVBoxLayout),
    symbolSizeTitleLayout_(new QHBoxLayout),
    symbolSizeTitle_(new QLabel(tr("Set the pdf symbol size (from %1 to %2):").
                                arg(QString::number(MIN_SYMBOL_SIZE)).
                                arg(QString::number(MAX_SYMBOL_SIZE)))),
    symbolSizeSpinBox_(new QSpinBox),
    symbolSizeKey_("pdf_symbol_size"),
    symbolPreviewLayout_(new QHBoxLayout),
    symbolPreview_(new QLabel) {

  // TODO this won't quite be right since the textEdit has mystery margins
  const int inputFieldWidth = pdfWidth + ::scrollbarWidth();
  const QFont applicationFont = QApplication::font();

  // title
  metadataLayout_->addWidget(titleLabel_);
  QFont titleFont = applicationFont;
  titleFont.setBold(true);
  titleFont.setPointSize(titleFontSize_);
  titleEdit_->setFont(titleFont);
  titleEdit_->setFixedWidth(inputFieldWidth);
  // TODO these fudges are probably not portable or lasting (then again,
  // what are the correct QT magic incantations to compute all of the
  // paddings, margins, frames, etc, and how often will they change?)
  const int lineHeightFudge = 12;//8;
  const int comboBoxWidthFudge = 100;
  titleEdit_->setFixedHeight(QFontMetrics(titleFont).lineSpacing() +
                             lineHeightFudge);
  metadataLayout_->addWidget(titleEdit_);
  metadataLayout_->addSpacing(20);

  // patternBy
  metadataLayout_->addWidget(patternByLabel_);
  QFont patternByFont = applicationFont;
  patternByFont.setPointSize(patternByFontSize_);
  patternByEdit_->setFont(patternByFont);
  patternByEdit_->setFixedWidth(inputFieldWidth);
  patternByEdit_->
    setFixedHeight(linesToKeep_*QFontMetrics(patternByFont).lineSpacing() +
                   lineHeightFudge);
  connect(patternByLicenseButton_, SIGNAL(clicked()),
          this, SLOT(insertPatternByLicense()));
  // unfortunately patternByLicenseButton_ doesn't know its width yet,
  // so we have to just approximate
  patternByLicenses_->setFixedWidth(inputFieldWidth - comboBoxWidthFudge);
  loadLicenses(patternByLicenses_, patternByFont, false);
  metadataLayout_->addWidget(patternByEdit_);
  patternByLicenseLayout_->addWidget(patternByLicenseButton_, 0,
                                     Qt::AlignLeft);
  patternByLicenseLayout_->addWidget(patternByLicenses_, 1, Qt::AlignLeft);
  metadataLayout_->addLayout(patternByLicenseLayout_);
  metadataLayout_->addSpacing(20);

  // photoBy
  metadataLayout_->addWidget(photoByLabel_);
  QFont photoByFont = applicationFont;
  photoByFont.setPointSize(photoByFontSize_);
  photoByEdit_->setFont(photoByFont);
  photoByEdit_->setFixedWidth(inputFieldWidth);
  photoByEdit_->
    setFixedHeight(linesToKeep_*QFontMetrics(photoByFont).lineSpacing() +
                   lineHeightFudge);
  connect(photoByLicenseButton_, SIGNAL(clicked()),
          this, SLOT(insertPhotoByLicense()));
  // unfortunately photobyLicenseButton_ doesn't know its width yet,
  // so we have to just approximate
  photoByLicenses_->setFixedWidth(inputFieldWidth - comboBoxWidthFudge);
  loadLicenses(photoByLicenses_, photoByFont, true);
  metadataLayout_->addWidget(photoByEdit_);
  photoByLicenseLayout_->addWidget(photoByLicenseButton_, 0, Qt::AlignLeft);
  photoByLicenseLayout_->addWidget(photoByLicenses_, 1, Qt::AlignLeft);
  metadataLayout_->addLayout(photoByLicenseLayout_);

  connect(clearMetadataButton_, SIGNAL(clicked()),
          this, SLOT(clearMetadata()));
  metadataLayout_->addWidget(clearMetadataButton_);

  metadataBox_->setLayout(metadataLayout_);
  widgetLayout_->addWidget(metadataBox_);
  setLayout(widgetLayout_);

  // load any saved fields
  const QSettings settings("cstitch", "cstitch");
  loadSettings(settings, titleSettingsKey_, titleEdit_);
  loadSettings(settings, patternBySettingsKey_, patternByEdit_);
  loadSettings(settings, photoBySettingsKey_, photoByEdit_);

  constructSymbolPreview(settings);

  widgetLayout_->addWidget(cancelAcceptWidget());
  titleEdit_->setFocus();
  setWindowTitle(tr("Pattern information"));
}

void patternMetadata::loadLicenses(QComboBox* box, const QFont& font,
                                   bool derived) {

  const QFontMetrics metric(font);
  QString copyrightCharacter =
    metric.inFont(QChar(0x00A9)) ? QString(QChar(0x00A9)) : "(c)";
  const QString year = QString::number(QDate::currentDate().year());
  const QString identifier = derived ?
    tr("The image on this page") : tr("This work");
  //: For example: "This work is (c) 2014 [insert your name]"
  box->addItem(tr("%1 is %2 %3 [insert your name]")
               .arg(identifier)
               .arg(copyrightCharacter)
               .arg(year));
  //: %1 is either "The image on this page" or "This work"
  box->addItem(tr("%1 is licensed under the Creative Commons Attribution-NonCommercial 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-nc/3.0/").arg(identifier));
  box->addItem(tr("%1 is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/").arg(identifier));
  box->addItem(tr("%1 is licensed under the Creative Commons Attribution-NonCommercial-NoDerivs 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/3.0/").arg(identifier));
  box->addItem(tr("%1 is licensed under the Creative Commons Attribution 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/").arg(identifier));
  box->addItem(tr("%1 is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/").arg(identifier));
  box->addItem(tr("%1 is licensed under the Creative Commons Attribution-NoDerivs 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-nd/3.0/").arg(identifier));
  box->addItem(tr("All copyright rights to %1 have been waived under the Creative Commons CC0 1.0 Universal dedication. To view a copy of the dedication, visit http://creativecommons.org/publicdomain/zero/1.0/").arg(identifier.toLower()));
  if (derived) {
    box->addItem(tr("This work is derived from the image on this page, \"[name of image]\", a photo by [name and http] with a [insert license]"));
    box->addItem(tr("Creative Commons Attribution-NonCommercial 3.0 Unported License"));
    box->addItem(tr("Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License"));
    box->addItem(tr("Creative Commons Attribution-NonCommercial-NoDerivs 3.0 Unported License"));
    box->addItem(tr("Creative Commons Attribution 3.0 Unported License"));
    box->addItem(tr("Creative Commons Attribution-ShareAlike 3.0 Unported License"));
    box->addItem(tr("Creative Commons Attribution-NoDerivs 3.0 Unported License"));
  }
}

void patternMetadata::constructSymbolPreview(const QSettings& settings) {

  connect(symbolSizeSpinBox_, SIGNAL(valueChanged(int )),
          this, SLOT(updateSymbolSize(int )));
  symbolSizeSpinBox_->setRange(MIN_SYMBOL_SIZE, MAX_SYMBOL_SIZE);
  if (settings.contains(symbolSizeKey_)) {
    symbolSizeSpinBox_->setValue(settings.value(symbolSizeKey_).toInt());
  }
  else {
    symbolSizeSpinBox_->setValue(30);
  }

  symbolSizeTitleLayout_->addWidget(symbolSizeTitle_);
  symbolSizeTitleLayout_->addWidget(symbolSizeSpinBox_);
  symbolSizeTitleLayout_->addStretch();
  symbolSizeLayout_->addLayout(symbolSizeTitleLayout_);

  symbolPreviewLayout_->addStretch();
  symbolPreviewLayout_->addWidget(symbolPreview_);
  symbolPreviewLayout_->addStretch();
  symbolSizeLayout_->addLayout(symbolPreviewLayout_);
  
  symbolSizeBox_->setLayout(symbolSizeLayout_);
  widgetLayout_->addWidget(symbolSizeBox_);
}

void patternMetadata::loadSettings(const QSettings& settings,
                                   const QString& settingsKey,
                                   fixedWidthTextEdit* editor) {

  if (settings.contains(settingsKey)) {
    editor->setPlainText(settings.value(settingsKey).toString());
  }
}

void patternMetadata::saveSettings() const {

  QSettings settings("cstitch", "cstitch");
  settings.setValue(titleSettingsKey_, titleEdit_->toPlainText());
  settings.setValue(patternBySettingsKey_, patternByEdit_->toPlainText());
  settings.setValue(photoBySettingsKey_, photoByEdit_->toPlainText());
  settings.setValue(symbolSizeKey_, symbolSizeSpinBox_->value());
}

void patternMetadata::insertPatternByLicense() {

  patternByEdit_->insertPlainText(patternByLicenses_->currentText());
  patternByEdit_->setFocus();
}

void patternMetadata::insertPhotoByLicense() {

  photoByEdit_->insertPlainText(photoByLicenses_->currentText());
  photoByEdit_->setFocus();
}

void patternMetadata::clearMetadata() {

  titleEdit_->setPlainText("");
  patternByEdit_->setPlainText("");
  photoByEdit_->setPlainText("");
}

void patternMetadata::updateSymbolSize(int newSymbolSize) {

  const int viewSize = MAX_SYMBOL_SIZE + 10;
  QPixmap symbol(viewSize, viewSize);
  symbol.fill(palette().color(QPalette::Window));
  QPainter painter(&symbol);
  const QPixmap sampleSymbol = symbolChooser::getSampleSymbol(newSymbolSize);
  painter.drawPixmap((viewSize - newSymbolSize)/2,
                     (viewSize - newSymbolSize)/2, sampleSymbol);
  symbolPreview_->setPixmap(symbol);
}

int patternMetadata::pdfSymbolSize() const {

  return symbolSizeSpinBox_->value();
}
