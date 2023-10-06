#include "ArrayConcretizationSolver.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/Expr/ArrayCache.h"
#include "klee/Config/config.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/ExprVisitor.h"
#include "klee/Support/OptionCategories.h"
#include "klee/Statistics/Statistics.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/FileHandling.h"
#include "klee/System/Time.h"

#include <set>
#include <vector>

namespace klee {

namespace  {

class IndexFinder : public ExprVisitor {
  protected:
    ExprVisitor::Action visitRead(const ReadExpr &re) {
      results.insert(re.index);
      for (const auto *un = re.updates.head.get(); un; un = un->next.get()) {
        results.insert(un->index);
      }

      return Action::doChildren();
    }

  public:
    std::set<ref<Expr>> results;
};

class SymbolicObjectFinder : public ExprVisitor {
protected:
  Action visitRead(const ReadExpr &re) {
    const UpdateList &ul = re.updates;

    for (const auto *un = ul.head.get(); un; un = un->next.get()) {
      visit(un->index);
      visit(un->value);
    }

    if (ul.root->isSymbolicArray())
      if (results.insert(ul.root).second)
        objects.push_back(ul.root);

    return Action::doChildren();
  }

public:
  std::set<const Array*> results;
  std::vector<const Array*> &objects;

  SymbolicObjectFinder(std::vector<const Array*> &_objects)
    : objects(_objects) {}
};


}


ArrayConcretizationSolver::ArrayConcretizationSolver(Solver *_solver) : solver(_solver) {
  assert(0 != solver);
}

ArrayConcretizationSolver::~ArrayConcretizationSolver() {
  delete solver;
}

bool ArrayConcretizationSolver::computeTruth(const Query &query, bool &isValid) {
  assert(false && "computeTruth unimplemented");
  bool success = solver->impl->computeTruth(query, isValid);
  return success;
}

bool ArrayConcretizationSolver::computeValidity(const Query &query,
                                         Solver::Validity &result) {
  assert(false && "computeValidity unimplemented");
  bool success = solver->impl->computeValidity(query, result);
  return success;
}

bool ArrayConcretizationSolver::computeValue(const Query &query, ref<Expr> &result) {
  assert(false && "computeValue unimplemented");
  bool success = solver->impl->computeValue(query, result);
  return success;
}

bool ArrayConcretizationSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char> > &values, bool &hasSolution) {

  IndexFinder indexFinder;
  indexFinder.visit(query.expr);
  for(const auto &e : query.constraints)
    indexFinder.visit(e);
  std::vector indices(indexFinder.results.begin(), indexFinder.results.end());

  std::vector<const Array *> usedObjects;
  SymbolicObjectFinder objectFinder(usedObjects);
  objectFinder.visit(query.expr);
  for(const auto &e : query.constraints)
    objectFinder.visit(e);

  // klee_message("Concretizing arrays. %ld indices, %ld objects", indices.size(), usedObjects.size());
  for(auto& idx : indices) {
    assert(idx.get()->getWidth() == 32);
  }

  // computeInitialValues only handles 8 bit ranges, so we need to split the bytes of each index
  auto index_eval = arrayCache.CreateArray("index_evaluation_" + std::to_string(indices.size()), indices.size() * 4);

  ConstraintSet cs;
  for(size_t i = 0; i < indices.size(); i++) {
    for(size_t j = 0; j < 4; j++) {
      auto offset = 8*j;
      auto b = ExtractExpr::create(indices[i], offset, Expr::Int8);
      cs.push_back(EqExpr::create(b, ReadExpr::create(UpdateList(index_eval, 0), ConstantExpr::create(4*i+j, 32))));
    }
  }

  // Constrain the input
  for(auto& obj : objects) {
    if(obj->isSymbolicArray()) {
      for(size_t i = 0; i < obj->getSize(); i++) {
        cs.push_back(EqExpr::create(
          ReadExpr::create(UpdateList(obj, 0), ConstantExpr::create(i, obj->getDomain())),
          ConstantExpr::create(0, obj->getRange())
        ));
      }
    }
  }

  std::vector<std::vector<unsigned char>> output_buf;
  Query tempQuery(cs, ConstantExpr::create(0, Expr::Bool));
  // ExprPPrinter::printQuery(llvm::errs(), cs, tempQuery.expr);

  bool success = solver->impl->computeInitialValues(tempQuery, std::vector{index_eval}, output_buf, hasSolution);
  // klee_message("success %d, hasSolution %d, output_buf.size() %d", success, hasSolution, output_buf.size());
  if(!hasSolution) {
    return success;
  }

  // Rebuild the indices
  assert(output_buf.size() == 1);
  assert(output_buf[0].size() == indices.size() * 4);

  ConstraintSet newCs;
  for(auto& c : query.constraints) {
    newCs.push_back(c);
  }
  for(size_t i = 0; i < indices.size(); i++) {
    uint32_t value = 0;
    for(size_t j = 0; j < 4; j++) {
      uint32_t b = output_buf[0][4*i+j];
      auto offset = 8*j;
      value |= b << offset;
    }
    // klee_message("Index %d -> %d", i, value);
    newCs.push_back(EqExpr::create(indices[i], ConstantExpr::create(value, Expr::Int32)));
  }

  Query finalQuery(newCs, query.expr);

  success =
      solver->impl->computeInitialValues(finalQuery, objects, values, hasSolution);
  return success;
}

SolverImpl::SolverRunStatus ArrayConcretizationSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}

char *ArrayConcretizationSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

void ArrayConcretizationSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

Solver *createArrayConcretizationSolver(Solver *_solver) {
  return new Solver(new ArrayConcretizationSolver(_solver));
}

}
