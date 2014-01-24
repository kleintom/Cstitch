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

#include "imageProcessing.h"

#include <algorithm>

#include <QtCore/QTime>
#include <QtCore/QStack>

#include "colorLists.h"
#include "grid.h"
#include "utility.h"
#include "imageUtility.h"
#include "versionProcessing.h"

// max ::ds distance between two colors
extern const int D_MAX = 766;
// larger than the largest possible sum of distances of a given color
// to colors in a square
extern const int D_SUM_MAX = 16777216;
// always show progress meters at the same coordinates (they usually don't
// have parents to center on)
extern const int PROGRESS_X_COORDINATE = 300;
extern const int PROGRESS_Y_COORDINATE = 250;

colorTransformerPtr
colorTransformer::createColorTransformer(flossType type) {

  switch(type.value()) {
  case flossDMC:
    return colorTransformerPtr(new dmcTransformer());
    break;
  case flossAnchor:
    return colorTransformerPtr(new anchorTransformer());
    break;
  case flossVariable:
    return colorTransformerPtr(new variableTransformer());
    break;
  default:
    qWarning() << "Bad transformer floss type:" << type.value();
    return colorTransformerPtr(new variableTransformer());
    break;
  }
}

void segment(const QImage& sourceImage, QImage* newImage,
             const QList<pixel>& squaresList, int dim,
             const QVector<triC>& colors) {

  const int colorListSize = colors.size();
  for (QList<pixel>::const_iterator it = squaresList.constBegin(),
        end = squaresList.constEnd(); it != end; ++it) {
    const int xStart = (*it).x() * dim;
    const int xEnd = xStart + dim;
    const int yStart = (*it).y() * dim;
    const int yEnd = yStart + dim;
    for (int i = xStart; i < xEnd; ++i) {
      for (int j = yStart; j < yEnd; ++j) {
        const QRgb thisColor = sourceImage.pixel(i, j);
        int min = D_MAX;
        int chosenIndex = 0;
        for (int k = 0; k < colorListSize; ++k) {
          const int tmpMin = ds(thisColor, colors[k]);
          if (tmpMin < min) {
            min = tmpMin;
            chosenIndex = k;
          }
        }
        newImage->setPixel(i, j, colors[chosenIndex].qrgb());
      }
    }
  }
}

QVector<triC> segment(QImage* newImage, const QVector<triC>& colors,
                      int numImageColors) {

  if (colors.empty()) {
    qWarning() << "Empty color list in segment.";
    return QVector<triC>();
  }
  //QTime t;
  //t.start();

  QSet<QRgb> colorsUsed;
  colorsUsed.reserve(colors.size());
  const int width = newImage->width();
  const int height = newImage->height();
  // keys are image colors; values are closest output matches
  QHash<QRgb, QRgb> colorMap;
  colorMap.reserve(numImageColors);
  triC previousColor;
  altMeter progressMeter(QObject::tr("Creating new image..."),
                         QObject::tr("Cancel"), 0, height/32);
  progressMeter.setMinimumDuration(2000);
  progressMeter.show();
  for (int j = 0; j < height; ++j) {
    if (progressMeter.wasCanceled()) {
      return QVector<triC>();
    }
    if (j%128 == 0) {
      progressMeter.setValue(j/32);
    }
    for (int i = 0; i < width; ++i) {
      const QRgb thisColor = newImage->pixel(i, j);
      //// [I removed lookahead to see if there are more of this color
      //// coming up since in photographs I think it's very rare that
      //// colors get repeated (and some tests confirmed that lookahead
      //// was slower).]
      QRgb chosenQRgbColor;
      const QHash<QRgb, QRgb>::const_iterator foundIt =
        colorMap.find(thisColor);
      if (foundIt != colorMap.end()) {
        chosenQRgbColor = *foundIt;
      }
      else {
        int min = D_MAX;
        int chosenIndex = 0;
        for (int k = 0, size = colors.size(); k < size; ++k) {
          const int thisD = ::ds(thisColor, colors[k]);
          if (thisD < min) {
            min = thisD;
            chosenIndex = k;
          }
        }
        chosenQRgbColor = colors[chosenIndex].qrgb();
        colorMap[thisColor] = chosenQRgbColor;
        colorsUsed.insert(chosenQRgbColor);
      }
      newImage->setPixel(i, j, chosenQRgbColor);
    }
  }
  QVector<triC> returnColors;
  for (QSet<QRgb>::const_iterator it = colorsUsed.begin(),
         end = colorsUsed.end(); it != end; ++it) {
    returnColors.push_back(triC(*it));
  }
  //  qDebug() << "segment time:" << double(t.elapsed())/1000.;
  return returnColors;
}

QVector<triC> mode(QImage* newImage, int dimension) {

  const int xMax = newImage->width() - dimension;
  const int yMax = newImage->height() - dimension;
  QSet<QRgb> colorsChosen; // colors to be returned
  QHash<QRgb, int> colorFrequencies; // color frequency counts
  altMeter progressMeter(QObject::tr("Creating new image..."),
                                QObject::tr("Cancel"), 0, yMax/dimension);
  progressMeter.setMinimumDuration(2000);
  progressMeter.show();
  for (int j = 0; j <= yMax; j += dimension) {
    const int thisYMax = j + dimension;
    if (progressMeter.wasCanceled()) {
      return QVector<triC>();
    }
    const int jBox = j/dimension;
    if (jBox%32 == 0) {
      progressMeter.setValue(jBox);
    }
    for (int i = 0; i <= xMax; i +=  dimension) {
      const int thisXMax = i + dimension;
      colorFrequencies.clear();
      for (int b = j; b < thisYMax; ++b) {
        for (int a = i; a < thisXMax; ++a) {
          colorFrequencies[newImage->pixel(a, b)]++;
        }
      }
      // find the one most represented
      int maxCount = 0;
      QRgb chosenColor = Qt::black;
      for (QHash<QRgb, int>::const_iterator it = colorFrequencies.begin(),
             end = colorFrequencies.end(); it != end; ++it) {
        if (it.value() > maxCount) {
          maxCount = it.value();
          chosenColor = it.key();
        }
        else if (it.value() == maxCount) {
          // decide if chosenColor or this new contender matches better
          int chosenDistanceSum = 0;
          const triC chosenTricColor(chosenColor);
          for (int b = j; b < thisYMax; ++b) {
            for (int a = i; a < thisXMax; ++a) {
              chosenDistanceSum +=
                ds(newImage->pixel(a, b), chosenTricColor);
            }
          }
          QRgb newContender = it.key();
          int newDistanceSum = 0;
          const triC newTricColor(newContender);
          for (int b = j; b < thisYMax; ++b) {
            for (int a = i; a < thisXMax; ++a) {
              newDistanceSum +=
                ds(newImage->pixel(a, b), newTricColor);
            }
          }
          if (newDistanceSum < chosenDistanceSum) {
            chosenColor = newContender;
          }
        }
      }
      colorsChosen.insert(chosenColor);
      for (int b = j; b < thisYMax; b++) {
        for (int a = i; a < thisXMax; a++) {
          newImage->setPixel(a, b, chosenColor);
        }
      }
    }
  }
  QVector<triC> returnColors;
  returnColors.reserve(colorsChosen.size());
  for (QSet<QRgb>::const_iterator it = colorsChosen.begin(),
          end = colorsChosen.end(); it != end; ++it) {
    returnColors.push_back(*it);
  }
  return returnColors;
}

QVector<triC> median(grid* newImage, const grid& originalImage,
                     int dimension) {

  const int xMax = newImage->width() - dimension;
  const int yMax = newImage->height() - dimension;
  QSet<triC> colorsChosen;
  altMeter progressMeter(QObject::tr("Creating new image..."),
                                QObject::tr("Cancel"), 0, yMax/dimension);
  progressMeter.setMinimumDuration(1500);
  progressMeter.show();
  for (int yStart = 0; yStart <= yMax; yStart += dimension) {
    const int yEnd = yStart + dimension;
    if (progressMeter.wasCanceled()) {
      return QVector<triC>();
    }
    const int yBox = yStart/dimension;
    if (yBox%32 == 0) {
      progressMeter.setValue(yBox);
    }
    for (int xStart = 0; xStart <= xMax; xStart += dimension) {
      const int xEnd = xStart + dimension;
      // for each pixel in this block...
      int smallestSum = D_SUM_MAX;
      pairOfInts chosenPoint;
      QSet<triC> colorsComputed;
      for (int j = yStart; j < yEnd; ++j) {
        for (int i = xStart; i < xEnd; ++i) {
          // compute a distance sum for this pixel in the block
          //// we're doing a lot of double computation here, but
          //// it's still faster than caching and looking up
          //// since we only repeat each computation once
          int distanceSum = 0;
          const triC thisColor = newImage->operator()(i, j);
          if (!colorsComputed.contains(thisColor)) {
            colorsComputed.insert(thisColor);
            for (int jj = yStart; jj < yEnd; ++jj) {
              for (int ii = xStart; ii < xEnd; ++ii) {
                distanceSum += ::ds(originalImage(ii, jj), thisColor);
                if (distanceSum > smallestSum) {
                  goto NEXT;
                }
              }
            }
            // this is the smallest sum so far
            smallestSum = distanceSum;
            chosenPoint = pairOfInts(i, j);
          NEXT: {}
          }
        }
      }
      const triC chosenColor =
        newImage->operator()(chosenPoint.x(), chosenPoint.y());
      // set everything in the block to the smallest sum pixel
      colorsChosen.insert(chosenColor);
      for (int j = yStart; j < yEnd; ++j) {
        for (int i = xStart; i < xEnd; ++i) {
          newImage->operator()(i, j) = chosenColor;
        }
      }
    }
  }
  QVector<triC> returnColors;
  returnColors.reserve(colorsChosen.size());
  for (QSet<triC>::const_iterator it = colorsChosen.begin(),
          end = colorsChosen.end(); it != end; ++it) {
    returnColors.push_back(*it);
  }
  return returnColors;
}

QVector<triC> median(QImage* newImage, const QImage& originalImage,
                     const QList<pixel>& squaresList,
                     const QVector<historyPixel>& oldColors,
                     int dimension) {

  QVector<triC> colorsChosen;
  colorsChosen.reserve(squaresList.size());
  // we're iterating over squaresList and oldColors at the same time
  QVector<historyPixel>::const_iterator oldColorsI = oldColors.begin();
  for (QList<pixel>::const_iterator it = squaresList.constBegin(),
        end = squaresList.constEnd(); it != end; ++it, ++oldColorsI) {
    const int xStart = (*it).x() * dimension;
    const int xEnd = xStart + dimension;
    const int yStart = (*it).y() * dimension;
    const int yEnd = yStart + dimension;
    int smallestSum = D_SUM_MAX;
    pairOfInts chosenPoint;
    QSet<triC> colorsComputed;
    // for each pixel in this block...
    for (int j = yStart; j < yEnd; ++j) {
      for (int i = xStart; i < xEnd; ++i) {
        // compute a distance sum for this pixel in the block
        //// we're doing a lot of double computation here, but
        //// it's still faster than caching and looking up
        //// since we only repeat each computation once
        int distanceSum = 0;
        const triC thisColor = newImage->pixel(i, j);
        if (!colorsComputed.contains(thisColor)) {
          colorsComputed.insert(thisColor);
          for (int jj = yStart; jj < yEnd; ++jj) {
            for (int ii = xStart; ii < xEnd; ++ii) {
              distanceSum += ds(originalImage.pixel(ii, jj), thisColor);
              if (distanceSum > smallestSum) {
                goto NEXT;
              }
            }
          }
          // this is the smallest sum so far
          smallestSum = distanceSum;
          chosenPoint = pairOfInts(i, j);
        NEXT: {}
        }
      }
    }
    triC chosenColor = newImage->pixel(chosenPoint.x(), chosenPoint.y());
    // check to see if the old color is actually a better fit
    int oldColorSum = 0;
    const triC thisOldColor = (*oldColorsI).oldColor();
    for (int j = yStart; j < yEnd; ++j) {
      for (int i = xStart; i < xEnd; ++i) {
        oldColorSum += ds(originalImage.pixel(i, j), thisOldColor);
      }
    }
    if (oldColorSum < smallestSum) {
      chosenColor = thisOldColor;
    }
    colorsChosen.push_back(chosenColor);
    for (int j = yStart; j < yEnd; ++j) {
      for (int i = xStart; i < xEnd; ++i) {
        newImage->setPixel(i, j, chosenColor.qrgb());
      }
    }
  }
  return colorsChosen;
}

QVector<triC> chooseColors(const QImage& image, int numColors,
                           const QVector<triC>& seedColors,
                           int numImageColors,
                           const colorTransformerPtr& transformer) {

//  QTime t;
//  t.start();
  const int width = image.width();
  const int height = image.height();
  altMeter progressMeter(QObject::tr("Choosing colors Step 1/2..."),
                                QObject::tr("Cancel"), 0, height/32);
  progressMeter.setMinimumDuration(1000);
  progressMeter.show();
  QHash<QRgb, int> colorCountMap;
  colorCountMap.reserve(numImageColors);
  // fill in colorCountMap, with colors for keys and color counts for
  // values
  for (int j = 0; j < height; ++j) {
    if (progressMeter.wasCanceled()) {
      return QVector<triC>();
    }
    if (j%32 == 0) {
      progressMeter.setValue(j/32);
    }
    for (int i = 0; i < width; ++i) {
      const QRgb thisColor = image.pixel(i, j);
      // look ahead
      int count = 1;
      while (i + count < width && image.pixel(i + count, j) == thisColor) {
        ++count;
      }
      colorCountMap[thisColor] += count;
      i += count - 1;
    }
  }
  //qDebug() << "First count time: " << double(t.elapsed())/1000.;
  QVector<QRgb> seedRgbColors;
  seedRgbColors.reserve(seedColors.size());
  for (int i = 0, size = seedColors.size(); i < size; ++i) {
    seedRgbColors.push_back(seedColors[i].qrgb());
  }
  return chooseColorsFromList(colorCountMap, seedRgbColors,
                              numColors + seedRgbColors.size(),
                              transformer);
}

QVector<triC> chooseColors(const QImage& image,
                                       const QList<pixel>& squaresList,
                                       int dimension, int numColors,
                                       const colorTransformerPtr& transformer) {

  QHash<QRgb, int> colorCount;
  // fill in colorCountMap, with colors for keys and color counts for
  // values
  for (QList<pixel>::const_iterator it = squaresList.constBegin(),
        end = squaresList.constEnd(); it != end; ++it) {
    const int xStart = (*it).x() * dimension;
    const int xEnd = xStart + dimension;
    const int yStart = (*it).y() * dimension;
    const int yEnd = yStart + dimension;
    for (int j = yStart; j < yEnd; ++j) {
      for (int i = xStart; i < xEnd; ++i) {
        ++colorCount[image.pixel(i, j)];
      }
    }
  }
  return chooseColorsFromList(colorCount, QVector<QRgb>(), numColors,
                              transformer);
}

QVector<triC> chooseColorsFromList(const QHash<QRgb, int>& colorCountMap,
                                   const QVector<QRgb> seedColors,
                                   int numColors,
                                   const colorTransformerPtr& transformer) {

  QHash<QRgb, QRgb> toDmc; // key is rgb, value is the closest dmc color
  toDmc.reserve(colorCountMap.size());
  QHash<QRgb, int> dmcCountMap; // counts of dmc colors
  dmcCountMap.reserve(428);
  QTime t;
  t.start();
  altMeter progressMeter(QObject::tr("Choosing colors Step 2/2..."),
                                QObject::tr("Cancel"), 0,
                                colorCountMap.size()/64);
  progressMeter.setMinimumDuration(1000);
  progressMeter.show();
  int progressCount = 0;
  //// Step 1: create a dmc color count map, where the keys are the
  //// closest dmc matches to colors in colorCountMap and counts are
  //// sums over all color counts in colorCountMap that map to the given
  //// dmc color.
  //// The idea here is to "blur" the original image to regions that are
  //// approximately the same color (where approximately means they all
  //// map to the same dmc color), and then to count colors for that
  //// blurred image (so large regions that never repeat a color but have
  //// all of their colors very close will get counted as one color).
  for (QHash<QRgb, int>::const_iterator it = colorCountMap.constBegin(),
         end = colorCountMap.end(); it != end; ++it, ++progressCount) {
    if (progressMeter.wasCanceled()) {
      return QVector<triC>();
    }
    if (progressCount % 64 == 0) {
      progressMeter.setValue(progressCount/64);
    }
    const QRgb keyColor = it.key();
    const QRgb dmcColor = ::rgbToDmc(keyColor).qrgb();
    toDmc[keyColor] = dmcColor;
    dmcCountMap[dmcColor] += it.value();
  }
  transformer->setDMCHash(toDmc);

  QVector<colorCount> colorCounts;
  colorCounts.reserve(colorCountMap.size());
  //// Step 2: add the original counts and the dmc counts by including
  //// in the original color count the dmc count from the dmc color that
  //// the original color maps to (so the original color gets its original
  //// count plus the count of its "blurred" region from Step 1).
  for (QHash<QRgb, int>::const_iterator it = colorCountMap.constBegin(),
          end = colorCountMap.constEnd(); it != end; ++it) {
    colorCounts.push_back(colorCount(it.key(),
                                     it.value() +
                                     dmcCountMap[toDmc[it.key()]]));
  }
  //  qDebug() << "cchoose colors recount:" << double(t.elapsed())/1000.;
  t.restart();
  std::sort(colorCounts.begin(), colorCounts.end(), qGreater<colorCount>());
  //qDebug() << "Sort time:" << double(t.elapsed())/1000.;
  t.restart();
  QVector<QRgb> returnColors = seedColors;
  returnColors.reserve(numColors);
  // TODO: should base separation on the spread of the colors
  int separation = 90;
  const int colorCountsSize = colorCounts.size();
  // don't choose any colors with frequency less than minCount
  int minCount = (colorCounts[colorCounts.size()/2].count() < 50) ? 0 : 50;
  //// Step 3: Choose the colors.  Start at the top of the count list and
  //// work down, but don't choose any color within <separation> of a color
  //// already chosen, and don't choose any color with count less than
  //// <minCount>.  If we reach the bottom of the count list and haven't
  //// chosen <numColors> yet, then reduce <separation> and run the list
  //// again.
  const chooseColorsVersionPtr chooser =
    versionProcessor::processor()->chooseColors();
  while (returnColors.size() < numColors && separation >= 0) {
    for (int i = 0; i < colorCountsSize; ++i) {
      QRgb thisColor = transformer->transform(colorCounts[i].color());
      if (colorCounts[i].count() >= minCount &&
          !returnColors.contains(thisColor)) {
        bool addColor = true;
        // don't add a color within separation of a color already chosen
        const triC thisTricColor(thisColor);
        for (int j = 0, size = returnColors.size(); j < size; ++j) {
          if (::ds(thisTricColor, returnColors[j]) < separation) {
            addColor = false;
            break;
          }
        }
        if (addColor) {
          if (separation >= 10) {
            // go back and see if we can do 10% better on count for
            // just a small distance allowance
            const int newMinCount = 1.1 * colorCounts[i].count();
            for (int j = 0; j < i; ++j) {
              const QRgb thisOldColor =
                chooser->transform(transformer, colorCounts[j].color());
              // TODO: ::ds is probably very rarely <= 7 if the 
              // transformer is to a fixed colors set
              if (colorCounts[j].count() >= newMinCount &&
                  ::ds(thisOldColor, thisColor) <= 7 &&
                  !returnColors.contains(thisOldColor)) {
                thisColor = thisOldColor;
                break;
              }
            }
          }
          returnColors.push_back(thisColor);
          if (returnColors.size() == numColors) {
            break;
          }
        }
      }
    }
    separation -= (separation > 10) ? 10 : 5;
    if (separation == 0) {
      minCount = 0;
    }
  }
  //qDebug() << "Actual choose time:" << double(t.elapsed())/1000.;
  QVector<triC> tricReturnColors;
  tricReturnColors.reserve(returnColors.size());
  for (int i = 0, size = returnColors.size(); i < size; ++i) {
    tricReturnColors.push_back(returnColors[i]);
  }
  return tricReturnColors;
}

// returns box coordinates
QVector<pairOfInts> changeColor(QImage* newImage, QRgb oldColor,
                                QRgb newColor, int dimension) {

  int xBoxes = newImage->width()/dimension;
  int yBoxes = newImage->height()/dimension;
  QVector<pairOfInts> returnCoords;
  for (int boxY = 0; boxY < yBoxes; ++boxY) {
    for (int boxX = 0; boxX < xBoxes; ++boxX) {
      int xStart = boxX * dimension;
      int yStart = boxY * dimension;
      if (newImage->pixel(xStart, yStart) == oldColor) {
        int xEnd = xStart + dimension;
        int yEnd = yStart + dimension;
        for (int j = yStart; j < yEnd; ++j) {
          for (int i = xStart; i < xEnd; ++i) {
            newImage->setPixel(i, j, newColor);
          }
        }
        returnCoords.push_back(pairOfInts(xStart/dimension,
                                          yStart/dimension));
      }
    }
  }
  return returnCoords;
}

void changeOneBlock(QImage* newImage, int x, int y, QRgb newColor,
                    int dimension, bool blockCoords) {

  int xStart, xEnd, yStart, yEnd;
  if (blockCoords == false) {
    xStart = (x / dimension)*dimension;
    xEnd = xStart + dimension;
    yStart = (y / dimension)*dimension;
    yEnd = yStart + dimension;
  }
  else {
    xStart = x*dimension;
    xEnd = xStart + dimension;
    yStart = y*dimension;
    yEnd = yStart + dimension;
  }
  for (int j = yStart; j < yEnd; ++j) {
    for (int i = xStart; i < xEnd; ++i) {
      newImage->setPixel(i, j, newColor);
    }
  }
}

template<class T>
void changeBlocks(QImage* newImage, const QVector<T>& points,
                  QRgb newColor, int dimension, bool blockCoords) {

  for (typename QVector<T>::const_iterator it = points.begin(),
          end = points.end(); it != end; ++it) {
    int x = (*it).x();
    int y = (*it).y();
    int xStart, yStart;
    if (blockCoords == false) {
      xStart = (x / dimension)*dimension;
      yStart = (y / dimension)*dimension;
    }
    else {
      xStart = x*dimension;
      yStart = y*dimension;
    }
    int xEnd = xStart + dimension;
    int yEnd = yStart + dimension;
    for (int j = yStart; j < yEnd; ++j) {
      for (int i = xStart; i < xEnd; ++i) {
        newImage->setPixel(i, j, newColor);
      }
    }
  }
}
template void changeBlocks<pairOfInts>(QImage* newImage,
                           const QVector<pairOfInts>& points,
                           QRgb newColor, int dimension, bool blockCoords);
template void changeBlocks<pixel>(QImage* newImage,
                           const QVector<pixel>& points,
                           QRgb newColor, int dimension, bool blockCoords);

void changeBlocks(QImage* newImage, const QVector<pixel>& pixels,
                  int dimension, bool blockCoords) {

  for (QVector<pixel>::const_iterator it = pixels.begin(),
          end = pixels.end(); it != end; ++it) {
    int x = (*it).x();
    int y = (*it).y();
    int xStart, yStart;
    if (blockCoords == false) {
      xStart = (x / dimension)*dimension;
      yStart = (y / dimension)*dimension;
    }
    else {
      xStart = x*dimension;
      yStart = y*dimension;
    }
    int xEnd = xStart + dimension;
    int yEnd = yStart + dimension;
    QRgb newColor = (*it).color();
    for (int j = yStart; j < yEnd; ++j) {
      for (int i = xStart; i < xEnd; ++i) {
        newImage->setPixel(i, j, newColor);
      }
    }
  }
}

QVector<pairOfInts> fillRegion(QImage* newImage, int x, int y,
                               QRgb newColor, int dimension) {

  const QRgb oldColor = newImage->pixel(x, y);
  if (oldColor == newColor) {
    return QVector<pairOfInts>();
  }

  newImage->setPixel(x, y, newColor);
  const int width = newImage->width();
  const int height = newImage->height();
  // a stack of squares the neighbors of which are still being checked
  QStack<pairOfInts> coordStack;
  QVector<pairOfInts> returnSquares;
  int i = (x/dimension)*dimension;
  int j = (y/dimension)*dimension;
  coordStack.push(pairOfInts(i, j));
  returnSquares.push_back(pairOfInts(i/dimension, j/dimension));
  for (int jj = j, jjStop = jj + dimension; jj < jjStop; ++jj) {
    for (int ii = i, iiStop = ii + dimension; ii < iiStop; ++ii) {
      newImage->setPixel(ii, jj, newColor);
    }
  }
  while (1) {
    // for each direction...
    while (1) { // right
      while (1) { // left
        while (1) { // down
          while (1) { // up
            const int newJ = j - dimension;
            if (newJ >= 0 && newImage->pixel(i, newJ) == oldColor) {
              for (int jj = newJ, jjStop = newJ + dimension; jj < jjStop;
                   ++jj) {
                for (int ii = i, iiStop = i + dimension; ii < iiStop;
                     ++ii) {
                  newImage->setPixel(ii, jj, newColor);
                }
              }
              j = newJ;
              coordStack.push(pairOfInts(i, j));
              returnSquares.push_back(pairOfInts(i/dimension, j/dimension));
            }
            else {
              break;
            }
          } // end up
          const int newJ = j + dimension;
          if (newJ < height && newImage->pixel(i, newJ) == oldColor) {
            for (int jj = newJ, jjStop = newJ + dimension; jj < jjStop;
                 ++jj) {
              for (int ii = i, iiStop = i + dimension; ii < iiStop; ++ii) {
                newImage->setPixel(ii, jj, newColor);
              }
            }
            j = newJ;
            coordStack.push(pairOfInts(i, j));
            returnSquares.push_back(pairOfInts(i/dimension, j/dimension));
          }
          else {
            break;
          }
        } // end down
        const int newI = i - dimension;
        if (newI >= 0 && newImage->pixel(newI, j) == oldColor) {
          for (int jj = j, jjStop = j + dimension; jj < jjStop; ++jj) {
            for (int ii = newI, iiStop = newI + dimension; ii < iiStop;
                 ++ii) {
              newImage->setPixel(ii, jj, newColor);
            }
          }
          i = newI;
          coordStack.push(pairOfInts(i, j));
          returnSquares.push_back(pairOfInts(i/dimension, j/dimension));
        }
        else {
          break;
        }
      } // end left
      const int newI = i + dimension;
      if (newI < width && newImage->pixel(newI, j) == oldColor) {
        for (int jj = j, jjStop = j + dimension; jj < jjStop; ++jj) {
          for (int ii = newI, iiStop = newI + dimension; ii < iiStop;
               ++ii) {
            newImage->setPixel(ii, jj, newColor);
          }
        }
        i = newI;
        coordStack.push(pairOfInts(i, j));
        returnSquares.push_back(pairOfInts(i/dimension, j/dimension));
      }
      else {
        break;
      }
    } // end right

    const int iLeft = i - dimension;
    const int iRight = i + dimension;
    const int jUp = j - dimension;
    const int jDown = j + dimension;
    if ((iLeft < 0 || newImage->pixel(iLeft, j) != oldColor) &&
        (iRight >= width || newImage->pixel(iRight, j) != oldColor) &&
        (jUp < 0 || newImage->pixel(i, jUp) != oldColor) &&
        (jDown >= height || newImage->pixel(i, jDown) != oldColor)) {
      coordStack.pop();
      if (!coordStack.isEmpty()) {
        i = coordStack.top().x();
        j = coordStack.top().y();
      }
      else {
        break;
      }
    }
  }
  return returnSquares;
}

QVector<triC> findColors(const grid& image, const QVector<triC>& colors) {

  const int width = image.width();
  const int height = image.height();
  QVector<triC> returnColors;
  for (int i = 0, size = colors.size(); i < size; ++i) {
    const triC thisColor = colors[i];
    for (int n = 0; n < height; ++n) {
      for (int m = 0; m < width; ++m) {
        if (image(m, n) == thisColor) {
          goto FOUND;
        }
      }
    }
    returnColors.push_back(thisColor);

  FOUND: {}
  }
  return returnColors;
}
