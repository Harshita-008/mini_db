#include "execution/projection.h"
#include "execution/filter.h"

namespace minidb {

ProjectionExecutor::ProjectionExecutor(std::unique_ptr<Executor> child,
                                       const std::vector<ExprPtr>& output_exprs,
                                       const Schema& input_schema)
    : child_(std::move(child)), output_exprs_(output_exprs), input_schema_(input_schema) {}

void ProjectionExecutor::Open() {
    child_->Open();
}

bool ProjectionExecutor::Next(Tuple& tuple, RID& rid) {
    Tuple input_tuple;
    if (!child_->Next(input_tuple, rid)) return false;

    // Project: evaluate each output expression
    tuple.clear();
    for (const auto& expr : output_exprs_) {
        Value val = FilterExecutor::EvalExpr(expr, input_tuple, input_schema_);
        tuple.push_back(val);
    }

    return true;
}

void ProjectionExecutor::Close() {
    child_->Close();
}

}  // namespace minidb
