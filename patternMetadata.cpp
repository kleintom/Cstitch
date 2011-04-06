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
#include <QtCore/QDebug>

#include <QtGui/QApplication>
#include <QtGui/QComboBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

#include "imageUtility.h"

patternMetadata::patternMetadata(int pdfWidth, int titleFontSize,
                                 int patternByFontSize, int photoByFontSize,
                                 QWidget* parent)
  : cancelAcceptDialogBase(parent),
    widgetLayout_(new QVBoxLayout),
    groupBox_(new QGroupBox(tr("Pattern information (all fields are optional, all text will be centered in the pdf)"))),
    groupBoxLayout_(new QVBoxLayout),
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
    photoByLicenses_(new QComboBox) {

  // TODO this won't quite be right since the textEdit has mystery margins
  const int inputFieldWidth = pdfWidth + ::scrollbarWidth();
  const QFont applicationFont = QApplication::font();
  
#ifdef Q_OS_LINUX
  // Qt-linux bug (4.6.3) for QFontMetrics.lineSpacing()?
  const int linesFudge = 2;
#else
  const int linesFudge = 1;
#endif

  // title
  groupBoxLayout_->addWidget(titleLabel_);
  QFont titleFont = applicationFont;
  titleFont.setBold(true);
  titleFont.setPointSize(titleFontSize_);
  titleEdit_->setFont(titleFont);
  titleEdit_->setFixedWidth(inputFieldWidth);
  // TODO these fudges are probably not portable or lasting (then again,
  // what are the correct QT magic incantations to compute all of the
  // paddings, margins, frames, etc, and how often will they change?)
  const int lineHeightFudge = 8;
  const int comboBoxWidthFudge = 100;
  // we double lineSpacing since that's what seems to be correct...(!)
  titleEdit_->setFixedHeight(linesFudge*QFontMetrics(titleFont).lineSpacing() +
                             lineHeightFudge);
  groupBoxLayout_->addWidget(titleEdit_);
  groupBoxLayout_->addSpacing(20);

  // patternBy
  groupBoxLayout_->addWidget(patternByLabel_);
  QFont patternByFont = applicationFont;
  patternByFont.setPointSize(patternByFontSize_);
  patternByEdit_->setFont(patternByFont);
  patternByEdit_->setFixedWidth(inputFieldWidth);
  patternByEdit_->
    setFixedHeight(linesFudge*linesToKeep_*QFontMetrics(patternByFont).lineSpacing() +
                   lineHeightFudge);
  connect(patternByLicenseButton_, SIGNAL(clicked()),
          this, SLOT(insertPatternByLicense()));
  // unfortunately patternByLicenseButton_ doesn't know its width yet,
  // so we have to just approximate
  patternByLicenses_->setFixedWidth(inputFieldWidth - comboBoxWidthFudge);
  loadLicenses(patternByLicenses_, patternByFont, false);
  groupBoxLayout_->addWidget(patternByEdit_);
  patternByLicenseLayout_->addWidget(patternByLicenseButton_, 0,
                                     Qt::AlignLeft);
  patternByLicenseLayout_->addWidget(patternByLicenses_, 1, Qt::AlignLeft);
  groupBoxLayout_->addLayout(patternByLicenseLayout_);
  groupBoxLayout_->addSpacing(20);

  // photoBy
  groupBoxLayout_->addWidget(photoByLabel_);
  QFont photoByFont = applicationFont;
  photoByFont.setPointSize(photoByFontSize_);
  photoByEdit_->setFont(photoByFont);
  photoByEdit_->setFixedWidth(inputFieldWidth);
  photoByEdit_->
    setFixedHeight(linesFudge*linesToKeep_*QFontMetrics(photoByFont).lineSpacing() +
                   lineHeightFudge);
  connect(photoByLicenseButton_, SIGNAL(clicked()),
          this, SLOT(insertPhotoByLicense()));
  // unfortunately photobyLicenseButton_ doesn't know its width yet,
  // so we have to just approximate
  photoByLicenses_->setFixedWidth(inputFieldWidth - comboBoxWidthFudge);
  loadLicenses(photoByLicenses_, photoByFont, true);
  groupBoxLayout_->addWidget(photoByEdit_);
  photoByLicenseLayout_->addWidget(photoByLicenseButton_, 0, Qt::AlignLeft);
  photoByLicenseLayout_->addWidget(photoByLicenses_, 1, Qt::AlignLeft);
  groupBoxLayout_->addLayout(photoByLicenseLayout_);

  groupBox_->setLayout(groupBoxLayout_);
  widgetLayout_->addWidget(groupBox_);
  widgetLayout_->addWidget(cancelAcceptWidget());
  setLayout(widgetLayout_);

  // load any saved fields
  const QSettings settings("stitch", "stitch");
  loadSettings(settings, titleSettingsKey_, titleEdit_);
  loadSettings(settings, patternBySettingsKey_, patternByEdit_);
  loadSettings(settings, photoBySettingsKey_, photoByEdit_);

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
  box->addItem(identifier + " is " + copyrightCharacter + " " +
               year + " " + tr("[insert your name]."));
  box->addItem(identifier + tr(" is licensed under the Creative Commons Attribution-NonCommercial 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-nc/3.0/"));
  box->addItem(identifier + tr(" is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/"));
  box->addItem(identifier + tr(" is licensed under the Creative Commons Attribution-NonCommercial-NoDerivs 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/3.0/"));
  box->addItem(identifier + tr(" is licensed under the Creative Commons Attribution 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/"));
  box->addItem(identifier + tr(" is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/"));
  box->addItem(identifier + tr(" is licensed under the Creative Commons Attribution-NoDerivs 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-nd/3.0/"));
  box->addItem(tr("All copyright rights to ") + identifier.toLower() +
               tr(" have been waived under the Creative Commons CC0 1.0 Universal dedication. To view a copy of the dedication, visit http://creativecommons.org/publicdomain/zero/1.0/"));
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

void patternMetadata::loadSettings(const QSettings& settings,
                                   const QString& settingsKey,
                                   fixedWidthTextEdit* editor) {

  if (settings.contains(settingsKey)) {
    editor->setPlainText(settings.value(settingsKey).toString());
  }
}

void patternMetadata::saveSettings() const {

  QSettings settings("stitch", "stitch");
  settings.setValue(titleSettingsKey_, titleEdit_->toPlainText());
  settings.setValue(patternBySettingsKey_, patternByEdit_->toPlainText());
  settings.setValue(photoBySettingsKey_, photoByEdit_->toPlainText());
}

void patternMetadata::insertPatternByLicense() {

  patternByEdit_->insertPlainText(patternByLicenses_->currentText());
  patternByEdit_->setFocus();
}

void patternMetadata::insertPhotoByLicense() {

  photoByEdit_->insertPlainText(photoByLicenses_->currentText());
  photoByEdit_->setFocus();
}
