#pragma once

#include "sql/ast.h"
#include "catalog/catalog.h"
#include "common/status.h"
#include <memory>

namespace minidb {

// Semantic analyzer / binder.
// Resolves table and column references against the catalog.
class Binder {
public:
    explicit Binder(Catalog* catalog);

    // Bind an AST node — resolves names, checks types
    Status Bind(std::shared_ptr<ASTNode> node);

private:
    Status BindCreateTable(CreateTableStmt& stmt);
    Status BindInsert(InsertStmt& stmt);
    Status BindSelect(SelectStmt& stmt);
    Status BindDelete(DeleteStmt& stmt);

    // Verify that column references in an expression are valid
    Status BindExpression(ExprPtr& expr, const std::vector<std::string>& table_names);

    Catalog* catalog_;
};

}  // namespace minidb
