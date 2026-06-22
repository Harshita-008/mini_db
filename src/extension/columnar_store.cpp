#include "extension/columnar_store.h"

namespace minidb {

ColumnarStore::ColumnarStore(const std::string& table_name, const Schema& schema)
    : table_name_(table_name), schema_(schema) {
    columns_.resize(schema.GetColumnCount());
    for (size_t i = 0; i < schema.GetColumnCount(); i++) {
        columns_[i].type = schema.GetColumn(i).type;
    }
}

void ColumnarStore::LoadFromTuples(const std::vector<Tuple>& tuples) {
    row_count_ = tuples.size();
    for (size_t i = 0; i < columns_.size(); i++) {
        columns_[i].size = row_count_;
        switch (columns_[i].type) {
            case ColumnType::INT:
                columns_[i].int_data.reserve(row_count_);
                for (const auto& tuple : tuples) {
                    columns_[i].int_data.push_back(std::get<int32_t>(tuple[i]));
                }
                break;
            case ColumnType::FLOAT:
                columns_[i].float_data.reserve(row_count_);
                for (const auto& tuple : tuples) {
                    columns_[i].float_data.push_back(std::get<double>(tuple[i]));
                }
                break;
            case ColumnType::VARCHAR:
                columns_[i].string_data.reserve(row_count_);
                for (const auto& tuple : tuples) {
                    columns_[i].string_data.push_back(std::get<std::string>(tuple[i]));
                }
                break;
            case ColumnType::BOOL:
                columns_[i].bool_data.reserve(row_count_);
                for (const auto& tuple : tuples) {
                    columns_[i].bool_data.push_back(std::get<bool>(tuple[i]));
                }
                break;
        }
    }
}

const ColumnData& ColumnarStore::ScanColumn(size_t col_idx) const {
    return columns_[col_idx];
}

std::vector<const ColumnData*> ColumnarStore::ScanColumns(const std::vector<size_t>& col_indices) const {
    std::vector<const ColumnData*> result;
    for (size_t idx : col_indices) {
        result.push_back(&columns_[idx]);
    }
    return result;
}

std::vector<size_t> ColumnarStore::FilterColumn(size_t col_idx, const std::string& op, const Value& value) const {
    std::vector<size_t> result;
    const auto& col = columns_[col_idx];

    if (col.type == ColumnType::INT && std::holds_alternative<int32_t>(value)) {
        int32_t val = std::get<int32_t>(value);
        for (size_t i = 0; i < col.int_data.size(); i++) {
            bool match = false;
            if (op == "=")       match = (col.int_data[i] == val);
            else if (op == "!=") match = (col.int_data[i] != val);
            else if (op == "<")  match = (col.int_data[i] < val);
            else if (op == ">")  match = (col.int_data[i] > val);
            else if (op == "<=") match = (col.int_data[i] <= val);
            else if (op == ">=") match = (col.int_data[i] >= val);
            if (match) result.push_back(i);
        }
    } else if (col.type == ColumnType::FLOAT && std::holds_alternative<double>(value)) {
        double val = std::get<double>(value);
        for (size_t i = 0; i < col.float_data.size(); i++) {
            bool match = false;
            if (op == "=")       match = (col.float_data[i] == val);
            else if (op == "<")  match = (col.float_data[i] < val);
            else if (op == ">")  match = (col.float_data[i] > val);
            else if (op == "<=") match = (col.float_data[i] <= val);
            else if (op == ">=") match = (col.float_data[i] >= val);
            if (match) result.push_back(i);
        }
    } else if (col.type == ColumnType::VARCHAR && std::holds_alternative<std::string>(value)) {
        const std::string& val = std::get<std::string>(value);
        for (size_t i = 0; i < col.string_data.size(); i++) {
            bool match = false;
            if (op == "=")       match = (col.string_data[i] == val);
            else if (op == "!=") match = (col.string_data[i] != val);
            if (match) result.push_back(i);
        }
    }

    return result;
}

int64_t ColumnarStore::SumInt(size_t col_idx) const {
    int64_t sum = 0;
    const auto& col = columns_[col_idx];
    for (int32_t v : col.int_data) sum += v;
    return sum;
}

int64_t ColumnarStore::SumIntFiltered(size_t col_idx, const std::vector<size_t>& row_indices) const {
    int64_t sum = 0;
    const auto& col = columns_[col_idx];
    for (size_t idx : row_indices) {
        if (idx < col.int_data.size()) sum += col.int_data[idx];
    }
    return sum;
}

Tuple ColumnarStore::MaterializeRow(size_t row_idx) const {
    Tuple tuple;
    for (size_t i = 0; i < columns_.size(); i++) {
        switch (columns_[i].type) {
            case ColumnType::INT:    tuple.push_back(columns_[i].int_data[row_idx]); break;
            case ColumnType::FLOAT:  tuple.push_back(columns_[i].float_data[row_idx]); break;
            case ColumnType::VARCHAR:tuple.push_back(columns_[i].string_data[row_idx]); break;
            case ColumnType::BOOL:   tuple.push_back(columns_[i].bool_data[row_idx]); break;
        }
    }
    return tuple;
}

Tuple ColumnarStore::MaterializeRow(size_t row_idx, const std::vector<size_t>& col_indices) const {
    Tuple tuple;
    for (size_t col_idx : col_indices) {
        switch (columns_[col_idx].type) {
            case ColumnType::INT:    tuple.push_back(columns_[col_idx].int_data[row_idx]); break;
            case ColumnType::FLOAT:  tuple.push_back(columns_[col_idx].float_data[row_idx]); break;
            case ColumnType::VARCHAR:tuple.push_back(columns_[col_idx].string_data[row_idx]); break;
            case ColumnType::BOOL:   tuple.push_back(columns_[col_idx].bool_data[row_idx]); break;
        }
    }
    return tuple;
}

}  // namespace minidb
