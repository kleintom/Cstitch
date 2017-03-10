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

#ifndef PATTERNMETADATA_H
#define PATTERNMETADATA_H

#include <QtWidgets/QPlainTextEdit>

#include "cancelAcceptDialogBase.h"

class QComboBox;
class QSettings;
class QVBoxLayout;
class QHBoxLayout;
class QGroupBox;
class QLabel;
class QSpinBox;
class QSettings;

// a QPlainTextEdit that always has a fixed line width (scrollbar is
// always visible) and returns as its text the first n lines, where n is
// specified at construction
class fixedWidthTextEdit : public QPlainTextEdit {

  Q_OBJECT

 public:
  fixedWidthTextEdit(int fixedPixelWidth, int numberOfLines,
                     QWidget* parent)
    : QPlainTextEdit(parent), fixedPixelWidth_(fixedPixelWidth),
    numberOfLines_(numberOfLines) {

    setTabChangesFocus(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  }
  // return the first numberOfLines_ lines of text
  QString startText() const {
    QTextCursor cursor = textCursor();
    // move to start of document
    cursor.movePosition(QTextCursor::Start);
    for (int i = 1; i < numberOfLines_; ++i) {
      // move down a line (noop if there's no line to move down to)
      cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
    }
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    return cursor.selectedText().replace(QChar(0x2029), '\n');
  }

 private:
  const int fixedPixelWidth_; // max width of the user's string in pixels
  // number of maxPixelWidth_ lines of input we allow
  const int numberOfLines_;
};

// class patternMetadata
//
// A dialog to get user input for the pdf pattern metadata: pattern title,
// pattern created by, original image (photo) by.
// Note that there's currently no way to produce pdf output with active
// hyperlinks, part of the reason we don't have explicit fields for such
// information.
//
class patternMetadata : public cancelAcceptDialogBase {

  Q_OBJECT

 public:
  patternMetadata(int pdfWidth, int titleFontSize, int patternByFontSize,
                  int photoByFontSize, QWidget* parent);
  void constructSymbolPreview(const QSettings& settings);
  QString title() const { return titleEdit_->startText(); }
  QString patternBy() const { return patternByEdit_->startText(); }
  QString photoBy() const { return photoByEdit_->startText(); }
  // use QSettings to save the current input fields to disk
  void saveSettings() const;
  int titleFontSize() const { return titleFontSize_; }
  int patternByFontSize() const { return patternByFontSize_; }
  int photoByFontSize() const { return photoByFontSize_; }
  int pdfSymbolSize() const;
  int boldLinesFrequency() const;

 private slots:
  // insert the text of the currently selected license at the current point
  void insertPatternByLicense();
  // insert the text of the currently selected license at the current point
  void insertPhotoByLicense();
  // clear all info fields
  void clearMetadata();
  // update the symbol size preview to reflect <newSymbolSize>
  void updateSymbolSize(int newSymbolSize);

 private:
  // load text for <editor> from <settings> using the settings key
  // <settingsKey>
  void loadSettings(const QSettings& settings,
                    const QString& settingsKey,
                    fixedWidthTextEdit* editor);
  // load <box> with copyright/license options (use the actual copyright
  // symbol if <font> provides it, else use (c)); add "derived from"
  // options if <derived>
  void loadLicenses(QComboBox* box, const QFont& font, bool derived);
  void constructBoldLinesFrequencyChooser(const QSettings& settings);

 private:
  QVBoxLayout* widgetLayout_;

  QGroupBox* metadataBox_;
  QVBoxLayout* metadataLayout_;

  //// pattern title
  QLabel* titleLabel_;
  fixedWidthTextEdit* titleEdit_;
  const QString titleSettingsKey_;
  const int titleFontSize_;

  // number of lines we'll use from the pattern by and photo by edits
  const int linesToKeep_;

  //// "pattern by" information
  QLabel* patternByLabel_;
  fixedWidthTextEdit* patternByEdit_;
  // the QSettings key string to use for patternByEdit_
  const QString patternBySettingsKey_;
  const int patternByFontSize_;
  QHBoxLayout* patternByLicenseLayout_;
  QPushButton* patternByLicenseButton_;
  QComboBox* patternByLicenses_;

  //// "photo by" information
  QLabel* photoByLabel_;
  fixedWidthTextEdit* photoByEdit_;
  // the QSettings key string to use for photoByEdit_
  const QString photoBySettingsKey_;
  const int photoByFontSize_;
  QHBoxLayout* photoByLicenseLayout_;
  QPushButton* photoByLicenseButton_;
  QComboBox* photoByLicenses_;

  QPushButton* clearMetadataButton_;

  //// symbol size selection
  QGroupBox* symbolSizeBox_;
  QVBoxLayout* symbolSizeLayout_;
  QHBoxLayout* symbolSizeTitleLayout_;
  QLabel* symbolSizeTitle_;
  QSpinBox* symbolSizeSpinBox_;
  QString symbolSizeKey_;

  QHBoxLayout* symbolPreviewLayout_;
  QLabel* symbolPreview_;

  //// Bold lines and square count frequency.
  QGroupBox* boldLinesBox_;
  QHBoxLayout* boldLinesLayout_;
  QLabel* boldLinesLabel_;
  QSpinBox* boldLinesFrequencySpinBox_;
  QString boldLinesFrequencyKey_;
};

#endif
