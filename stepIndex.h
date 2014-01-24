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

#ifndef STEPINDEX_H
#define STEPINDEX_H

#include <QtCore/QMap>

//
// stepIndex is used to keep track of which of a set of indices are
// used/unused.  The indices go from <min> to <max-1> in steps of
// <step> (cf. the constructor).  next() returns the smallest
// available index, availableIndices() returns all available indices,
// reserve() marks an index as in use, and free() marks an index as
// available.
//
// Implementation note: We're using QMap since it orders its keys
//
class stepIndex {

 public:
  stepIndex() : minIndex_(0), maxIndex_(0), indexStep_(0) {}
  // max should be one past actual max; the set will hold indices starting
  // at <min> and going by <step> while < <max>
  stepIndex(int min, int max, int step);
  // return the next available index; return of max means none available
  int next();
  QList<int> availableIndices() const { return availableIndices_.keys(); }
  bool indexIsAvailable(int index) const {
    return availableIndices_.contains(index);
  }
  // mark <i> as in use (noop if <i> isn't in the set)
  // returns true if <i> was in the set
  bool reserve(int i);
  // mark <i> as available (noop if <i> isn't in the set)
  // return true if <i> is a valid member of the set
  bool free(int i);

 private:
  int minIndex_;
  int maxIndex_;
  int indexStep_;
  // keys are available indices, values are always 0
  QMap<int, int> availableIndices_;
};

#endif
