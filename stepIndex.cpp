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

#include "stepIndex.h"

stepIndex::stepIndex(int min, int max, int step)
  : minIndex_(min), maxIndex_(max), indexStep_(step) {

  for (int i = minIndex_; i < maxIndex_; i += indexStep_) {
    availableIndices_.insert(i, 0);
  }
}

int stepIndex::next() {

  if (!availableIndices_.empty()) {
    const int returnIndex = availableIndices_.firstKey();
    availableIndices_.remove(returnIndex);
    return returnIndex;
  }
  else {
    return maxIndex_;
  }
}

bool stepIndex::reserve(int i) {

  if (availableIndices_.remove(i)) {
    return true;
  }
  else {
    return false;
  }
}

bool stepIndex::free(int i) {

  if (i >= minIndex_ && i < maxIndex_ && (i - minIndex_) % indexStep_ == 0) {
    availableIndices_.insert(i, 0);
    return true;
  }
  else {
    return false;
  }
}


