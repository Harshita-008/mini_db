#include "extension/col_converter.h"

namespace minidb {

std::unique_ptr<ColumnarStore> ColumnConverter::Convert(HeapFile* heap_file,
                                                        const std::string& table_name,
                                                        const Schema& schema) {
    auto store = std::make_unique<ColumnarStore>(table_name, schema);

    // Scan all tuples from heap file
    std::vector<Tuple> tuples;
    heap_file->Scan([&](const RID& rid, const char* data, uint16_t length) -> bool {
        Tuple tuple = schema.DeserializeTuple(data, length);
        tuples.push_back(std::move(tuple));
        return true;
    });

    store->LoadFromTuples(tuples);
    return store;
}

}  // namespace minidb
