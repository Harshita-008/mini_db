#pragma once

#include "optimizer/stats.h"
#include "sql/ast.h"
#include <cmath>

namespace minidb {

// Cost model for query optimization.
// Estimates the cost of different access paths and join orders.
class CostModel {
public:
    // Estimate selectivity of a predicate
    static double EstimateSelectivity(const ExprPtr& expr, const TableStats& stats) {
        auto* bin = dynamic_cast<BinaryOpExpr*>(expr.get());
        if (!bin) return 0.5;  // Default

        if (bin->op == BinaryOpType::AND) {
            return EstimateSelectivity(bin->left, stats) *
                   EstimateSelectivity(bin->right, stats);
        }
        if (bin->op == BinaryOpType::OR) {
            double s1 = EstimateSelectivity(bin->left, stats);
            double s2 = EstimateSelectivity(bin->right, stats);
            return s1 + s2 - s1 * s2;
        }

        // For comparison operators, check if one side is a column
        auto* col = dynamic_cast<ColumnRefExpr*>(bin->left.get());
        if (!col) col = dynamic_cast<ColumnRefExpr*>(bin->right.get());

        if (col) {
            auto it = stats.column_stats.find(col->column_name);
            if (it == stats.column_stats.end()) {
                it = stats.column_stats.find("_pk");
            }

            if (it != stats.column_stats.end()) {
                const auto& cs = it->second;
                switch (bin->op) {
                    case BinaryOpType::EQ:
                        return cs.distinct_values > 0 ? 1.0 / cs.distinct_values : 0.1;
                    case BinaryOpType::NEQ:
                        return cs.distinct_values > 0 ? 1.0 - 1.0 / cs.distinct_values : 0.9;
                    case BinaryOpType::LT:
                    case BinaryOpType::GT:
                    case BinaryOpType::LTE:
                    case BinaryOpType::GTE: {
                        // Uniform distribution assumption
                        if (cs.max_value > cs.min_value) {
                            return 1.0 / 3.0;  // Default range selectivity
                        }
                        return 0.5;
                    }
                    default:
                        return 0.5;
                }
            }
        }

        return 0.1;  // Default selectivity
    }

    // Cost of sequential scan
    static double SeqScanCost(const TableStats& stats) {
        return static_cast<double>(stats.page_count > 0 ? stats.page_count : stats.row_count);
    }

    // Cost of index scan
    static double IndexScanCost(const TableStats& stats, double selectivity) {
        // Tree traversal (log) + fraction of pages to read
        double height = stats.row_count > 0 ? std::log2(stats.row_count) : 1.0;
        double pages_to_read = selectivity * stats.page_count;
        return height + pages_to_read;
    }

    // Cost of nested loop join
    static double NLJoinCost(const TableStats& outer, const TableStats& inner) {
        return SeqScanCost(outer) + outer.row_count * SeqScanCost(inner);
    }

    // Should we use index scan vs sequential scan?
    static bool ShouldUseIndex(const TableStats& stats, double selectivity) {
        if (stats.row_count < 100) return false;  // Too small, seq scan is fine
        return IndexScanCost(stats, selectivity) < SeqScanCost(stats);
    }
};

}  // namespace minidb
