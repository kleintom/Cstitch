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

#include "colorLists.h"

#include <QtCore/QDebug>

#include "triC.h"
#include "utility.h"
#include "imageProcessing.h"

extern const int D_MAX;

QHash<colorOrder, QVector<iColor> > colorMatcher::dmcColorsHash_ =
  QHash<colorOrder, QVector<iColor> >();
QHash<colorOrder, QVector<iColor> > colorMatcher::anchorColorsHash_ =
  QHash<colorOrder, QVector<iColor> >();
QHash<colorOrder, int> colorMatcher::dmcIntensitySpreads_ = 
  QHash<colorOrder, int>();
QHash<colorOrder, int> colorMatcher::anchorIntensitySpreads_ = 
  QHash<colorOrder, int>();

void colorMatcher::initializeIntensitySpreads() {

  dmcIntensitySpreads_[O_GRAY] = 120;
  dmcIntensitySpreads_[O_RGB] = 103;
  dmcIntensitySpreads_[O_RBG] = 156;
  dmcIntensitySpreads_[O_GRB] = 215;
  dmcIntensitySpreads_[O_GBR] = 231;
  dmcIntensitySpreads_[O_BGR] = 269;
  dmcIntensitySpreads_[O_BRG] = 209;
//  int minIntensityDistance = 0;
//  const QVector<triC> anchors = loadAnchor();
//  brgComparator comparator;
//  QVector<triC> colors;
//  for (int i = 0, size = anchors.size(); i < size; ++i) {
//    if (comparator(anchors[i])) {
//      qDebug() << ::ctos(anchors[i]);
//      colors.push_back(anchors[i]);          
//    }
//  }
//  const int size = colors.size();
//  for (int b = 0; b < 256; ++b) {
//    for (int g = 0; g < 256; ++g) {
//      for (int r = 0; r < 256; ++r) {
//        const triC thisColor(r, g, b);
//        if (comparator(thisColor)) {
//          // find the closest match on colors
//          int min = D_MAX;
//          int chosenIndex = 0;
//          for (int i = 0; i < size; ++i) {
//            const int thisD = ::ds(thisColor, colors[i]);
//            if (thisD < min) {
//              min = thisD;
//              chosenIndex = i;
//            }
//          }
//          const int thisMinIntensityDistance =
//            qAbs(thisColor.intensity() - colors[chosenIndex].intensity());
//          if (thisMinIntensityDistance > minIntensityDistance) {
//            minIntensityDistance = thisMinIntensityDistance;
//          }
//        }
//      }
//    }
//  }
//  qDebug() << "min intensity distance" << minIntensityDistance;
  anchorIntensitySpreads_[O_GRAY] = 104;
  anchorIntensitySpreads_[O_RGB] = 96;
  anchorIntensitySpreads_[O_RBG] = 173;
  anchorIntensitySpreads_[O_GRB] = 175;
  anchorIntensitySpreads_[O_GBR] = 175;
  anchorIntensitySpreads_[O_BGR] = 175;
  anchorIntensitySpreads_[O_BRG] = 263;
}

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

QVector<triC> loadAnchor() {

  static QVector<triC> anchorColors;
  if (anchorColors.isEmpty()) {
    anchorColors.reserve(ANCHOR_COUNT);
    const QVector<floss> anchorFloss = initializeAnchor();
    for (int i = 0, size = anchorFloss.size(); i < size; ++i) {
      anchorColors.push_back(anchorFloss[i].color());
    }
  }
  return anchorColors;
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

QVector<triC> rgbToDmc(const QVector<triC>& colors, bool noDups) {

  colorTransformer* transformer = new dmcTransformer();
  return rgbToColorList(colors, colorTransformerPtr(transformer), noDups);
}

QVector<triC> rgbToAnchor(const QVector<triC>& colors, bool noDups) {

  colorTransformer* transformer = new anchorTransformer();
  return rgbToColorList(colors, colorTransformerPtr(transformer), noDups);
}

QVector<triC> rgbToColorList(const QVector<triC>& rgbColors,
                             const colorTransformerPtr& transformer,
                             bool noDups) {

  QVector<triC> returnColors;
  returnColors.reserve(rgbColors.size());
  for (int i = 0, size = rgbColors.size(); i < size; ++i) {
    const triC newColor = transformer->transform(rgbColors[i]);    
    if (!noDups || !returnColors.contains(newColor)) {
      returnColors.push_back(newColor);
    }
  }
  return returnColors;
}

triC rgbToDmc(const triC& color) {

  const colorMatcher matcher(flossDMC, color);
  return matcher.closestMatch();
}

triC rgbToAnchor(const triC& color) {

  const colorMatcher matcher(flossAnchor, color);
  return matcher.closestMatch();
}

colorMatcher::colorMatcher(flossType type, const triC& color)
  : color_(color) {

  const colorOrder order = getColorOrder(color);
  orderComparator* comparator = NULL;
  switch (order) {
  case O_RGB:
    comparator = new rgbComparator();
    break;
  case O_RBG:
    comparator = new rbgComparator();
    break;
  case O_GRB:
    comparator = new grbComparator();
    break;
  case O_GBR:
    comparator = new gbrComparator();
    break;
  case O_BGR:
    comparator = new bgrComparator();
    break;
  case O_BRG:
    comparator = new brgComparator();
    break;
  case O_GRAY:
    comparator = new grayComparator();
    break;
  }

  switch (type.value()) {
  case flossDMC:
    colorList_ = dmcColorsHash_[order];
    if (colorList_.isEmpty()) {
      QVector<iColor> newList;
      loadColorList(comparator, loadDMC(), &newList);
      dmcColorsHash_[order] = newList;
      colorList_ = newList;
    }
    intensitySpread_ = dmcIntensitySpreads_[order];
    break;
  case flossAnchor:
    colorList_ = anchorColorsHash_[order];
    if (colorList_.isEmpty()) {
      QVector<iColor> newList;
      loadColorList(comparator, loadAnchor(), &newList);
      anchorColorsHash_[order] = newList;
      colorList_ = anchorColorsHash_[order];
    }
    intensitySpread_ = anchorIntensitySpreads_[order];
    break;
  default:
    qWarning() << "Unknown floss type in colorMatcher" << type.value();
    colorList_ = dmcColorsHash_[order];
    if (colorList_.isEmpty()) {
      QVector<iColor> newList;
      loadColorList(comparator, loadDMC(), &newList);
      dmcColorsHash_[order] = newList;
      colorList_ = dmcColorsHash_[order];
    }
    intensitySpread_ = dmcIntensitySpreads_[order];
    break;
  }
  delete comparator;
}

void colorMatcher::loadColorList(const orderComparator* comparator,
                                 const QVector<triC>& colors,
                                 QVector<iColor>* newList) const {

  for (int i = 0, size = colors.size(); i < size; ++i) {
    const triC thisColor(colors[i]);
    if ((*comparator)(thisColor)) {
      newList->push_back(iColor(thisColor));
    }
  }
  std::sort(newList->begin(), newList->end());
}

triC colorMatcher::closestMatch() const {

  const int inputIntensity = color_.intensity();
  const int lowerIBound = inputIntensity - intensitySpread_;
  const int upperIBound = inputIntensity + intensitySpread_;
  const int lastIndex = colorList_.size() - 1;
  Q_ASSERT_X(lowerIBound <= colorList_[lastIndex].intensity(),
             "closestMatch",
             QString("lowerIBound too large: " +
                     ::itoqs(lowerIBound) + QString(", ") +
                     ::itoqs(colorList_[lastIndex].intensity())).
             toStdString().c_str());
  Q_ASSERT_X(upperIBound >= colorList_[0].intensity(),
             "closestMatch",
             QString("upperIBound too small: " +
                     ::itoqs(upperIBound) + ", " +
                     ::itoqs(colorList_[0].intensity())).toStdString().c_str());
  // find out where to start looking (modified binary search)
  int startIndex = 0;
  int step = lastIndex/2;
  while (step > 0) {
    // [I assume the compiler only computes (startIndex + step) once here]
    while ((startIndex + step <= lastIndex) &&
           (colorList_[startIndex + step].intensity() < lowerIBound)) {
      startIndex = startIndex + step;
    }
    step = step/2;
  }
  // find out where to stop looking
  int endIndex = lastIndex;
  step = lastIndex/2;
  while (step > 0) {
    while ((endIndex - step >= 0) &&
           (colorList_[endIndex - step].intensity() > upperIBound)) {
      endIndex = endIndex - step;
    }
    step = step/2;
  }

  int min = D_MAX;
  int chosenIndex = 0;
  for (int i = startIndex; i <= endIndex; ++i) {
    const int thisD = ::ds(color_, colorList_[i].color());
    if (thisD < min) {
      min = thisD;
      chosenIndex = i;
    }
  }
  return colorList_[chosenIndex].color();
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

QVector<int> rgbToCode(const QVector<flossColor>& colors) {

  QVector<int> returnCodes;
  returnCodes.reserve(colors.size());
  const QVector<floss> dmcColors = ::initializeDMC();
  const QVector<floss> anchorColors = ::initializeAnchor();  
  for (int i = 0, size = colors.size(); i < size; ++i) {
    const flossColor thisColor = colors[i];
    switch (thisColor.type().value()) {
    case flossDMC: {
      const int code =
        dmcColors[dmcColors.indexOf(floss(thisColor.color()))].code();
      returnCodes.push_back(code);
      break;
    }
    case flossAnchor: {
      const int code =
        anchorColors[anchorColors.indexOf(floss(thisColor.color()))].code();
      returnCodes.push_back(code);
      break;
    }
    case flossVariable:
      returnCodes.push_back(-1);
      break;
    }
  }
  return returnCodes;
}

QVector<floss> initializeDMC() {

  QVector<floss> dmc;
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
    dmc.push_back(floss(000, "Blanc White", triC(255,255,255)));
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

  return dmc;
}

QVector<floss> initializeAnchor() {

  QVector<floss> anchor;
  anchor.reserve(ANCHOR_COUNT);

  anchor.push_back(floss(1, "", triC(255,255,255)));
  anchor.push_back(floss(2, "", triC(237,236,237)));
  anchor.push_back(floss(6, "", triC(235,177,167)));
  anchor.push_back(floss(8, "", triC(228,141,138)));
  anchor.push_back(floss(9, "", triC(219,116,117)));
  anchor.push_back(floss(10, "", triC(213,75,79)));
  anchor.push_back(floss(11, "", triC(212,72,61)));
  anchor.push_back(floss(13, "", triC(148,8,7)));
  anchor.push_back(floss(19, "", triC(132,3,26)));
  anchor.push_back(floss(20, "", triC(88,5,20)));
  anchor.push_back(floss(22, "", triC(60,5,16)));
  anchor.push_back(floss(23, "", triC(235,178,186)));
  anchor.push_back(floss(24, "", triC(225,140,154)));
  anchor.push_back(floss(25, "", triC(215,93,124)));
  anchor.push_back(floss(26, "", triC(218,75,113)));
  anchor.push_back(floss(27, "", triC(218,70,108)));
  anchor.push_back(floss(28, "", triC(213,49,85)));
  anchor.push_back(floss(29, "", triC(191,7,28)));
  anchor.push_back(floss(31, "", triC(221,89,120)));
  anchor.push_back(floss(33, "", triC(219,51,74)));
  anchor.push_back(floss(35, "", triC(216,35,51)));
  anchor.push_back(floss(36, "", triC(214,121,133)));
  anchor.push_back(floss(38, "", triC(215,45,76)));
  anchor.push_back(floss(39, "", triC(167,11,26)));
  anchor.push_back(floss(40, "", triC(215,46,89)));
  anchor.push_back(floss(41, "", triC(205,23,61)));
  anchor.push_back(floss(42, "", triC(162,7,29)));
  anchor.push_back(floss(43, "", triC(103,4,13)));
  anchor.push_back(floss(44, "", triC(71,3,7)));
  anchor.push_back(floss(45, "", triC(61,3,7)));
  anchor.push_back(floss(46, "", triC(172,3,22)));
  anchor.push_back(floss(47, "", triC(128,3,25)));
  anchor.push_back(floss(48, "", triC(225,181,196)));
  anchor.push_back(floss(49, "", triC(211,147,167)));
  anchor.push_back(floss(50, "", triC(220,103,146)));
  anchor.push_back(floss(52, "", triC(133,14,79)));
  anchor.push_back(floss(54, "", triC(139,16,82)));
  anchor.push_back(floss(55, "", triC(216,73,122)));
  anchor.push_back(floss(57, "", triC(196,23,73)));
  anchor.push_back(floss(59, "", triC(124,18,22)));
  anchor.push_back(floss(60, "", triC(208,106,148)));
  anchor.push_back(floss(62, "", triC(190,46,105)));
  anchor.push_back(floss(63, "", triC(172,14,62)));
  anchor.push_back(floss(65, "", triC(116,16,42)));
  anchor.push_back(floss(66, "", triC(212,96,145)));
  anchor.push_back(floss(68, "", triC(177,57,115)));
  anchor.push_back(floss(69, "", triC(111,7,43)));
  anchor.push_back(floss(70, "", triC(53,12,47)));
  anchor.push_back(floss(72, "", triC(47,9,34)));
  anchor.push_back(floss(73, "", triC(218,163,183)));
  anchor.push_back(floss(74, "", triC(208,123,151)));
  anchor.push_back(floss(75, "", triC(196,84,121)));
  anchor.push_back(floss(76, "", triC(176,51,86)));
  anchor.push_back(floss(77, "", triC(127,19,62)));
  anchor.push_back(floss(78, "", triC(111,13,48)));
  anchor.push_back(floss(85, "", triC(183,109,156)));
  anchor.push_back(floss(86, "", triC(167,68,128)));
  anchor.push_back(floss(87, "", triC(162,43,108)));
  anchor.push_back(floss(88, "", triC(134,15,80)));
  anchor.push_back(floss(89, "", triC(140,17,83)));
  anchor.push_back(floss(90, "", triC(155,88,139)));
  anchor.push_back(floss(92, "", triC(103,27,96)));
  anchor.push_back(floss(94, "", triC(80,12,67)));
  anchor.push_back(floss(95, "", triC(199,143,178)));
  anchor.push_back(floss(96, "", triC(177,84,139)));
  anchor.push_back(floss(97, "", triC(136,68,127)));
  anchor.push_back(floss(98, "", triC(116,53,118)));
  anchor.push_back(floss(99, "", triC(110,43,109)));
  anchor.push_back(floss(100, "", triC(87,27,93)));
  anchor.push_back(floss(101, "", triC(58,18,78)));
  anchor.push_back(floss(102, "", triC(35,14,64)));
  anchor.push_back(floss(103, "", triC(213,174,202)));
  anchor.push_back(floss(108, "", triC(162,139,183)));
  anchor.push_back(floss(109, "", triC(112,89,148)));
  anchor.push_back(floss(110, "", triC(82,57,131)));
  anchor.push_back(floss(111, "", triC(55,27,100)));
  anchor.push_back(floss(112, "", triC(39,20,88)));
  anchor.push_back(floss(117, "", triC(93,126,182)));
  anchor.push_back(floss(118, "", triC(68,76,146)));
  anchor.push_back(floss(119, "", triC(35,38,117)));
  anchor.push_back(floss(120, "", triC(148,177,211)));
  anchor.push_back(floss(121, "", triC(78,102,159)));
  anchor.push_back(floss(122, "", triC(33,56,112)));
  anchor.push_back(floss(123, "", triC(11,28,78)));
  anchor.push_back(floss(127, "", triC(9,13,32)));
  anchor.push_back(floss(128, "", triC(175,197,221)));
  anchor.push_back(floss(129, "", triC(99,140,188)));
  anchor.push_back(floss(130, "", triC(82,114,174)));
  anchor.push_back(floss(131, "", triC(43,63,133)));
  anchor.push_back(floss(132, "", triC(4,54,131)));
  anchor.push_back(floss(133, "", triC(2,45,118)));
  anchor.push_back(floss(134, "", triC(2,30,87)));
  anchor.push_back(floss(136, "", triC(53,96,156)));
  anchor.push_back(floss(137, "", triC(26,61,134)));
  anchor.push_back(floss(139, "", triC(1,42,108)));
  anchor.push_back(floss(140, "", triC(90,129,182)));
  anchor.push_back(floss(142, "", triC(25,74,149)));
  anchor.push_back(floss(143, "", triC(2,46,122)));
  anchor.push_back(floss(144, "", triC(131,169,207)));
  anchor.push_back(floss(145, "", triC(90,122,176)));
  anchor.push_back(floss(146, "", triC(37,91,163)));
  anchor.push_back(floss(147, "", triC(12,54,127)));
  anchor.push_back(floss(148, "", triC(11,31,81)));
  anchor.push_back(floss(149, "", triC(1,18,66)));
  anchor.push_back(floss(150, "", triC(2,18,56)));
  anchor.push_back(floss(152, "", triC(5,13,38)));
  anchor.push_back(floss(158, "", triC(161,182,174)));
  anchor.push_back(floss(159, "", triC(121,156,196)));
  anchor.push_back(floss(160, "", triC(118,163,205)));
  anchor.push_back(floss(161, "", triC(50,103,157)));
  anchor.push_back(floss(162, "", triC(1,60,116)));
  anchor.push_back(floss(164, "", triC(1,46,100)));
  anchor.push_back(floss(167, "", triC(97,159,167)));
  anchor.push_back(floss(168, "", triC(53,124,154)));
  anchor.push_back(floss(169, "", triC(1,86,131)));
  anchor.push_back(floss(170, "", triC(1,65,101)));
  anchor.push_back(floss(175, "", triC(119,150,197)));
  anchor.push_back(floss(176, "", triC(75,97,156)));
  anchor.push_back(floss(177, "", triC(41,53,120)));
  anchor.push_back(floss(178, "", triC(12,39,102)));
  anchor.push_back(floss(185, "", triC(130,191,185)));
  anchor.push_back(floss(186, "", triC(79,164,156)));
  anchor.push_back(floss(187, "", triC(34,140,122)));
  anchor.push_back(floss(188, "", triC(2,109,102)));
  anchor.push_back(floss(189, "", triC(2,104,90)));
  anchor.push_back(floss(203, "", triC(107,164,120)));
  anchor.push_back(floss(204, "", triC(91,155,113)));
  anchor.push_back(floss(205, "", triC(32,114,72)));
  anchor.push_back(floss(206, "", triC(124,176,141)));
  anchor.push_back(floss(208, "", triC(69,125,82)));
  anchor.push_back(floss(209, "", triC(56,125,80)));
  anchor.push_back(floss(210, "", triC(28,97,48)));
  anchor.push_back(floss(211, "", triC(14,86,47)));
  anchor.push_back(floss(212, "", triC(5,71,33)));
  anchor.push_back(floss(213, "", triC(172,197,171)));
  anchor.push_back(floss(214, "", triC(115,158,118)));
  anchor.push_back(floss(215, "", triC(74,124,87)));
  anchor.push_back(floss(216, "", triC(49,99,76)));
  anchor.push_back(floss(217, "", triC(35,83,57)));
  anchor.push_back(floss(218, "", triC(17,62,37)));
  anchor.push_back(floss(225, "", triC(111,159,92)));
  anchor.push_back(floss(226, "", triC(78,141,71)));
  anchor.push_back(floss(227, "", triC(7,99,42)));
  anchor.push_back(floss(228, "", triC(1,84,27)));
  anchor.push_back(floss(229, "", triC(1,85,27)));
  anchor.push_back(floss(230, "", triC(2,78,53)));
  anchor.push_back(floss(231, "", triC(183,166,162)));
  anchor.push_back(floss(232, "", triC(137,130,129)));
  anchor.push_back(floss(233, "", triC(107,93,98)));
  anchor.push_back(floss(234, "", triC(185,187,184)));
  anchor.push_back(floss(235, "", triC(88,94,109)));
  anchor.push_back(floss(236, "", triC(48,51,60)));
  anchor.push_back(floss(238, "", triC(89,142,49)));
  anchor.push_back(floss(239, "", triC(28,99,23)));
  anchor.push_back(floss(240, "", triC(198,223,184)));
  anchor.push_back(floss(241, "", triC(90,154,112)));
  anchor.push_back(floss(242, "", triC(102,155,93)));
  anchor.push_back(floss(243, "", triC(92,144,88)));
  anchor.push_back(floss(244, "", triC(52,123,58)));
  anchor.push_back(floss(245, "", triC(7,86,29)));
  anchor.push_back(floss(246, "", triC(1,61,18)));
  anchor.push_back(floss(253, "", triC(193,209,122)));
  anchor.push_back(floss(254, "", triC(167,185,83)));
  anchor.push_back(floss(255, "", triC(95,125,27)));
  anchor.push_back(floss(256, "", triC(66,111,25)));
  anchor.push_back(floss(257, "", triC(35,95,23)));
  anchor.push_back(floss(258, "", triC(36,95,23)));
  anchor.push_back(floss(259, "", triC(202,219,173)));
  anchor.push_back(floss(260, "", triC(194,197,168)));
  anchor.push_back(floss(261, "", triC(96,134,75)));
  anchor.push_back(floss(262, "", triC(56,78,37)));
  anchor.push_back(floss(263, "", triC(36,61,27)));
  anchor.push_back(floss(264, "", triC(154,175,114)));
  anchor.push_back(floss(265, "", triC(130,152,78)));
  anchor.push_back(floss(266, "", triC(89,118,41)));
  anchor.push_back(floss(267, "", triC(67,88,21)));
  anchor.push_back(floss(268, "", triC(37,83,26)));
  anchor.push_back(floss(269, "", triC(17,39,8)));
  anchor.push_back(floss(271, "", triC(236,214,217)));
  anchor.push_back(floss(273, "", triC(48,44,36)));
  anchor.push_back(floss(274, "", triC(150,168,164)));
  anchor.push_back(floss(275, "", triC(241,231,171)));
  anchor.push_back(floss(276, "", triC(214,196,169)));
  anchor.push_back(floss(277, "", triC(96,58,17)));
  anchor.push_back(floss(278, "", triC(208,203,85)));
  anchor.push_back(floss(279, "", triC(148,159,40)));
  anchor.push_back(floss(280, "", triC(122,114,28)));
  anchor.push_back(floss(281, "", triC(61,81,15)));
  anchor.push_back(floss(288, "", triC(237,213,82)));
  anchor.push_back(floss(289, "", triC(238,206,62)));
  anchor.push_back(floss(290, "", triC(245,195,4)));
  anchor.push_back(floss(291, "", triC(245,196,4)));
  anchor.push_back(floss(292, "", triC(232,219,133)));
  anchor.push_back(floss(293, "", triC(238,216,97)));
  anchor.push_back(floss(295, "", triC(235,200,67)));
  anchor.push_back(floss(297, "", triC(239,180,21)));
  anchor.push_back(floss(298, "", triC(236,164,10)));
  anchor.push_back(floss(300, "", triC(240,212,119)));
  anchor.push_back(floss(301, "", triC(236,192,62)));
  anchor.push_back(floss(302, "", triC(232,151,35)));
  anchor.push_back(floss(303, "", triC(225,118,7)));
  anchor.push_back(floss(304, "", triC(220,80,3)));
  anchor.push_back(floss(305, "", triC(231,179,50)));
  anchor.push_back(floss(306, "", triC(193,123,30)));
  anchor.push_back(floss(307, "", triC(167,90,17)));
  anchor.push_back(floss(308, "", triC(140,64,13)));
  anchor.push_back(floss(309, "", triC(104,38,7)));
  anchor.push_back(floss(310, "", triC(76,24,7)));
  anchor.push_back(floss(311, "", triC(224,171,99)));
  anchor.push_back(floss(313, "", triC(226,144,70)));
  anchor.push_back(floss(314, "", triC(223,96,25)));
  anchor.push_back(floss(316, "", triC(219,58,10)));
  anchor.push_back(floss(323, "", triC(222,88,53)));
  anchor.push_back(floss(324, "", triC(212,52,15)));
  anchor.push_back(floss(326, "", triC(149,24,8)));
  anchor.push_back(floss(328, "", triC(223,96,86)));
  anchor.push_back(floss(329, "", triC(221,71,45)));
  anchor.push_back(floss(330, "", triC(221,57,26)));
  anchor.push_back(floss(332, "", triC(221,42,12)));
  anchor.push_back(floss(333, "", triC(220,34,10)));
  anchor.push_back(floss(334, "", triC(184,3,23)));
  anchor.push_back(floss(335, "", triC(203,2,9)));
  anchor.push_back(floss(336, "", triC(207,130,103)));
  anchor.push_back(floss(337, "", triC(185,88,61)));
  anchor.push_back(floss(338, "", triC(168,57,31)));
  anchor.push_back(floss(339, "", triC(144,36,15)));
  anchor.push_back(floss(340, "", triC(109,24,9)));
  anchor.push_back(floss(341, "", triC(100,23,10)));
  anchor.push_back(floss(342, "", triC(187,166,201)));
  anchor.push_back(floss(343, "", triC(122,145,167)));
  anchor.push_back(floss(347, "", triC(186,120,76)));
  anchor.push_back(floss(349, "", triC(131,48,21)));
  anchor.push_back(floss(351, "", triC(93,20,7)));
  anchor.push_back(floss(352, "", triC(74,14,7)));
  anchor.push_back(floss(355, "", triC(98,33,13)));
  anchor.push_back(floss(357, "", triC(63,26,14)));
  anchor.push_back(floss(358, "", triC(72,38,19)));
  anchor.push_back(floss(359, "", triC(51,24,12)));
  anchor.push_back(floss(360, "", triC(53,24,12)));
  anchor.push_back(floss(361, "", triC(216,171,100)));
  anchor.push_back(floss(362, "", triC(188,123,52)));
  anchor.push_back(floss(363, "", triC(186,108,35)));
  anchor.push_back(floss(365, "", triC(123,51,9)));
  anchor.push_back(floss(366, "", triC(212,173,124)));
  anchor.push_back(floss(367, "", triC(204,158,102)));
  anchor.push_back(floss(368, "", triC(184,123,77)));
  anchor.push_back(floss(369, "", triC(141,75,35)));
  anchor.push_back(floss(370, "", triC(106,44,16)));
  anchor.push_back(floss(371, "", triC(94,35,14)));
  anchor.push_back(floss(372, "", triC(197,169,128)));
  anchor.push_back(floss(373, "", triC(177,137,82)));
  anchor.push_back(floss(374, "", triC(122,71,38)));
  anchor.push_back(floss(375, "", triC(99,65,36)));
  anchor.push_back(floss(376, "", triC(174,146,126)));
  anchor.push_back(floss(378, "", triC(127,87,67)));
  anchor.push_back(floss(379, "", triC(109,68,50)));
  anchor.push_back(floss(380, "", triC(46,21,12)));
  anchor.push_back(floss(381, "", triC(38,16,10)));
  anchor.push_back(floss(382, "", triC(20,7,5)));
  anchor.push_back(floss(386, "", triC(242,222,143)));
  anchor.push_back(floss(387, "", triC(212,191,164)));
  anchor.push_back(floss(388, "", triC(190,170,143)));
  anchor.push_back(floss(390, "", triC(200,183,158)));
  anchor.push_back(floss(391, "", triC(183,162,138)));
  anchor.push_back(floss(392, "", triC(142,124,101)));
  anchor.push_back(floss(393, "", triC(89,74,62)));
  anchor.push_back(floss(397, "", triC(204,202,190)));
  anchor.push_back(floss(398, "", triC(151,143,144)));
  anchor.push_back(floss(399, "", triC(134,129,133)));
  anchor.push_back(floss(400, "", triC(74,77,92)));
  anchor.push_back(floss(401, "", triC(36,35,38)));
  anchor.push_back(floss(403, "", triC(10,6,8)));
  anchor.push_back(floss(410, "", triC(1,95,174)));
  anchor.push_back(floss(433, "", triC(12,137,203)));
  anchor.push_back(floss(681, "", triC(51,69,29)));
  anchor.push_back(floss(683, "", triC(6,46,25)));
  anchor.push_back(floss(778, "", triC(212,178,154)));
  anchor.push_back(floss(779, "", triC(39,79,92)));
  anchor.push_back(floss(830, "", triC(178,170,154)));
  anchor.push_back(floss(831, "", triC(150,137,114)));
  anchor.push_back(floss(832, "", triC(117,100,67)));
  anchor.push_back(floss(842, "", triC(171,174,126)));
  anchor.push_back(floss(843, "", triC(119,124,58)));
  anchor.push_back(floss(844, "", triC(86,86,30)));
  anchor.push_back(floss(845, "", triC(66,71,28)));
  anchor.push_back(floss(846, "", triC(27,37,8)));
  anchor.push_back(floss(847, "", triC(172,187,179)));
  anchor.push_back(floss(848, "", triC(132,153,153)));
  anchor.push_back(floss(849, "", triC(133,154,154)));
  anchor.push_back(floss(850, "", triC(67,101,111)));
  anchor.push_back(floss(851, "", triC(66,99,109)));
  anchor.push_back(floss(852, "", triC(210,198,150)));
  anchor.push_back(floss(853, "", triC(148,144,102)));
  anchor.push_back(floss(854, "", triC(134,119,65)));
  anchor.push_back(floss(855, "", triC(136,120,66)));
  anchor.push_back(floss(856, "", triC(86,85,27)));
  anchor.push_back(floss(858, "", triC(125,136,105)));
  anchor.push_back(floss(859, "", triC(101,118,78)));
  anchor.push_back(floss(860, "", triC(78,97,59)));
  anchor.push_back(floss(861, "", triC(50,80,38)));
  anchor.push_back(floss(862, "", triC(33,58,31)));
  anchor.push_back(floss(868, "", triC(195,132,105)));
  anchor.push_back(floss(869, "", triC(159,143,156)));
  anchor.push_back(floss(870, "", triC(126,102,133)));
  anchor.push_back(floss(871, "", triC(65,40,66)));
  anchor.push_back(floss(872, "", triC(58,35,61)));
  anchor.push_back(floss(873, "", triC(39,17,33)));
  anchor.push_back(floss(874, "", triC(198,161,101)));
  anchor.push_back(floss(875, "", triC(114,156,127)));
  anchor.push_back(floss(876, "", triC(72,119,97)));
  anchor.push_back(floss(877, "", triC(53,103,85)));
  anchor.push_back(floss(878, "", triC(30,80,53)));
  anchor.push_back(floss(879, "", triC(6,67,45)));
  anchor.push_back(floss(880, "", triC(209,187,168)));
  anchor.push_back(floss(881, "", triC(208,181,157)));
  anchor.push_back(floss(882, "", triC(190,131,106)));
  anchor.push_back(floss(883, "", triC(143,72,48)));
  anchor.push_back(floss(884, "", triC(126,38,16)));
  anchor.push_back(floss(885, "", triC(219,206,166)));
  anchor.push_back(floss(886, "", triC(187,178,112)));
  anchor.push_back(floss(887, "", triC(163,151,83)));
  anchor.push_back(floss(888, "", triC(104,77,36)));
  anchor.push_back(floss(889, "", triC(72,39,15)));
  anchor.push_back(floss(890, "", triC(202,152,77)));
  anchor.push_back(floss(891, "", triC(201,152,77)));
  anchor.push_back(floss(892, "", triC(213,183,172)));
  anchor.push_back(floss(893, "", triC(205,151,148)));
  anchor.push_back(floss(894, "", triC(194,113,116)));
  anchor.push_back(floss(895, "", triC(189,85,96)));
  anchor.push_back(floss(896, "", triC(112,29,38)));
  anchor.push_back(floss(897, "", triC(77,8,17)));
  anchor.push_back(floss(898, "", triC(98,75,45)));
  anchor.push_back(floss(899, "", triC(171,149,130)));
  anchor.push_back(floss(900, "", triC(161,162,148)));
  anchor.push_back(floss(901, "", triC(132,64,12)));
  anchor.push_back(floss(903, "", triC(113,95,65)));
  anchor.push_back(floss(904, "", triC(84,64,46)));
  anchor.push_back(floss(905, "", triC(58,40,27)));
  anchor.push_back(floss(906, "", triC(70,47,24)));
  anchor.push_back(floss(907, "", triC(151,111,19)));
  anchor.push_back(floss(914, "", triC(142,89,72)));
  anchor.push_back(floss(920, "", triC(112,140,150)));
  anchor.push_back(floss(921, "", triC(63,96,110)));
  anchor.push_back(floss(922, "", triC(37,69,81)));
  anchor.push_back(floss(923, "", triC(0,54,14)));
  anchor.push_back(floss(924, "", triC(52,65,12)));
  anchor.push_back(floss(925, "", triC(218,44,9)));
  anchor.push_back(floss(926, "", triC(222,210,196)));
  anchor.push_back(floss(928, "", triC(140,183,186)));
  anchor.push_back(floss(933, "", triC(200,180,161)));
  anchor.push_back(floss(936, "", triC(64,25,15)));
  anchor.push_back(floss(939, "", triC(78,107,151)));
  anchor.push_back(floss(940, "", triC(36,67,135)));
  anchor.push_back(floss(941, "", triC(21,46,107)));
  anchor.push_back(floss(942, "", triC(219,186,144)));
  anchor.push_back(floss(943, "", triC(170,118,64)));
  anchor.push_back(floss(944, "", triC(88,43,16)));
  anchor.push_back(floss(945, "", triC(152,140,81)));
  anchor.push_back(floss(956, "", triC(186,179,143)));
  anchor.push_back(floss(968, "", triC(207,162,170)));
  anchor.push_back(floss(969, "", triC(191,133,155)));
  anchor.push_back(floss(970, "", triC(140,44,77)));
  anchor.push_back(floss(972, "", triC(104,7,49)));
  anchor.push_back(floss(975, "", triC(147,178,195)));
  anchor.push_back(floss(976, "", triC(124,154,176)));
  anchor.push_back(floss(977, "", triC(59,102,156)));
  anchor.push_back(floss(978, "", triC(58,96,144)));
  anchor.push_back(floss(979, "", triC(25,67,119)));
  anchor.push_back(floss(1001, "", triC(166,62,11)));
  anchor.push_back(floss(1002, "", triC(195,83,21)));
  anchor.push_back(floss(1003, "", triC(176,68,37)));
  anchor.push_back(floss(1004, "", triC(128,25,9)));
  anchor.push_back(floss(1005, "", triC(100,3,20)));
  anchor.push_back(floss(1006, "", triC(122,3,25)));
  anchor.push_back(floss(1007, "", triC(117,66,55)));
  anchor.push_back(floss(1008, "", triC(171,125,104)));
  anchor.push_back(floss(1009, "", triC(230,217,186)));
  anchor.push_back(floss(1010, "", triC(222,195,170)));
  anchor.push_back(floss(1011, "", triC(228,205,182)));
  anchor.push_back(floss(1012, "", triC(218,185,158)));
  anchor.push_back(floss(1013, "", triC(146,67,46)));
  anchor.push_back(floss(1014, "", triC(118,15,9)));
  anchor.push_back(floss(1015, "", triC(98,4,7)));
  anchor.push_back(floss(1016, "", triC(172,111,132)));
  anchor.push_back(floss(1017, "", triC(162,93,114)));
  anchor.push_back(floss(1018, "", triC(115,55,67)));
  anchor.push_back(floss(1019, "", triC(88,33,37)));
  anchor.push_back(floss(1020, "", triC(218,179,184)));
  anchor.push_back(floss(1021, "", triC(210,153,156)));
  anchor.push_back(floss(1022, "", triC(201,111,105)));
  anchor.push_back(floss(1023, "", triC(184,70,67)));
  anchor.push_back(floss(1024, "", triC(173,51,40)));
  anchor.push_back(floss(1025, "", triC(150,9,12)));
  anchor.push_back(floss(1026, "", triC(221,197,196)));
  anchor.push_back(floss(1027, "", triC(136,50,70)));
  anchor.push_back(floss(1028, "", triC(87,6,37)));
  anchor.push_back(floss(1029, "", triC(77,8,46)));
  anchor.push_back(floss(1030, "", triC(76,63,134)));
  anchor.push_back(floss(1031, "", triC(185,204,212)));
  anchor.push_back(floss(1032, "", triC(164,184,195)));
  anchor.push_back(floss(1033, "", triC(129,153,174)));
  anchor.push_back(floss(1034, "", triC(71,101,124)));
  anchor.push_back(floss(1035, "", triC(17,35,57)));
  anchor.push_back(floss(1036, "", triC(16,35,63)));
  anchor.push_back(floss(1037, "", triC(209,211,213)));
  anchor.push_back(floss(1038, "", triC(119,157,193)));
  anchor.push_back(floss(1039, "", triC(54,131,156)));
  anchor.push_back(floss(1040, "", triC(116,121,118)));
  anchor.push_back(floss(1041, "", triC(35,45,38)));
  anchor.push_back(floss(1042, "", triC(88,141,48)));
  anchor.push_back(floss(1043, "", triC(1,55,15)));
  anchor.push_back(floss(1044, "", triC(2,53,16)));
  anchor.push_back(floss(1045, "", triC(151,87,36)));
  anchor.push_back(floss(1046, "", triC(134,60,15)));
  anchor.push_back(floss(1047, "", triC(199,113,51)));
  anchor.push_back(floss(1048, "", triC(136,50,15)));
  anchor.push_back(floss(1049, "", triC(114,32,9)));
  anchor.push_back(floss(1050, "", triC(0,0,0)));
  anchor.push_back(floss(1060, "", triC(143,180,174)));
  anchor.push_back(floss(1062, "", triC(107,160,160)));
  anchor.push_back(floss(1064, "", triC(76,142,137)));
  anchor.push_back(floss(1066, "", triC(22,108,97)));
  anchor.push_back(floss(1068, "", triC(7,81,77)));
  anchor.push_back(floss(1070, "", triC(133,186,182)));
  anchor.push_back(floss(1072, "", triC(84,163,154)));
  anchor.push_back(floss(1074, "", triC(42,143,123)));
  anchor.push_back(floss(1076, "", triC(3,104,91)));
  anchor.push_back(floss(1080, "", triC(193,165,120)));
  anchor.push_back(floss(1082, "", triC(167,143,96)));
  anchor.push_back(floss(1084, "", triC(132,109,69)));
  anchor.push_back(floss(1086, "", triC(67,44,21)));
  anchor.push_back(floss(1088, "", triC(45,24,13)));
  anchor.push_back(floss(1089, "", triC(6,114,187)));
  anchor.push_back(floss(1090, "", triC(49,154,212)));
  anchor.push_back(floss(1092, "", triC(159,209,206)));
  anchor.push_back(floss(1094, "", triC(218,95,138)));
  anchor.push_back(floss(1096, "", triC(144,174,194)));
  anchor.push_back(floss(1098, "", triC(178,8,24)));
  anchor.push_back(floss(4146, "", triC(210,160,138)));
  anchor.push_back(floss(5975, "", triC(124,32,15)));
  anchor.push_back(floss(8581, "", triC(112,113,102)));
  anchor.push_back(floss(9046, "", triC(149,4,20)));
  anchor.push_back(floss(9159, "", triC(135,172,202)));
  anchor.push_back(floss(9575, "", triC(210,125,99)));

  return anchor;
}
