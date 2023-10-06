#ifndef KLEE_ARRAYCONCRETIZATIONSOLVER_H
#define KLEE_ARRAYCONCRETIZATIONSOLVER_H

#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/System/Time.h"
#include "klee/Expr/ArrayCache.h"

#include "llvm/Support/raw_ostream.h"

namespace klee {

class ArrayConcretizationSolver : public SolverImpl {

protected:
  Solver *solver;

public:
  ArrayConcretizationSolver(Solver *_solver);

  virtual ~ArrayConcretizationSolver();

  /// implementation of the SolverImpl interface
  bool computeTruth(const Query &query, bool &isValid);
  bool computeValidity(const Query &query, Solver::Validity &result);
  bool computeValue(const Query &query, ref<Expr> &result);
  bool computeInitialValues(const Query &query,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(time::Span timeout);

private:
  ArrayCache arrayCache;
};

}

#endif /* KLEE_ARRAYCONCRETIZATIONSOLVER_H */
