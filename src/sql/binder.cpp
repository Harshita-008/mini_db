#include "sql/binder.h"

namespace minidb {

Binder::Binder(Catalog* catalog) : catalog_(catalog) {}

Status Binder::Bind(std::shared_ptr<ASTNode> node) {
    switch (node->type) {
        case ASTNodeType::CREATE_TABLE:
            return BindCreateTable(std::get<CreateTableStmt>(node->stmt));
        case ASTNodeType::INSERT:
            return BindInsert(std::get<InsertStmt>(node->stmt));
        case ASTNodeType::SELECT:
            return BindSelect(std::get<SelectStmt>(node->stmt));
        case ASTNodeType::DELETE_STMT:
            return BindDelete(std::get<DeleteStmt>(node->stmt));
        default:
            return Status::BindError("Unknown statement type");
    }
}

Status Binder::BindCreateTable(CreateTableStmt& stmt) {
    // Check that table doesn't already exist
    if (catalog_->TableExists(stmt.table_name)) {
        return Status::BindError("Table '" + stmt.table_name + "' already exists");
    }

    // Validate columns
    if (stmt.columns.empty()) {
        return Status::BindError("Table must have at least one column");
    }

    // Check for duplicate column names
    for (size_t i = 0; i < stmt.columns.size(); i++) {
        for (size_t j = i + 1; j < stmt.columns.size(); j++) {
            if (stmt.columns[i].name == stmt.columns[j].name) {
                return Status::BindError("Duplicate column name: " + stmt.columns[i].name);
            }
        }
    }

    return Status::OK();
}

Status Binder::BindInsert(InsertStmt& stmt) {
    // Check table exists
    if (!catalog_->TableExists(stmt.table_name)) {
        return Status::BindError("Table '" + stmt.table_name + "' does not exist");
    }

    TableInfo* info = catalog_->GetTable(stmt.table_name);
    if (!info) return Status::BindError("Cannot get table info");

    // Check value count matches column count
    for (const auto& values : stmt.values_list) {
        if (values.size() != info->schema.GetColumnCount()) {
            return Status::BindError("Value count (" + std::to_string(values.size()) +
                                     ") doesn't match column count (" +
                                     std::to_string(info->schema.GetColumnCount()) + ")");
        }
    }

    return Status::OK();
}

Status Binder::BindSelect(SelectStmt& stmt) {
    // Check all FROM tables exist
    for (const auto& table : stmt.from_tables) {
        if (!catalog_->TableExists(table)) {
            return Status::BindError("Table '" + table + "' does not exist");
        }
    }

    // Check JOIN tables exist
    for (const auto& join : stmt.joins) {
        if (!catalog_->TableExists(join.table_name)) {
            return Status::BindError("Table '" + join.table_name + "' does not exist");
        }
    }

    // Collect all table names for column resolution
    std::vector<std::string> all_tables = stmt.from_tables;
    for (const auto& join : stmt.joins) {
        all_tables.push_back(join.table_name);
    }

    // Bind select list
    for (auto& expr : stmt.select_list) {
        if (auto* star = dynamic_cast<StarExpr*>(expr.get())) {
            continue;  // Star is always valid
        }
        Status s = BindExpression(expr, all_tables);
        if (!s.ok()) return s;
    }

    // Bind WHERE clause
    if (stmt.where_clause) {
        Status s = BindExpression(stmt.where_clause, all_tables);
        if (!s.ok()) return s;
    }

    // Bind JOIN conditions
    for (auto& join : stmt.joins) {
        if (join.on_condition) {
            Status s = BindExpression(join.on_condition, all_tables);
            if (!s.ok()) return s;
        }
    }

    return Status::OK();
}

Status Binder::BindDelete(DeleteStmt& stmt) {
    if (!catalog_->TableExists(stmt.table_name)) {
        return Status::BindError("Table '" + stmt.table_name + "' does not exist");
    }

    if (stmt.where_clause) {
        std::vector<std::string> tables = {stmt.table_name};
        return BindExpression(stmt.where_clause, tables);
    }

    return Status::OK();
}

Status Binder::BindExpression(ExprPtr& expr, const std::vector<std::string>& table_names) {
    if (auto* col_ref = dynamic_cast<ColumnRefExpr*>(expr.get())) {
        // If table is specified, check it exists in our scope
        if (!col_ref->table_name.empty()) {
            bool found = false;
            for (const auto& t : table_names) {
                if (t == col_ref->table_name) { found = true; break; }
            }
            if (!found) {
                return Status::BindError("Unknown table: " + col_ref->table_name);
            }
            // Check column exists
            TableInfo* info = catalog_->GetTable(col_ref->table_name);
            if (info && info->schema.FindColumn(col_ref->column_name) < 0) {
                return Status::BindError("Column '" + col_ref->column_name +
                                         "' not found in table '" + col_ref->table_name + "'");
            }
        } else {
            // Unqualified column — search all tables
            bool found = false;
            for (const auto& t : table_names) {
                TableInfo* info = catalog_->GetTable(t);
                if (info && info->schema.FindColumn(col_ref->column_name) >= 0) {
                    col_ref->table_name = t;  // Resolve the table
                    found = true;
                    break;
                }
            }
            if (!found) {
                return Status::BindError("Column '" + col_ref->column_name + "' not found");
            }
        }
    } else if (auto* bin = dynamic_cast<BinaryOpExpr*>(expr.get())) {
        Status s = BindExpression(bin->left, table_names);
        if (!s.ok()) return s;
        s = BindExpression(bin->right, table_names);
        if (!s.ok()) return s;
    }
    // Literals need no binding

    return Status::OK();
}

}  // namespace minidb
