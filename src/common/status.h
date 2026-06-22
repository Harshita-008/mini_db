#pragma once

#include <string>
#include <stdexcept>

namespace minidb {

enum class StatusCode {
    OK,
    NOT_FOUND,
    DUPLICATE_KEY,
    OUT_OF_MEMORY,
    IO_ERROR,
    PAGE_FULL,
    BUFFER_FULL,
    DEADLOCK,
    TXN_ABORT,
    LOCK_CONFLICT,
    PARSE_ERROR,
    BIND_ERROR,
    TYPE_ERROR,
    INTERNAL_ERROR,
};

class Status {
public:
    Status() : code_(StatusCode::OK) {}
    Status(StatusCode code, const std::string& msg = "")
        : code_(code), message_(msg) {}

    static Status OK() { return Status(); }
    static Status NotFound(const std::string& msg = "") { return Status(StatusCode::NOT_FOUND, msg); }
    static Status DuplicateKey(const std::string& msg = "") { return Status(StatusCode::DUPLICATE_KEY, msg); }
    static Status OutOfMemory(const std::string& msg = "") { return Status(StatusCode::OUT_OF_MEMORY, msg); }
    static Status IOError(const std::string& msg = "") { return Status(StatusCode::IO_ERROR, msg); }
    static Status PageFull(const std::string& msg = "") { return Status(StatusCode::PAGE_FULL, msg); }
    static Status BufferFull(const std::string& msg = "") { return Status(StatusCode::BUFFER_FULL, msg); }
    static Status Deadlock(const std::string& msg = "") { return Status(StatusCode::DEADLOCK, msg); }
    static Status TxnAbort(const std::string& msg = "") { return Status(StatusCode::TXN_ABORT, msg); }
    static Status ParseError(const std::string& msg = "") { return Status(StatusCode::PARSE_ERROR, msg); }
    static Status BindError(const std::string& msg = "") { return Status(StatusCode::BIND_ERROR, msg); }
    static Status TypeError(const std::string& msg = "") { return Status(StatusCode::TYPE_ERROR, msg); }
    static Status InternalError(const std::string& msg = "") { return Status(StatusCode::INTERNAL_ERROR, msg); }

    bool ok() const { return code_ == StatusCode::OK; }
    bool IsNotFound() const { return code_ == StatusCode::NOT_FOUND; }
    bool IsDeadlock() const { return code_ == StatusCode::DEADLOCK; }

    StatusCode code() const { return code_; }
    const std::string& message() const { return message_; }

    std::string ToString() const {
        std::string result;
        switch (code_) {
            case StatusCode::OK: result = "OK"; break;
            case StatusCode::NOT_FOUND: result = "NOT_FOUND"; break;
            case StatusCode::DUPLICATE_KEY: result = "DUPLICATE_KEY"; break;
            case StatusCode::OUT_OF_MEMORY: result = "OUT_OF_MEMORY"; break;
            case StatusCode::IO_ERROR: result = "IO_ERROR"; break;
            case StatusCode::PAGE_FULL: result = "PAGE_FULL"; break;
            case StatusCode::BUFFER_FULL: result = "BUFFER_FULL"; break;
            case StatusCode::DEADLOCK: result = "DEADLOCK"; break;
            case StatusCode::TXN_ABORT: result = "TXN_ABORT"; break;
            case StatusCode::LOCK_CONFLICT: result = "LOCK_CONFLICT"; break;
            case StatusCode::PARSE_ERROR: result = "PARSE_ERROR"; break;
            case StatusCode::BIND_ERROR: result = "BIND_ERROR"; break;
            case StatusCode::TYPE_ERROR: result = "TYPE_ERROR"; break;
            case StatusCode::INTERNAL_ERROR: result = "INTERNAL_ERROR"; break;
        }
        if (!message_.empty()) {
            result += ": " + message_;
        }
        return result;
    }

private:
    StatusCode code_;
    std::string message_;
};

}  // namespace minidb
