#ifndef KLEE_ARRAYDUMPER_H
#define KLEE_ARRAYDUMPER_H

#include <map>
#include <string>
#include <vector>

#include "klee/Expr/Constraints.h"
#include "klee/Solver/Solver.h"

namespace klee {
  class ArrayDumper {
    static std::map<std::string, std::vector<char>> constant_arrays;
    static std::map<std::string, unsigned int> array_lengths;

    static void updateSize(const klee::Array*);

  public:
    static void dumpArrays(const klee::Query&);
  };
}

#endif // KLEE_ARRAYDUMPER_H
