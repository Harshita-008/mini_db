#pragma once

#include "sql/ast.h"
#include "catalog/catalog.h"
#include "index/index_manager.h"
#include "optimizer/stats.h"
#include "optimizer/cost_model.h"
#include <string>
#include <vector>
#include <memory>

namespace minidb {

// Physical plan node types
enum class PlanNodeType {
    SEQ_SCAN,
    INDEX_SCAN,
    FILTER,
    PROJECTION,
    NESTED_LOOP_JOIN,
    INSERT,
    DELETE_PLAN,
};

// Physical plan node
struct PlanNode {
    PlanNodeType type;
    std::string table_name;
    std::vector<ExprPtr> output_exprs;   // For projection
    ExprPtr predicate;                    // For filter / join condition
    std::vector<std::shared_ptr<PlanNode>> children;

    // For insert
    std::vector<Tuple> tuples_to_insert;

    // For index scan
    int32_t index_key = 0;
    bool use_index = false;

    PlanNode(PlanNodeType t) : type(t) {}
};

using PlanNodePtr = std::shared_ptr<PlanNode>;

// Query optimizer — produces physical plans from parsed AST.
class Optimizer {
public:
    Optimizer(Catalog* catalog, IndexManager* index_mgr, StatsManager* stats_mgr);

    // Optimize a parsed & bound AST node into a physical plan
    PlanNodePtr Optimize(std::shared_ptr<ASTNode> ast);

private:
    PlanNodePtr OptimizeSelect(SelectStmt& stmt);
    PlanNodePtr OptimizeInsert(InsertStmt& stmt);
    PlanNodePtr OptimizeDelete(DeleteStmt& stmt);

    // Choose access path for a single table
    PlanNodePtr MakeTableScan(const std::string& table_name, ExprPtr predicate);

    // Build join plan for multiple tables
    PlanNodePtr BuildJoinPlan(const std::vector<std::string>& tables,
                              const std::vector<ExprPtr>& join_conditions,
                              ExprPtr where_clause);

    // Check if a predicate is an equality on the primary key
    bool IsPrimaryKeyEq(const std::string& table_name, const ExprPtr& pred, int32_t& key_value);

    Catalog* catalog_;
    IndexManager* index_mgr_;
    StatsManager* stats_mgr_;
};

}  // namespace minidb
