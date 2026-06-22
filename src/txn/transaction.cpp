#include "txn/transaction.h"

namespace minidb {

Transaction::Transaction(txn_id_t txn_id)
    : txn_id_(txn_id), state_(TxnState::GROWING), prev_lsn_(INVALID_LSN) {}

}  // namespace minidb
