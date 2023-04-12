#include "ArrayDumper.h"

#include "klee/Expr/ExprUtil.h"
#include "klee/Support/ErrorHandling.h"

#include <fstream>

using namespace klee;

std::map<std::string, std::vector<char>> ArrayDumper::constant_arrays;
std::map<std::string, unsigned int> ArrayDumper::array_lengths;

void ArrayDumper::dumpArrays(const klee::ConstraintSet& constraints) {
  klee::ArrayFinder finder;
  for(auto& e : constraints) {
    finder.visit(e);
  }
  for(auto& a : finder.results) {
    // Not a new array, do some sanity checks
    if(array_lengths.find(a->name) != array_lengths.end()) {
      if(a->isConstantArray()) {
        assert(array_lengths[a->name] == a->size);
        assert(constant_arrays.find(a->name) != constant_arrays.end());

        auto& data = constant_arrays[a->name];
        for(size_t i = 0; i < a->size; i++) {
          char ch = a->constantValues.at(i)->getZExtValue(8);
          assert(ch == data[i]);
        }
      }

      updateSize(a);
      continue;
    }

    updateSize(a);

    if(a->isConstantArray()) {
      klee_warning("Found constant array %s (%d bytes)", a->name.c_str(), a->size);

      auto& data = constant_arrays[a->name];
      data.resize(a->size);
      for(size_t i = 0; i < a->size; i++) {
        data[i] = a->constantValues.at(i)->getZExtValue(8);
      }

      std::ofstream out("klee-last/arrays/" + a->name, std::ios::binary);
      assert(out.good());
      out.write(data.data(), data.size());
    } else {
      klee_warning("Found symbolic array %s (%d bytes)", a->name.c_str(), a->size);
    }
  }
}

void ArrayDumper::updateSize(const klee::Array* a) {
  if(array_lengths.find(a->name) == array_lengths.end() ||
      a->size > array_lengths[a->name]) {

    array_lengths[a->name] = a->size;
    std::ofstream out("klee-last/array-sizes/" + a->name);
    assert(out.good());
    out << a->size << std::endl;
  }
}
