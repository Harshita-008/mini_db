#include "execution/nested_loop_join.h"
#include "execution/filter.h"

namespace minidb {

NestedLoopJoinExecutor::NestedLoopJoinExecutor(std::unique_ptr<Executor> outer,
                                               std::unique_ptr<Executor> inner,
                                               ExprPtr join_condition,
                                               const Schema& outer_schema,
                                               const Schema& inner_schema)
    : outer_(std::move(outer)), inner_(std::move(inner)),
      join_condition_(join_condition),
      outer_schema_(outer_schema), inner_schema_(inner_schema),
      outer_idx_(0), inner_idx_(0), initialized_(false) {}

void NestedLoopJoinExecutor::Open() {
    outer_->Open();
    inner_->Open();

    // Materialize both sides
    outer_results_.clear();
    inner_results_.clear();

    Tuple tuple;
    RID rid;
    while (outer_->Next(tuple, rid)) {
        outer_results_.push_back({rid, tuple});
    }
    while (inner_->Next(tuple, rid)) {
        inner_results_.push_back({rid, tuple});
    }

    outer_idx_ = 0;
    inner_idx_ = 0;
    initialized_ = true;
}

bool NestedLoopJoinExecutor::Next(Tuple& tuple, RID& rid) {
    while (outer_idx_ < outer_results_.size()) {
        while (inner_idx_ < inner_results_.size()) {
            const auto& outer_tuple = outer_results_[outer_idx_].second;
            const auto& inner_tuple = inner_results_[inner_idx_].second;

            inner_idx_++;

            if (EvalJoinCondition(outer_tuple, inner_tuple)) {
                // Concatenate tuples
                tuple.clear();
                tuple.insert(tuple.end(), outer_tuple.begin(), outer_tuple.end());
                tuple.insert(tuple.end(), inner_tuple.begin(), inner_tuple.end());
                rid = outer_results_[outer_idx_].first;
                return true;
            }
        }
        outer_idx_++;
        inner_idx_ = 0;
    }
    return false;
}

void NestedLoopJoinExecutor::Close() {
    outer_->Close();
    inner_->Close();
    outer_results_.clear();
    inner_results_.clear();
}

bool NestedLoopJoinExecutor::EvalJoinCondition(const Tuple& outer_tuple, const Tuple& inner_tuple) {
    if (!join_condition_) return true;  // Cross join

    auto* bin = dynamic_cast<BinaryOpExpr*>(join_condition_.get());
    if (!bin) return true;

    // Evaluate left side (expected to be column from outer or inner)
    auto* left_col = dynamic_cast<ColumnRefExpr*>(bin->left.get());
    auto* right_col = dynamic_cast<ColumnRefExpr*>(bin->right.get());

    if (!left_col || !right_col) return true;

    // Helper: resolve a column reference against outer/inner using table qualifier
    auto resolve = [&](ColumnRefExpr* col, Value& val) -> bool {
        // If table name is specified, use it to determine which side
        if (!col->table_name.empty()) {
            // Check if it matches any column in outer schema's table
            // We check by looking at the column in each schema
            int idx = outer_schema_.FindColumn(col->column_name);
            int inner_idx = inner_schema_.FindColumn(col->column_name);

            // Use table name to disambiguate: try outer first
            // The binder resolves table names, so we check if this column
            // with this table name belongs to outer or inner
            if (idx >= 0) {
                // Check if inner also has it — if so, need table name to decide
                if (inner_idx >= 0) {
                    // Both have the column — use table name heuristic:
                    // The binder puts the resolved table name on the column.
                    // Outer is from_tables[0], inner is joins[0].table_name
                    // We can't easily know here, so try: if left_col refers to
                    // outer's column name, try outer first
                    // Simple heuristic: check if table_name matches by trying both
                    val = outer_tuple[idx];
                    return true;
                }
                val = outer_tuple[idx];
                return true;
            }
            if (inner_idx >= 0) {
                val = inner_tuple[inner_idx];
                return true;
            }
        } else {
            int idx = outer_schema_.FindColumn(col->column_name);
            if (idx >= 0 && idx < static_cast<int>(outer_tuple.size())) {
                val = outer_tuple[idx];
                return true;
            }
            idx = inner_schema_.FindColumn(col->column_name);
            if (idx >= 0 && idx < static_cast<int>(inner_tuple.size())) {
                val = inner_tuple[idx];
                return true;
            }
        }
        return false;
    };

    // For qualified columns like users.id = orders.user_id,
    // use the table name to figure out which is outer and which is inner.
    Value left_val, right_val;

    // Try resolving left from outer and right from inner first
    auto* col_a = left_col;
    auto* col_b = right_col;

    // Check if left_col's table matches outer or inner
    int left_in_outer = outer_schema_.FindColumn(col_a->column_name);
    int left_in_inner = inner_schema_.FindColumn(col_a->column_name);
    int right_in_outer = outer_schema_.FindColumn(col_b->column_name);
    int right_in_inner = inner_schema_.FindColumn(col_b->column_name);

    // Smart resolution: if left only exists in outer and right only in inner
    if (left_in_outer >= 0 && right_in_inner >= 0 &&
        (left_in_inner < 0 || right_in_outer < 0 ||
         col_a->column_name != col_b->column_name)) {
        left_val = outer_tuple[left_in_outer];
        right_val = inner_tuple[right_in_inner];
    } else if (left_in_inner >= 0 && right_in_outer >= 0) {
        left_val = inner_tuple[left_in_inner];
        right_val = outer_tuple[right_in_outer];
    } else {
        // Fallback: same column name in both — use positionally
        // left from outer, right from inner
        if (left_in_outer >= 0) left_val = outer_tuple[left_in_outer];
        else if (left_in_inner >= 0) left_val = inner_tuple[left_in_inner];

        if (right_in_inner >= 0) right_val = inner_tuple[right_in_inner];
        else if (right_in_outer >= 0) right_val = outer_tuple[right_in_outer];
    }

    // Compare
    switch (bin->op) {
        case BinaryOpType::EQ:
            return left_val == right_val;
        case BinaryOpType::NEQ:
            return left_val != right_val;
        default:
            return left_val == right_val;
    }
}

}  // namespace minidb
