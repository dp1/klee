#ifndef KLEE_ARRAYDUMPER_H
#define KLEE_ARRAYDUMPER_H

#include <map>
#include <string>
#include <vector>

#include "klee/Expr/Constraints.h"

namespace klee {
  class ArrayDumper {
    static std::map<std::string, std::vector<char>> constant_arrays;
    static std::map<std::string, unsigned int> array_lengths;

  public:
    static void dumpArrays(const klee::ConstraintSet&);
  };
}

#endif // KLEE_ARRAYDUMPER_H
