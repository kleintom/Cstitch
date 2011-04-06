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

#include "dmcList.h"

#include <QtCore/QDebug>

#include "triC.h"
#include "utility.h"

extern const int D_MAX;

QVector<triC> loadDMC() {

  static QVector<triC> dmcColors;
  if (dmcColors.isEmpty()) {
    dmcColors.reserve(DMC_COUNT);
    const QVector<floss> dmcFloss = initializeDMC();
    for (QVector<floss>::const_iterator it = dmcFloss.begin(),
           end = dmcFloss.end(); it != end; ++it) {
      dmcColors.push_back((*it).color());
    }
  }
  return dmcColors;
}

bool colorIsDmc(const triC& color) {

  return loadDMC().contains(color);
}

bool colorsAreDmc(const QVector<triC>& colors) {

  QVector<triC> dmcColors = loadDMC();
  for (int i = 0, size = colors.size(); i < size; ++i) {
    if (!dmcColors.contains(colors[i])) {
      return false;
    }
  }
  return true;
}

QVector<triC> rgbToDmc(const QVector<triC>& rgbColors, bool noDups) {

  QVector<triC> returnColors;
  returnColors.reserve(rgbColors.size());
  const QVector<triC> dmcColors = loadDMC();
  const int dmcSize = dmcColors.size();
  for (int i = 0, size = rgbColors.size(); i < size; ++i) {
    int minDistance = D_MAX;
    int chosenIndex = 0;
    for (int j = 0; j < dmcSize; ++j) {
      int thisDistance = ::ds(rgbColors[i], dmcColors[j]);
      if (thisDistance < minDistance) {
        minDistance = thisDistance;
        chosenIndex = j;
      }
    }
    if (!noDups || !returnColors.contains(dmcColors[chosenIndex])) {
      returnColors.push_back(dmcColors[chosenIndex]);
    }
  }
  return returnColors;
}

triC rgbToDmc(const triC& color) {

  colorOrder order = getColorOrder(color);

  //const int inputIntensity = color.intensity();
  QVector<iColor> compareColors;
  int intensitySpread = 765; //3*255;
  switch (order) {
  case O_RGB:
    compareColors = loadDmcRGB(&intensitySpread);
    break;
  case O_RBG:
    compareColors = loadDmcRBG(&intensitySpread);
    break;
  case O_GRB:
    compareColors = loadDmcGRB(&intensitySpread);
    break;
  case O_GBR:
    compareColors = loadDmcGBR(&intensitySpread);
    break;
  case O_BRG:
    compareColors = loadDmcBRG(&intensitySpread);
    break;
  case O_BGR:
    compareColors = loadDmcBGR(&intensitySpread);
    break;
  case O_GRAY:
    compareColors = loadDmcGray(&intensitySpread);
    break;
  default:
    qWarning() << "Bad order in rgbToDmc:" << order << ::ctos(color);
    compareColors = loadIDMC(&intensitySpread);
  }

  return closestMatch(color, compareColors, intensitySpread);
}

QRgb closestMatch(const triC& color, const QList<QRgb>& colorList) {

  const colorOrder desiredOrder = ::getColorOrder(color);
  QList<triC> triColorList;
  for (int i = 0, size = colorList.size(); i < size; ++i) {
    triColorList.push_back(colorList[i]);
  }

  // first look for a color with the same order that's within a min
  // distance of the input color
  int min = 100;
  int chosenIndex = -1;
  for (int i = 0, size = triColorList.size(); i < size; ++i) {
    if (::getColorOrder(triColorList[i]) == desiredOrder) {
      const int thisD = ::ds(color, triColorList[i]);
      if (thisD < min) {
        min = thisD;
        chosenIndex = i;
      }
    }
  }

  if (chosenIndex == -1) {
    // no good order match, so just choose the closest color
    min = D_MAX;
    chosenIndex = 0;
    for (int i = 0, size = triColorList.size(); i < size; ++i) {
      const int thisD = ::ds(color, triColorList[i]);
      if (thisD < min) {
        min = thisD;
        chosenIndex = i;
      }
    }
  }
  return colorList[chosenIndex];
}

triC closestMatch(const triC& color, const QVector<iColor>& colorList,
                  int intensitySpread) {

  const int inputIntensity = color.intensity();
  const int lowerIBound = inputIntensity - intensitySpread;
  const int upperIBound = inputIntensity + intensitySpread;
  const int lastIndex = colorList.size() - 1;
  Q_ASSERT_X(lowerIBound <= colorList[colorList.size() - 1].intensity(),
             "closestMatch",
             QString(QObject::tr("lowerIBound too large: ") +
                     ::itoqs(lowerIBound) + QString(", ") +
                     ::itoqs(colorList[colorList.size() - 1].intensity())).
             toStdString().c_str());
  Q_ASSERT_X(upperIBound >= colorList[0].intensity(),
             "closestMatch",
             QString(QObject::tr("upperIBound too small: ") +
                     ::itoqs(upperIBound) + ", " +
                     ::itoqs(colorList[0].intensity())).toStdString().c_str());
  // find out where to start looking (modified binary search)
  int startIndex = 0;
  int step = lastIndex/2;
  while (step > 0) {
    // [I assume the compiler only computes (startIndex + step) once here]
    while ((startIndex + step <= lastIndex) &&
           (colorList[startIndex + step].intensity() < lowerIBound)) {
      startIndex = startIndex + step;
    }
    step = step/2;
  }
  // find out where to stop looking
  int endIndex = lastIndex;
  step = lastIndex/2;
  while (step > 0) {
    while ((endIndex - step >= 0) &&
           (colorList[endIndex - step].intensity() > upperIBound)) {
      endIndex = endIndex - step;
    }
    step = step/2;
  }

  int min = D_MAX;
  int chosenIndex = 0;
  for (int i = startIndex; i <= endIndex; ++i) {
    const int thisD = ::ds(color, colorList[i].color());
    if (thisD < min) {
      min = thisD;
      chosenIndex = i;
    }
  }
  return colorList[chosenIndex].color();
}

QVector<iColor> loadDmcGray(int* intensitySpread) {

  static QVector<iColor> dmcGrays;
  if (dmcGrays.isEmpty()) {
    QVector<triC> dmcs = loadDMC();
    for (int i = 0, size = dmcs.size(); i < size; ++i) {
      const triC thisColor(dmcs[i]);
      if (qAbs(thisColor.r() - thisColor.g()) < GRAY_DIFF &&
          qAbs(thisColor.r() - thisColor.b()) < GRAY_DIFF &&
          qAbs(thisColor.g() - thisColor.b()) < GRAY_DIFF) {
        dmcGrays.push_back(iColor(thisColor));
      }
    }
    qSort(dmcGrays.begin(), dmcGrays.end());
//    int minIntensityDistance = 0;
//    const int dmcSize = dmcGrays.size();
//    for (int b = 0; b < 256; ++b) {
//      for (int g = 0; g < 256; ++g) {
//      for (int r = 0; r < 256; ++r) {
//        if (qAbs(r - g) < GRAY_DIFF && qAbs(r - b) < GRAY_DIFF &&
//            qAbs(g - b) < GRAY_DIFF) {
//          int min = D_MAX;
//          int chosenIndex = 0;
//          const triC thisColor(r, g, b);
//          for (int i = 0; i < dmcSize; ++i) {
//            const int thisD = ::ds(thisColor, dmcGrays[i].color());
//            if (thisD < min) {
//              min = thisD;
//              chosenIndex = i;
//            }
//          }
//          const int thisMinIntensityDistance =
//            qAbs(thisColor.intensity() - dmcGrays[chosenIndex].intensity());
//          if (thisMinIntensityDistance > minIntensityDistance) {
//            minIntensityDistance = thisMinIntensityDistance;
//          }
//        }
//      }
//      }
//    }
//    qDebug() << "min intensity distance GRAy" << minIntensityDistance;
  }
  *intensitySpread = 120;
  return dmcGrays;
}

QVector<iColor> loadDmcRGB(int* intensitySpread) {

  static QVector<iColor> dmcRGB;
  if (dmcRGB.isEmpty()) {
    QVector<triC> dmcs = loadDMC();
    for (int i = 0, size = dmcs.size(); i < size; ++i) {
      const triC thisColor(dmcs[i]);
      if (thisColor.r() >= thisColor.g() &&
          thisColor.g() >= thisColor.b()) {
        dmcRGB.push_back(iColor(thisColor));
      }
    }
    qSort(dmcRGB.begin(), dmcRGB.end());
//    // intensity distance from an arbitrary color of type r>=g>=b guaranteed
//    // to include the color in dmcRGB at min distance from the arbitrary one
//    int minIntensityDistance = 0;
//    const int dmcSize = dmcRGB.size();
//    for (int b = 0; b < 256; ++b) {
//      for (int g = b; g < 256; ++g) {
//      for (int r = g; r < 256; ++r) {
//        int min = D_MAX;
//        int chosenIndex = 0;
//        const triC thisColor(r, g, b);
//        for (int i = 0; i < dmcSize; ++i) {
//          const int thisD = ::ds(thisColor, dmcRGB[i].color());
//          if (thisD < min) {
//            min = thisD;
//            chosenIndex = i;
//          }
//        }
//        const int thisMinIntensityDistance =
//          qAbs(thisColor.intensity() - dmcRGB[chosenIndex].intensity());
//        if (thisMinIntensityDistance > minIntensityDistance) {
//          minIntensityDistance = thisMinIntensityDistance;
//        }
//      }
//      }
//    }
//    qDebug() << "min intensity distance RGB" << minIntensityDistance;
  }

  *intensitySpread = 103;
  return dmcRGB;
}

QVector<iColor> loadDmcRBG(int* intensitySpread) {

  static QVector<iColor> dmcRBG;
  if (dmcRBG.isEmpty()) {
    QVector<triC> dmcs = loadDMC();
    for (int i = 0, size = dmcs.size(); i < size; ++i) {
      const triC thisColor(dmcs[i]);
      if (thisColor.r() >= thisColor.b() &&
          thisColor.b() >= thisColor.g()) {
        dmcRBG.push_back(iColor(thisColor));
      }
    }
    qSort(dmcRBG.begin(), dmcRBG.end());
  }
  *intensitySpread = 156;
  return dmcRBG;
}

QVector<iColor> loadDmcGRB(int* intensitySpread) {

  static QVector<iColor> dmcGRB;
  if (dmcGRB.isEmpty()) {
    QVector<triC> dmcs = loadDMC();
    for (int i = 0, size = dmcs.size(); i < size; ++i) {
      const triC thisColor(dmcs[i]);
      if (thisColor.g() >= thisColor.r() &&
          thisColor.r() >= thisColor.b()) {
        dmcGRB.push_back(iColor(thisColor));
      }
    }
    qSort(dmcGRB.begin(), dmcGRB.end());
  }
  *intensitySpread = 215;
  return dmcGRB;
}

QVector<iColor> loadDmcGBR(int* intensitySpread) {

  static QVector<iColor> dmcGBR;
  if (dmcGBR.isEmpty()) {
    QVector<triC> dmcs = loadDMC();
    for (int i = 0, size = dmcs.size(); i < size; ++i) {
      const triC thisColor(dmcs[i]);
      if (thisColor.g() >= thisColor.b() &&
          thisColor.b() >= thisColor.r()) {
        dmcGBR.push_back(iColor(thisColor));
      }
    }
    qSort(dmcGBR.begin(), dmcGBR.end());
  }
  *intensitySpread = 231;
  return dmcGBR;
}

QVector<iColor> loadDmcBRG(int* intensitySpread) {

  static QVector<iColor> dmcBRG;
  if (dmcBRG.isEmpty()) {
    QVector<triC> dmcs = loadDMC();
    for (int i = 0, size = dmcs.size(); i < size; ++i) {
      const triC thisColor(dmcs[i]);
      if (thisColor.b() >= thisColor.r() &&
          thisColor.r() >= thisColor.g()) {
        dmcBRG.push_back(iColor(thisColor));
      }
    }
    qSort(dmcBRG.begin(), dmcBRG.end());
  }
  *intensitySpread = 269;
  return dmcBRG;
}

QVector<iColor> loadDmcBGR(int* intensitySpread) {

  static QVector<iColor> dmcBGR;
  if (dmcBGR.isEmpty()) {
    QVector<triC> dmcs = loadDMC();
    for (int i = 0, size = dmcs.size(); i < size; ++i) {
      const triC thisColor(dmcs[i]);
      if (thisColor.b() >= thisColor.g() &&
          thisColor.g() >= thisColor.r()) {
        dmcBGR.push_back(iColor(thisColor));
      }
    }
    qSort(dmcBGR.begin(), dmcBGR.end());
  }
  *intensitySpread = 209;
  return dmcBGR;
}

QVector<iColor> loadIDMC(int* intensitySpread) {

  QVector<iColor> iDMCs;
  if (iDMCs.isEmpty()) {
    QVector<triC> dmcs = loadDMC();
    for (int i = 0, size = dmcs.size(); i < size; ++i) {
      iDMCs.push_back(iColor(dmcs[i]));
    }
    qSort(dmcs.begin(), dmcs.end());
//    // intensity distance from an arbitrary color of type r>=g>=b guaranteed
//    // to include the color in dmcRGB at min distance from the arbitrary one
//    int minIntensityDistance = 0;
//    const int dmcSize = iDMCs.size();
//    for (int g = 0; g < 256; ++g) {
//      for (int r = 0; r < 256; ++r) {
//      for (int b = 0; b < 256; ++b) {
//        int min = D_MAX;
//        int chosenIndex = 0;
//        const triC thisColor(r, g, b);
//        for (int i = 0; i < dmcSize; ++i) {
//          const int thisD = ::ds(thisColor, iDMCs[i].color());
//          if (thisD < min) {
//            min = thisD;
//            chosenIndex = i;
//          }
//        }
//        const int thisMinIntensityDistance =
//          qAbs(thisColor.intensity() - iDMCs[chosenIndex].intensity());
//        if (thisMinIntensityDistance > minIntensityDistance) {
//          minIntensityDistance = thisMinIntensityDistance;
//        }
//      }
//      }
//    }
//    qDebug() << "min intensity distance DMC" << minIntensityDistance;
  }
  *intensitySpread = 212;
  return iDMCs;
}

QVector<floss> rgbToFloss(const QVector<triC>& rgbColors) {

  const QVector<floss> dmcFloss = initializeDMC();
  QVector<floss> returnColors;
  returnColors.reserve(rgbColors.size());
  QVector<floss>::const_iterator dmcFindI;
  const QVector<floss>::const_iterator dmcFlossIBegin = dmcFloss.begin();
  const QVector<floss>::const_iterator dmcFlossIEnd = dmcFloss.end();
  for (QVector<triC>::const_iterator it = rgbColors.begin(),
          end = rgbColors.end(); it != end; ++it) {
    dmcFindI = std::find(dmcFlossIBegin, dmcFlossIEnd, *it);
    if (dmcFindI != dmcFlossIEnd) {
      returnColors.push_back(*dmcFindI);
    } else {
      // initialize the floss with code -1 but the closest DMC name
      // preceded by a '~'
      const triC closestDMC = rgbToDmc(*it);
      for (QVector<floss>::const_iterator dmcI = dmcFlossIBegin;
           dmcI != dmcFlossIEnd; ++dmcI) {
        if ((*dmcI).color() == closestDMC) {
          const int code = (*dmcI).code();
          QString codeString = "~" + QString::number(code) + ":";
          // dmc doesn't assign codes to some colors, so they were assigned
          // negative numbers, which shouldn't appear externally
          if (code < 0) {
            codeString = "~";
          }
          returnColors.push_back(floss(-1, codeString + (*dmcI).name(),
                                       *it));
          break;
        }
      }
    }
  }
  return returnColors;
}

QVector<floss> initializeDMC() {

  QVector<floss> dmc;
  if (dmc.isEmpty()) {
    dmc.reserve(DMC_COUNT);
/*produced with
cat dmc_color_list.txt| perl -e   '{while(<>) {($c,$n,$r,$g,$b,$h) = \
 m/^(-?[0-9]+)(?: |\t)+([a-zA-Z -.]+)(?: |\t)+([0-9]+)(?: |\t)+([0-9]+)(?: |\t)+([0-9]+)/; \
print "  dmc.push_back(floss($c, \"$n\", triC($r,$g,$b)));\n";}}' > out

//  cat dmc_color_list.txt| perl -e \
//  '{while(<>) {($c,$n,$r,$g,$b,$h) = split(/\t/,$_); \
//  print "  dmc.push_back(floss($c, \"$n\", triC($r,$g,$b)));\n";}}' > out
 */
    dmc.push_back(floss(WHITE_CODE, "White", triC(252,251,248)));
    dmc.push_back(floss(ECRU_CODE, "Ecru", triC(240,234,218)));
    dmc.push_back(floss(000, "blanc White", triC(255,255,255)));
    dmc.push_back(floss(208, "Lavender-VY DK", triC(148,91,128)));
    dmc.push_back(floss(209, "Lavender-DK", triC(206,148,186)));
    dmc.push_back(floss(210, "Lavender-MD", triC(236,207,225)));
    dmc.push_back(floss(211, "Lavender-LT", triC(243,218,228)));
    dmc.push_back(floss(221, "Shell Pink-VY DK", triC(156,41,74)));
    dmc.push_back(floss(223, "Shell Pink-LT", triC(219,128,115)));
    dmc.push_back(floss(224, "Shell Pink-VY LT", triC(255,199,176)));
    dmc.push_back(floss(225, "Shell Pink-ULT VY L", triC(255,240,228)));
    dmc.push_back(floss(300, "Mahogany-VY DK", triC(143,57,38)));
    dmc.push_back(floss(301, "Mahogany-MD", triC(209,102,84)));
    dmc.push_back(floss(304, "Christmas Red-MD", triC(188,0,97)));
    dmc.push_back(floss(307, "Lemon", triC(255,231,109)));
    dmc.push_back(floss(309, "Rose-DP", triC(214,43,91)));
    dmc.push_back(floss(310, "Black", triC(0,0,0)));
    dmc.push_back(floss(311, "Navy Blue-MD", triC(0,79,97)));
    dmc.push_back(floss(312, "Navy Blue-LT", triC(58,84,103)));
    dmc.push_back(floss(315, "Antique Mauve-VY DK", triC(163,90,91)));
    dmc.push_back(floss(316, "Antique Mauve-MD", triC(220,141,141)));
    dmc.push_back(floss(317, "Pewter Grey", triC(167,139,136)));
    dmc.push_back(floss(318, "Steel Grey-LT", triC(197,198,190)));
    dmc.push_back(floss(319, "Pistachio Grn-VY DK", triC(85,95,82)));
    dmc.push_back(floss(320, "Pistachio Green-MD", triC(138,153,120)));
    dmc.push_back(floss(321, "Christmas Red", triC(231,18,97)));
    dmc.push_back(floss(322, "Navy Blue-VY LT", triC(81,109,135)));
    dmc.push_back(floss(326, "Rose-VY DP", triC(188,22,65)));
    dmc.push_back(floss(327, "Violet-DK", triC(61,0,103)));
    dmc.push_back(floss(333, "Blue Violet-VY DK", triC(127,84,130)));
    dmc.push_back(floss(334, "Baby Blue-MD", triC(115,140,170)));
    dmc.push_back(floss(335, "Rose", triC(219,36,79)));
    dmc.push_back(floss(336, "Navy Blue", triC(36,73,103)));
    dmc.push_back(floss(340, "Blue Violet-MD", triC(162,121,164)));
    dmc.push_back(floss(341, "Blue Violet-LT", triC(145,180,197)));
    dmc.push_back(floss(347, "Salmon-VY DK", triC(194,36,67)));
    dmc.push_back(floss(349, "Coral-DK", triC(220,61,91)));
    dmc.push_back(floss(350, "Coral-MD", triC(237,69,90)));
    dmc.push_back(floss(351, "Coral", triC(255,128,135)));
    dmc.push_back(floss(352, "Coral-LT", triC(255,157,144)));
    dmc.push_back(floss(353, "Peach Flesh", triC(255,196,184)));
    dmc.push_back(floss(355, "Terra Cotta-DK", triC(189,73,47)));
    dmc.push_back(floss(356, "Terra Cotta-MD", triC(226,114,91)));
    dmc.push_back(floss(367, "Pistachio Green-DK", triC(95,112,91)));
    dmc.push_back(floss(368, "Pistachio Green-LT", triC(181,206,162)));
    dmc.push_back(floss(369, "Pistachio Grn-VY LT", triC(243,250,209)));
    dmc.push_back(floss(370, "Mustard-MD", triC(184,138,87)));
    dmc.push_back(floss(371, "Mustard", triC(196,155,100)));
    dmc.push_back(floss(372, "Mustard-LT", triC(203,162,107)));
    dmc.push_back(floss(400, "Mahogany-DK", triC(157,60,39)));
    dmc.push_back(floss(402, "Mahogany-VY LT", triC(255,190,164)));
    dmc.push_back(floss(407, "Sportsman Flsh-VY D", triC(194,101,76)));
    dmc.push_back(floss(413, "Pewter Grey-DK", triC(109,95,95)));
    dmc.push_back(floss(414, "Steel Grey-DK", triC(167,139,136)));
    dmc.push_back(floss(415, "Pearl Grey", triC(221,221,218)));
    dmc.push_back(floss(420, "Hazel Nut Brown-DK", triC(140,91,43)));
    dmc.push_back(floss(422, "Hazel Nut Brown-LT", triC(237,172,123)));
    dmc.push_back(floss(433, "Brown-MD", triC(151,84,20)));
    dmc.push_back(floss(434, "Brown-LT", triC(178,103,70)));
    dmc.push_back(floss(435, "Brown-VY LT", triC(187,107,57)));
    dmc.push_back(floss(436, "Tan", triC(231,152,115)));
    dmc.push_back(floss(437, "Tan-LT", triC(238,171,121)));
    dmc.push_back(floss(444, "Lemon-DK", triC(255,176,0)));
    dmc.push_back(floss(445, "Lemon-LT", triC(255,255,190)));
    dmc.push_back(floss(451, "Shell Grey-DK", triC(179,151,143)));
    dmc.push_back(floss(452, "Shell Grey-MD", triC(210,185,175)));
    dmc.push_back(floss(453, "Shell Grey-LT", triC(235,207,185)));
    dmc.push_back(floss(469, "Avocado Green", triC(116,114,92)));
    dmc.push_back(floss(470, "Avocado Green-LT", triC(133,143,108)));
    dmc.push_back(floss(471, "Avocado Green-VY LT", triC(176,187,140)));
    dmc.push_back(floss(472, "Avocado Green-ULT L", triC(238,255,182)));
    dmc.push_back(floss(498, "Christmas Red-LT", triC(187,0,97)));
    dmc.push_back(floss(500, "Blue Green-VY DK", triC(43,57,41)));
    dmc.push_back(floss(501, "Blue Green-DK", triC(67,85,73)));
    dmc.push_back(floss(502, "Blue Green", triC(134,158,134)));
    dmc.push_back(floss(503, "Blue Green-MD", triC(195,206,183)));
    dmc.push_back(floss(504, "Blue Green-LT", triC(206,221,193)));
    dmc.push_back(floss(517, "Wedgewood-MD", triC(16,127,135)));
    dmc.push_back(floss(518, "Wedgewood-LT", triC(102,148,154)));
    dmc.push_back(floss(519, "Sky Blue", triC(194,209,207)));
    dmc.push_back(floss(520, "Fern Green-DK", triC(55,73,18)));
    dmc.push_back(floss(522, "Fern Green", triC(159,169,142)));
    dmc.push_back(floss(523, "Fern Green-LT", triC(172,183,142)));
    dmc.push_back(floss(524, "Fern Green-VY LT", triC(205,182,158)));
    dmc.push_back(floss(535, "Ash Grey-VY LT", triC(85,85,89)));
    dmc.push_back(floss(543, "Beige Brown-UL VY L", triC(239,214,188)));
    dmc.push_back(floss(550, "Violet-VY LT", triC(109,18,97)));
    dmc.push_back(floss(552, "Violet-MD", triC(146,85,130)));
    dmc.push_back(floss(553, "Violet", triC(160,100,146)));
    dmc.push_back(floss(554, "Violet-LT", triC(243,206,225)));
    dmc.push_back(floss(561, "Jade-VY DK", triC(59,96,76)));
    dmc.push_back(floss(562, "Jade-MD", triC(97,134,97)));
    dmc.push_back(floss(563, "Jade-LT", triC(182,212,180)));
    dmc.push_back(floss(564, "Jade-VY LT", triC(214,230,204)));
    dmc.push_back(floss(580, "Moss Green-DK", triC(0,103,0)));
    dmc.push_back(floss(581, "Moss Green", triC(151,152,49)));
    dmc.push_back(floss(597, "Turquoise", triC(128,151,132)));
    dmc.push_back(floss(598, "Turquoise-LT", triC(208,223,205)));
    dmc.push_back(floss(600, "Cranberry-VY DK", triC(208,57,106)));
    dmc.push_back(floss(601, "Cranberry-DK", triC(222,57,105)));
    dmc.push_back(floss(602, "Cranberry-MD", triC(231,84,122)));
    dmc.push_back(floss(603, "Cranberry", triC(255,115,140)));
    dmc.push_back(floss(604, "Cranberry-LT", triC(255,189,202)));
    dmc.push_back(floss(605, "Cranberry-VY LT", triC(255,207,214)));
    dmc.push_back(floss(606, "Bright Orange-Red", triC(255,0,0)));
    dmc.push_back(floss(608, "Bright Orange", triC(255,91,0)));
    dmc.push_back(floss(610, "Drab Brown-VY DK", triC(151,104,84)));
    dmc.push_back(floss(611, "Drab Brown-DK", triC(158,109,91)));
    dmc.push_back(floss(612, "Drab Brown-MD", triC(203,152,103)));
    dmc.push_back(floss(613, "Drab Brown-LT", triC(219,176,122)));
    dmc.push_back(floss(632, "Desert Sand-ULT VY DK", triC(162,77,52)));
    dmc.push_back(floss(640, "Beige Grey-VY DK", triC(163,163,157)));
    dmc.push_back(floss(642, "Beige Grey-DK", triC(174,176,170)));
    dmc.push_back(floss(644, "Beige Grey-MD", triC(224,224,215)));
    dmc.push_back(floss(645, "Beaver Grey-VY DK", triC(113,113,113)));
    dmc.push_back(floss(646, "Beaver Grey-DK", triC(121,121,121)));
    dmc.push_back(floss(647, "Beaver Grey-MD", triC(190,190,185)));
    dmc.push_back(floss(648, "Beaver Grey-LT", triC(202,202,202)));
    dmc.push_back(floss(666, "Christmas Red-LT", triC(213,39,86)));
    dmc.push_back(floss(676, "Old Gold-LT", triC(255,206,158)));
    dmc.push_back(floss(677, "Old Gold-VY LT", triC(255,231,182)));
    dmc.push_back(floss(680, "Old Gold-DK", triC(209,140,103)));
    dmc.push_back(floss(699, "Chirstmas Green", triC(0,91,6)));
    dmc.push_back(floss(700, "Christmas Green-BRT", triC(0,96,47)));
    dmc.push_back(floss(701, "Christmas Green-LT", triC(79,108,69)));
    dmc.push_back(floss(702, "Kelly Green", triC(79,121,66)));
    dmc.push_back(floss(703, "Chartreuse", triC(121,144,76)));
    dmc.push_back(floss(704, "Chartreuse-BRT", triC(165,164,103)));
    dmc.push_back(floss(712, "Cream", triC(245,240,219)));
    dmc.push_back(floss(718, "Plum", triC(219,55,121)));
    dmc.push_back(floss(720, "Orange Spice-DK", triC(200,36,43)));
    dmc.push_back(floss(721, "Orange Spice-MD", triC(255,115,97)));
    dmc.push_back(floss(722, "Orange Spice-LT", triC(255,146,109)));
    dmc.push_back(floss(725, "Topaz", triC(255,200,124)));
    dmc.push_back(floss(726, "Topaz-LT", triC(255,224,128)));
    dmc.push_back(floss(727, "Topaz-VY LT", triC(255,235,168)));
    dmc.push_back(floss(729, "Old Gold-MD", triC(243,176,128)));
    dmc.push_back(floss(730, "Olive Green-VY DK", triC(132,102,0)));
    dmc.push_back(floss(731, "Olive Green-DK", triC(140,103,0)));
    dmc.push_back(floss(732, "Olive Green", triC(145,104,0)));
    dmc.push_back(floss(733, "Olive Green-MD", triC(206,155,97)));
    dmc.push_back(floss(734, "Olive Green-LT", triC(221,166,107)));
    dmc.push_back(floss(738, "Tan-VY LT", triC(244,195,139)));
    dmc.push_back(floss(739, "Tan-ULT VY LT", triC(244,233,202)));
    dmc.push_back(floss(740, "Tangerine", triC(255,131,19)));
    dmc.push_back(floss(741, "Tangerine-MD", triC(255,142,4)));
    dmc.push_back(floss(742, "Tangerine-LT", triC(255,183,85)));
    dmc.push_back(floss(743, "Yellow-MD", triC(255,230,146)));
    dmc.push_back(floss(744, "Yellow-PALE", triC(255,239,170)));
    dmc.push_back(floss(745, "Yellow-LT PALE", triC(255,240,197)));
    dmc.push_back(floss(746, "Off White", triC(246,234,219)));
    dmc.push_back(floss(747, "Sky Blue-VY LT", triC(240,247,239)));
    dmc.push_back(floss(754, "Peach Flesh-LT", triC(251,227,209)));
    dmc.push_back(floss(758, "Terra Cotta-VY LT", triC(255,177,147)));
    dmc.push_back(floss(760, "Salmon", triC(249,160,146)));
    dmc.push_back(floss(761, "Salmon-LT", triC(255,201,188)));
    dmc.push_back(floss(762, "Pearl Grey-VY LT", triC(232,232,229)));
    dmc.push_back(floss(772, "Pine Green--LT", triC(231,249,203)));
    dmc.push_back(floss(775, "Baby Blue-VY LT", triC(247,246,248)));
    dmc.push_back(floss(776, "Pink-MD", triC(255,177,174)));
    dmc.push_back(floss(778, "Antique Mauve-VY LT", triC(255,199,184)));
    dmc.push_back(floss(780, "Topaz-ULT VY DK", triC(181,98,46)));
    dmc.push_back(floss(781, "Topaz-VY DK", triC(181,107,56)));
    dmc.push_back(floss(782, "Topaz-DK", triC(204,119,66)));
    dmc.push_back(floss(783, "Topaz-MD", triC(225,146,85)));
    dmc.push_back(floss(791, "Cornflower Blue-VYD", triC(71,55,93)));
    dmc.push_back(floss(792, "Cornflower Blue-DK", triC(97,97,128)));
    dmc.push_back(floss(793, "Cornflower Blue-MD", triC(147,139,164)));
    dmc.push_back(floss(794, "Cornflower Blue-LT", triC(187,208,218)));
    dmc.push_back(floss(796, "Royal Blue-DK", triC(30,58,95)));
    dmc.push_back(floss(797, "Royal Blue", triC(30,66,99)));
    dmc.push_back(floss(798, "Delft-DK", triC(103,115,141)));
    dmc.push_back(floss(799, "Delft-MD", triC(132,156,182)));
    dmc.push_back(floss(800, "Delft-PALE", triC(233,238,233)));
    dmc.push_back(floss(801, "Coffee Brown-DK", triC(123,71,20)));
    dmc.push_back(floss(806, "Peacock Blue-DK", triC(30,130,133)));
    dmc.push_back(floss(807, "Peacock Blue", triC(128,167,160)));
    dmc.push_back(floss(809, "Delft", triC(190,193,205)));
    dmc.push_back(floss(813, "Blue-LT", triC(175,195,205)));
    dmc.push_back(floss(814, "Garnet-DK", triC(162,0,88)));
    dmc.push_back(floss(815, "Garnet-MD", triC(166,0,91)));
    dmc.push_back(floss(816, "Garnet", triC(179,0,91)));
    dmc.push_back(floss(817, "Coral Red-VY DK", triC(219,24,85)));
    dmc.push_back(floss(818, "Baby Pink", triC(255,234,235)));
    dmc.push_back(floss(819, "Baby Pink-LT", triC(248,247,221)));
    dmc.push_back(floss(820, "Royal Blue-VY DK", triC(30,54,85)));
    dmc.push_back(floss(822, "Beige Grey-LT", triC(242,234,219)));
    dmc.push_back(floss(823, "Navy Blue-DK", triC(0,0,73)));
    dmc.push_back(floss(824, "Blue-VY DK", triC(71,97,116)));
    dmc.push_back(floss(825, "Blue-DK", triC(85,108,128)));
    dmc.push_back(floss(826, "Blue-MD", triC(115,138,153)));
    dmc.push_back(floss(827, "Blue-VY LT", triC(213,231,232)));
    dmc.push_back(floss(828, "Blue-ULT VY LT", triC(237,247,238)));
    dmc.push_back(floss(829, "Golden Olive-VY DK", triC(130,90,8)));
    dmc.push_back(floss(830, "Golden Olive-DK", triC(136,95,18)));
    dmc.push_back(floss(831, "Golden Olive-MD", triC(144,103,18)));
    dmc.push_back(floss(832, "Golden Olive", triC(178,119,55)));
    dmc.push_back(floss(833, "Golden Olive-LT", triC(219,182,128)));
    dmc.push_back(floss(834, "Golden Olive-VY LT", triC(242,209,142)));
    dmc.push_back(floss(838, "Beige Brown-VY DK", triC(94,56,27)));
    dmc.push_back(floss(839, "Beige Brown-DK", triC(109,66,39)));
    dmc.push_back(floss(840, "Beige Brown-MD", triC(128,85,30)));
    dmc.push_back(floss(841, "Beige Brown-LT", triC(188,134,107)));
    dmc.push_back(floss(842, "Beige Brown-VY LT", triC(219,194,164)));
    dmc.push_back(floss(844, "Beaver Brown -ULT D", triC(107,103,102)));
    // DUP! dmc.push_back(floss(868, "Hazel Nut Brown-VYD", triC(153,92,48)));
    dmc.push_back(floss(869, "Hazel Nut Brn-VY DK", triC(153,92,48)));
    dmc.push_back(floss(890, "Pistachio Grn-ULT D", triC(79,86,76)));
    dmc.push_back(floss(891, "Carnation-DK", triC(241,49,84)));
    dmc.push_back(floss(892, "Carnation-MD", triC(249,90,97)));
    dmc.push_back(floss(893, "Carnation-LT", triC(243,149,157)));
    dmc.push_back(floss(894, "Carnation-VY LT", triC(255,194,191)));
    dmc.push_back(floss(895, "Hunter Green-VY DK", triC(89,92,78)));
    dmc.push_back(floss(898, "Coffee Brown-VY DK", triC(118,55,19)));
    dmc.push_back(floss(899, "Rose-MD", triC(233,109,115)));
    dmc.push_back(floss(900, "Burnt Orange-DK", triC(206,43,0)));
    dmc.push_back(floss(902, "Granet-VY DK", triC(138,24,77)));
    dmc.push_back(floss(904, "Parrot Green-VY DK", triC(78,95,57)));
    dmc.push_back(floss(905, "Parrot Green-DK", triC(98,119,57)));
    dmc.push_back(floss(906, "Parrot Green-MD", triC(143,163,89)));
    dmc.push_back(floss(907, "Parrot Green-LT", triC(185,200,102)));
    dmc.push_back(floss(909, "Emerald Green-VY DK", triC(49,105,85)));
    dmc.push_back(floss(910, "Emerald Green-DK", triC(48,116,91)));
    dmc.push_back(floss(911, "Emerald Green-MD", triC(49,128,97)));
    dmc.push_back(floss(912, "Emerald Green-LT", triC(115,158,115)));
    dmc.push_back(floss(913, "Nile Green-MD", triC(153,188,149)));
    dmc.push_back(floss(915, "Plum-DK", triC(170,24,91)));
    dmc.push_back(floss(917, "Plum-MD", triC(171,22,95)));
    dmc.push_back(floss(918, "Red Copper-DK", triC(168,68,76)));
    dmc.push_back(floss(919, "Red Copper", triC(180,75,82)));
    dmc.push_back(floss(920, "Copper-MD", triC(197,94,88)));
    dmc.push_back(floss(921, "Copper", triC(206,103,91)));
    dmc.push_back(floss(922, "Copper-LT", triC(237,134,115)));
    dmc.push_back(floss(924, "Grey Green--VY DK", triC(86,99,100)));
    dmc.push_back(floss(926, "Grey Green-LT", triC(96,116,115)));
    dmc.push_back(floss(927, "Grey Green-LT", triC(200,198,194)));
    dmc.push_back(floss(928, "Grey Green--VY LT", triC(225,224,216)));
    dmc.push_back(floss(930, "Antique Blue-DK", triC(102,122,140)));
    dmc.push_back(floss(931, "Antique Blue-MD", triC(124,135,145)));
    dmc.push_back(floss(932, "Antique Blue-LT", triC(182,186,194)));
    dmc.push_back(floss(934, "Black Avocado Green", triC(62,59,40)));
    dmc.push_back(floss(935, "Avocado Green-DK", triC(67,63,47)));
    dmc.push_back(floss(936, "Avocado Green--VY D", triC(69,69,49)));
    dmc.push_back(floss(937, "Avocado Green-MD", triC(73,86,55)));
    dmc.push_back(floss(938, "Coffee Brown-ULT DK", triC(99,39,16)));
    dmc.push_back(floss(939, "Navy Blue-Vy DK", triC(0,0,49)));
    dmc.push_back(floss(943, "Aquamarine-MD", triC(0,162,117)));
    dmc.push_back(floss(945, "Flesh-MD", triC(255,206,164)));
    dmc.push_back(floss(946, "Burnt Orange-MD", triC(244,73,0)));
    dmc.push_back(floss(947, "Burnt Orange", triC(255,91,0)));
    dmc.push_back(floss(948, "Peach Flesh-VY LT", triC(255,243,231)));
    dmc.push_back(floss(950, "Sportsman Flesh", triC(239,162,127)));
    dmc.push_back(floss(951, "Flesh", triC(255,229,188)));
    dmc.push_back(floss(954, "Nile Green", triC(170,213,164)));
    dmc.push_back(floss(955, "Nile Green-LT", triC(214,230,204)));
    dmc.push_back(floss(956, "Geranium", triC(255,109,115)));
    dmc.push_back(floss(957, "Gernanium-PALE", triC(255,204,208)));
    dmc.push_back(floss(958, "Sea Green-DK", triC(0,160,130)));
    dmc.push_back(floss(959, "Sea Green-MD", triC(171,206,177)));
    dmc.push_back(floss(961, "Dusty Rose-DK", triC(243,108,123)));
    dmc.push_back(floss(962, "Dusty Rose-MD", triC(253,134,141)));
    dmc.push_back(floss(963, "Dusty Rose-ULT VY L", triC(255,233,233)));
    dmc.push_back(floss(964, "Sea Green-LT", triC(208,224,210)));
    dmc.push_back(floss(966, "Baby Green-MD", triC(206,213,176)));
    dmc.push_back(floss(970, "Pumpkin-LT", triC(255,117,24)));
    dmc.push_back(floss(971, "Pumpkin", triC(255,106,0)));
    dmc.push_back(floss(972, "Canary-DP", triC(255,146,0)));
    dmc.push_back(floss(973, "Canary-BRT", triC(255,194,67)));
    dmc.push_back(floss(975, "Golden Brown-DK", triC(158,67,18)));
    dmc.push_back(floss(976, "Golden Brown-MD", triC(246,141,57)));
    dmc.push_back(floss(977, "Golden Brown-LT", triC(255,164,73)));
    dmc.push_back(floss(986, "Forest Green-VY DK", triC(58,82,65)));
    dmc.push_back(floss(987, "Forest Green-DK", triC(83,97,73)));
    dmc.push_back(floss(988, "Forest Green-MD", triC(134,145,110)));
    dmc.push_back(floss(989, "Forest Green", triC(134,153,110)));
    dmc.push_back(floss(991, "Aquamarine-DK", triC(47,91,73)));
    dmc.push_back(floss(992, "Aquamarine", triC(146,183,165)));
    dmc.push_back(floss(993, "Aquamarine-LT", triC(192,224,200)));
    dmc.push_back(floss(995, "Electric Blue-DK", triC(0,123,134)));
    dmc.push_back(floss(996, "Electric Blue-MD", triC(170,222,225)));
    dmc.push_back(floss(3011, "Khaki Green-DK", triC(123,91,64)));
    dmc.push_back(floss(3012, "Khaki Green-MD", triC(170,134,103)));
    dmc.push_back(floss(3013, "Khaki Green-LT", triC(208,195,164)));
    dmc.push_back(floss(3021, "Brown Grey-VY DK", triC(115,91,93)));
    dmc.push_back(floss(3022, "Brown Grey-MD", triC(172,172,170)));
    dmc.push_back(floss(3023, "Brown Grey-LT", triC(198,190,173)));
    dmc.push_back(floss(3024, "Brown Grey-VY LT", triC(210,208,205)));
    dmc.push_back(floss(3031, "Mocha Brown-VY DK", triC(84,56,23)));
    dmc.push_back(floss(3032, "Mocha Brown-MD", triC(188,156,120)));
    dmc.push_back(floss(3033, "Mocha Brown-VY LT", triC(239,219,190)));
    dmc.push_back(floss(3041, "Antique Violet-MD", triC(190,155,167)));
    dmc.push_back(floss(3042, "Antique Violet-LT", triC(225,205,200)));
    dmc.push_back(floss(3045, "Yellow Beige-DK", triC(216,151,105)));
    dmc.push_back(floss(3046, "Yellow Beige-MD", triC(229,193,139)));
    dmc.push_back(floss(3047, "Yellow Beige-LT", triC(255,236,211)));
    dmc.push_back(floss(3051, "Green Grey-DK", triC(85,73,0)));
    dmc.push_back(floss(3052, "Green Grey--MD", triC(137,141,114)));
    dmc.push_back(floss(3053, "Green Grey", triC(187,179,148)));
    dmc.push_back(floss(3064, "Sportsman Flsh-VY D", triC(194,101,76)));
    dmc.push_back(floss(3072, "Beaver Grey-VY LT", triC(233,233,223)));
    dmc.push_back(floss(3078, "Golden Yellow-VY LT", triC(255,255,220)));
    dmc.push_back(floss(3325, "Baby Blue-LT", triC(202,226,229)));
    dmc.push_back(floss(3326, "Rose-LT", triC(255,157,150)));
    dmc.push_back(floss(3328, "Salmon-DK", triC(188,64,85)));
    dmc.push_back(floss(3340, "Apricot-MD", triC(255,123,103)));
    dmc.push_back(floss(3341, "Apricot", triC(255,172,162)));
    dmc.push_back(floss(3345, "Hunter Green-DK", triC(97,100,82)));
    dmc.push_back(floss(3346, "Hunter Green", triC(120,134,107)));
    dmc.push_back(floss(3347, "Yellow Green-MD", triC(128,152,115)));
    dmc.push_back(floss(3348, "Yellow Green-LT", triC(225,249,190)));
    dmc.push_back(floss(3350, "Dusty Rose-ULT DK", triC(201,79,91)));
    dmc.push_back(floss(3354, "Dusty Rose-LT", triC(255,214,209)));
    dmc.push_back(floss(3362, "Pine Green-DK", triC(96,95,84)));
    dmc.push_back(floss(3363, "Pine Green-MD", triC(116,127,96)));
    dmc.push_back(floss(3364, "Pine Green", triC(161,167,135)));
    dmc.push_back(floss(3371, "Black Brown", triC(83,37,16)));
    dmc.push_back(floss(3607, "Plum-LT", triC(231,79,134)));
    dmc.push_back(floss(3608, "Plum-VY LT", triC(247,152,182)));
    dmc.push_back(floss(3609, "Plum-ULT LT", triC(255,214,229)));
    dmc.push_back(floss(3685, "Mauve-DK", triC(161,53,79)));
    dmc.push_back(floss(3687, "Mauve", triC(203,78,97)));
    dmc.push_back(floss(3688, "Mauve-MD", triC(250,151,144)));
    dmc.push_back(floss(3689, "Mauve-LT", triC(255,213,216)));
    dmc.push_back(floss(3705, "Melon-DK", triC(255,85,91)));
    dmc.push_back(floss(3706, "Melon-MD", triC(255,128,109)));
    dmc.push_back(floss(3708, "Melon-LT", triC(254,212,219)));
    dmc.push_back(floss(3712, "Salmon-MD", triC(230,101,107)));
    dmc.push_back(floss(3713, "Salmon-VY LT", triC(253,229,217)));
    dmc.push_back(floss(3716, "Dusty Rose-VY LT", triC(255,211,212)));
    dmc.push_back(floss(3721, "Shell Pink-DK", triC(184,75,77)));
    dmc.push_back(floss(3722, "Shell Pink-MD", triC(184,89,88)));
    dmc.push_back(floss(3726, "Antique Mauve-DK", triC(195,118,123)));
    dmc.push_back(floss(3727, "Antique Mauve-LT", triC(255,199,196)));
    dmc.push_back(floss(3731, "Dusty Rose-VY DK", triC(209,93,103)));
    dmc.push_back(floss(3733, "Dusty Rose", triC(255,154,148)));
    dmc.push_back(floss(3740, "Antique Violet-DK", triC(156,125,133)));
    dmc.push_back(floss(3743, "Antique Violet-VY L", triC(235,235,231)));
    dmc.push_back(floss(3746, "Blue Violet-DK", triC(149,102,162)));
    dmc.push_back(floss(3747, "Blue Violet-VY LT", triC(230,236,232)));
    dmc.push_back(floss(3750, "Antique Blue-VY DK", triC(12,91,108)));
    dmc.push_back(floss(3752, "Antique Blue-VY LT", triC(194,209,206)));
    dmc.push_back(floss(3753, "Ant. Blue-ULT VY LT", triC(237,247,247)));
    dmc.push_back(floss(3755, "Baby Blue", triC(158,176,206)));
    dmc.push_back(floss(3756, "Baby Blue-ULT VY LT", triC(248,248,252)));
    dmc.push_back(floss(3760, "Wedgewood", triC(102,142,152)));
    dmc.push_back(floss(3761, "Sky Blue-LT", triC(227,234,230)));
    dmc.push_back(floss(3765, "Peacock Blue-VY DK", triC(24,128,134)));
    dmc.push_back(floss(3766, "Peacock Blue-LT", triC(24,101,111)));
    dmc.push_back(floss(3768, "Grey Green-DK", triC(92,110,108)));
    dmc.push_back(floss(3770, "Flesh-VY LT", triC(255,250,224)));
    dmc.push_back(floss(3772, "Desert Sand-VY DK", triC(173,83,62)));
    dmc.push_back(floss(3773, "Sportsman Flsh-MD", triC(231,134,103)));
    dmc.push_back(floss(3774, "Sportsman Flsh-VY L", triC(255,220,193)));
    dmc.push_back(floss(3776, "Mahogony-LT", triC(221,109,91)));
    dmc.push_back(floss(3777, "Terra Cotta-VY DK", triC(191,64,36)));
    dmc.push_back(floss(3778, "Terra Cotta-LT", triC(237,122,100)));
    dmc.push_back(floss(3779, "Ter. Cotta-ULT VY L", triC(255,177,152)));
    dmc.push_back(floss(3781, "Mocha Brown-DK", triC(113,71,42)));
    dmc.push_back(floss(3782, "Mocho Brown-LT", triC(206,175,144)));
    dmc.push_back(floss(3787, "Brown Grey-DK", triC(139,109,115)));
    dmc.push_back(floss(3790, "Beige Grey-ULT DK", triC(140,117,109)));
    dmc.push_back(floss(3799, "Pewter Grey-VY DK", triC(81,76,83)));
    dmc.push_back(floss(3801, "V DK Melon", triC(231,73,103)));
    dmc.push_back(floss(3802, "V DK Antique Mauve", triC(113,65,73)));
    dmc.push_back(floss(3803, "DK Mauve", triC(171,51,87)));
    dmc.push_back(floss(3804, "DK Cyclamen Pink", triC(224,40,118)));
    dmc.push_back(floss(3805, "Cyclamen Pink", triC(243,71,139)));
    dmc.push_back(floss(3806, "LT Cyclamen Pink", triC(255,140,174)));
    dmc.push_back(floss(3807, "Cornflower Blue", triC(96,103,140)));
    dmc.push_back(floss(3808, "Ultra V DK Turquoise", triC(54,105,112)));
    dmc.push_back(floss(3809, "V DK Turquoise", triC(63,124,133)));
    dmc.push_back(floss(3810, "DK Turquoise", triC(72,142,154)));
    dmc.push_back(floss(3811, "V LT Turquoise", triC(188,227,230)));
    dmc.push_back(floss(3812, "V DK Seagreen", triC(47,140,132)));
    dmc.push_back(floss(3813, "LT Blue Green", triC(178,212,189)));
    dmc.push_back(floss(3814, "Aquamarine", triC(80,139,125)));
    dmc.push_back(floss(3815, "DK Celadon Green", triC(71,119,89)));
    dmc.push_back(floss(3816, "Celadon Green", triC(101,165,125)));
    dmc.push_back(floss(3817, "LT Celadon Green", triC(153,195,170)));
    dmc.push_back(floss(3818, "Ultra V DK Emerald Greene", triC(17,90,59)));
    dmc.push_back(floss(3819, "LT Moss Green", triC(224,232,104)));
    dmc.push_back(floss(3820, "DK Straw", triC(223,182,95)));
    dmc.push_back(floss(3821, "Straw", triC(243,206,117)));
    dmc.push_back(floss(3822, "LT Straw", triC(246,220,152)));
    dmc.push_back(floss(3823, "Ultra Pale Yellow", triC(255,253,227)));
    dmc.push_back(floss(3824, "LT Apricot", triC(254,205,194)));
    dmc.push_back(floss(3825, "Pale Pumpkin", triC(253,189,150)));
    dmc.push_back(floss(3826, "Golden Brown", triC(173,114,57)));
    dmc.push_back(floss(3827, "Pale Golden Brown", triC(247,187,119)));
    dmc.push_back(floss(3828, "Hazelnut Brown", triC(183,139,97)));
    dmc.push_back(floss(3829, "V DK Old Gold", triC(169,130,4)));
    dmc.push_back(floss(3830, "Terra Cotta", triC(188,85,68)));
    dmc.push_back(floss(3831, "DK Raspberry", triC(179,47,72)));
    dmc.push_back(floss(3832, "MD Raspberry", triC(219,85,110)));
    dmc.push_back(floss(3833, "LT Raspberry", triC(234,134,153)));
    dmc.push_back(floss(3834, "DK Grape", triC(114,55,93)));
    dmc.push_back(floss(3835, "MD Grape", triC(148,96,131)));
    dmc.push_back(floss(3836, "LT Grape", triC(186,145,170)));
    dmc.push_back(floss(3837, "Ultra DK Lavender", triC(108,58,110)));
    dmc.push_back(floss(3838, "DK Lavender Blue", triC(92,114,148)));
    dmc.push_back(floss(3839, "MD Lavender Blue", triC(123,142,171)));
    dmc.push_back(floss(3840, "LT Lavender Blue", triC(176,192,218)));
    dmc.push_back(floss(3841, "Pale Baby Blue", triC(205,223,237)));
    dmc.push_back(floss(3842, "DK Wedgwood", triC(50,102,124)));
    dmc.push_back(floss(3843, "Electric Blue", triC(20,170,208)));
    dmc.push_back(floss(3844, "DK Bright Turquoise", triC(18,174,186)));
    dmc.push_back(floss(3845, "MD Bright Turquoise", triC(4,196,202)));
    dmc.push_back(floss(3846, "LT Bright Turquoise", triC(6,227,230)));
    dmc.push_back(floss(3847, "DK Teal Green", triC(52,125,117)));
    dmc.push_back(floss(3848, "MD Teal Green", triC(85,147,146)));
    dmc.push_back(floss(3849, "LT Teal Green", triC(82,179,164)));
    dmc.push_back(floss(3850, "DK Bright Green", triC(55,132,119)));
    dmc.push_back(floss(3851, "LT Bright Green", triC(73,179,161)));
    dmc.push_back(floss(3852, "V DK Straw", triC(205,157,55)));
    dmc.push_back(floss(3853, "DK Autumn Gold", triC(242,151,70)));
    dmc.push_back(floss(3854, "MD Autumn Gold", triC(242,175,104)));
    dmc.push_back(floss(3855, "LT Autumn Gold", triC(250,211,150)));
    dmc.push_back(floss(3856, "Ultra V LT Mahogany", triC(255,211,181)));
    dmc.push_back(floss(3857, "DK Rosewood", triC(104,37,26)));
    dmc.push_back(floss(3858, "MD Rosewood", triC(150,74,63)));
    dmc.push_back(floss(3859, "LT Rosewood", triC(186,139,124)));
    dmc.push_back(floss(3860, "Cocoa", triC(125,93,87)));
    dmc.push_back(floss(3861, "LT Cocoa", triC(166,136,129)));
    dmc.push_back(floss(3862, "DK Mocha Beige", triC(138,110,78)));
    dmc.push_back(floss(3863, "MD Mocha Beige", triC(164,131,92)));
    dmc.push_back(floss(3864, "LT Mocha Beige", triC(203,182,156)));
    dmc.push_back(floss(3865, "Winter White", triC(249,247,241)));
    dmc.push_back(floss(3866, "Ultra V LT Mocha Brown", triC(250,246,240)));
  }

  return dmc;

}
