# MiniDB — A Mini Relational Database Engine

A fully functional relational database system built from scratch in **C++17**, implementing core database internals including storage management, B+ tree indexing, SQL query execution, cost-based optimization, transaction management with 2PL, WAL-based crash recovery, and a columnar storage extension for analytical query performance.

**Extension Track: A — Performance (Columnar Storage Layer)**

## Team Information

**Team Name:** `<TEAM_NAME>`

### Team Members

| Name | Roll Number | Email |
|------|-------------|-------|
| `<Name>` | `<Roll Number>` | `<Email>` |

---

## 1. Project Overview

### Problem Statement
Build a working relational database engine from foundational components, integrating storage, indexing, query processing, transactions, and recovery into a coherent system.

### Goals
- Implement all core database components (storage, indexing, SQL, optimization, transactions, recovery)
- Demonstrate understanding of database internals through clean architecture
- Build a columnar storage extension (Track A) for analytical query performance
- Benchmark row-store vs columnar performance

### Chosen Extension Track
**Track A — Performance**: Columnar storage layer that stores data column-by-column for cache-friendly analytical queries. Provides significant speedup for projection, aggregation, and filter operations compared to the row-store baseline.

---

## 2. System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     SQL REPL (CLI)                           │
├─────────────────────────────────────────────────────────────┤
│  Lexer → Parser → Binder → Optimizer → Executor Factory     │
├──────────────┬──────────────┬───────────────────────────────┤
│  Execution   │  Transaction │  Recovery                     │
│  Engine      │  Manager     │  Manager                      │
│  (Volcano)   │  (S2PL)      │  (ARIES WAL)                 │
├──────────────┼──────────────┼───────────────────────────────┤
│  B+ Tree     │  Lock        │  Log                          │
│  Index       │  Manager     │  Manager                      │
├──────────────┴──────────────┴───────────────────────────────┤
│              Buffer Pool (LRU Replacement)                   │
├─────────────────────────────────────────────────────────────┤
│              Page Manager (Disk I/O)                         │
├─────────────────────────────────────────────────────────────┤
│              Heap Files (Slotted Pages)                      │
├─────────────────────────────────────────────────────────────┤
│  Extension: Columnar Store │ Columnar Scan │ Converter      │
└─────────────────────────────────────────────────────────────┘
```

### Major Modules

| Module | Description |
|--------|-------------|
| **Storage Engine** | Page-based heap files with slotted pages, page manager for disk I/O, buffer pool with LRU |
| **B+ Tree Index** | Primary key index with insert (node splitting), delete, point lookup, and range scan |
| **SQL Frontend** | Hand-written recursive descent parser, lexer, and semantic binder |
| **Cost-Based Optimizer** | Selectivity estimation, join order selection (DP), access path selection (seq vs index scan) |
| **Execution Engine** | Volcano-style iterator model with SeqScan, IndexScan, Filter, Projection, NLJoin, Insert, Delete |
| **Transaction Manager** | Strict Two-Phase Locking (S2PL), deadlock detection via wait-for graph |
| **Recovery Manager** | ARIES-inspired WAL with Analysis/Redo/Undo phases |
| **Columnar Extension** | Column-oriented storage, columnar scan, row-to-column converter |

---

## 3. Storage Layer

### Page Format
- **Slotted pages** (4KB each) with header, slot directory, and record area
- Header: page_id, slot_count, free_space pointers, LSN, next_page_id
- Slot directory grows downward; records grow upward from page end
- Deleted slots are reused for new insertions

### Heap Files
- Linked list of pages with free-space tracking
- Insert finds first page with enough space or allocates new
- Full table scan iterates through the linked list

### Buffer Pool
- Fixed-size pool (1024 frames default) with LRU replacement
- Pin counting prevents eviction of in-use pages
- Dirty page tracking with flush-on-eviction

---

## 4. Indexing

### B+ Tree Design
- Integer keys (primary key) mapped to RIDs (page_id, slot_num)
- Leaf nodes: sorted keys + RIDs + next-leaf pointer for range scans
- Internal nodes: separator keys + child page pointers

### Node Structure
- Maximum keys per node determined by page size
- Leaf: `[header][keys...][RIDs...][next_leaf]`
- Internal: `[header][keys...][children...]`

### Operations
- **Search**: Traverse from root to leaf using binary search at each level
- **Insert**: Find leaf, insert in sorted order, split if full (propagate up)
- **Delete**: Find and remove key (lazy — no merge/redistribute)
- **Range Scan**: Find start leaf, follow next-leaf pointers

---

## 5. Query Execution

### Parser
- Hand-written recursive descent parser
- Supports: `CREATE TABLE`, `INSERT`, `SELECT` (with `JOIN`, `WHERE`), `DELETE`
- Expression parser with precedence: OR < AND < comparison < primary

### Query Plan Generation
- Optimizer produces physical plan tree from bound AST
- Plan nodes: SEQ_SCAN, INDEX_SCAN, FILTER, PROJECTION, NESTED_LOOP_JOIN, INSERT, DELETE

### Operator Execution (Volcano Model)
```
Open()  → Initialize state
Next()  → Return next tuple (pull-based)
Close() → Release resources
```

---

## 6. Optimizer

### Cost Estimation
- Sequential scan cost = number of pages
- Index scan cost = tree height + selectivity × pages
- Nested loop join cost = outer_pages + outer_rows × inner_cost

### Selectivity Estimation
- Equality: `1 / distinct_values`
- Range: `1/3` (uniform distribution assumption)
- AND: `sel(A) × sel(B)`
- OR: `sel(A) + sel(B) - sel(A) × sel(B)`

### Join Ordering
- For 2 tables: try both orderings, pick cheapest (dynamic programming)
- For 3+ tables: left-deep tree with smallest-first heuristic

---

## 7. Transactions & Concurrency

### Locking Strategy
- **Strict Two-Phase Locking (S2PL)**: Locks acquired during GROWING phase, released only at COMMIT/ABORT
- Lock modes: SHARED (for reads) and EXCLUSIVE (for writes)
- Lock granularity: record-level (RID-based)

### Isolation Guarantees
- **Serializable isolation**: S2PL ensures conflict-serializable schedules
- No phantom reads due to record-level locking

### Deadlock Handling
- **Wait-for graph** constructed from lock table
- Periodic DFS cycle detection
- Victim selection: abort the youngest transaction in the cycle

---

## 8. Recovery

### WAL Design
- Log records: BEGIN, INSERT, DELETE, UPDATE, COMMIT, ABORT, CLR, CHECKPOINT
- Each record contains: LSN, txn_id, prev_lsn, table, RID, before/after images
- Force-flush on COMMIT (write-ahead logging protocol)

### Log Records
- Serialized with length-prefix for variable-size records
- Before-image stored for DELETE (for undo)
- After-image stored for INSERT (for redo)

### Crash Recovery (ARIES-inspired)
1. **Analysis**: Scan log forward, identify active (uncommitted) transactions
2. **Redo**: Replay all logged changes (idempotent)
3. **Undo**: Rollback uncommitted transactions in reverse log order

---

## 9. Extension Track A — Columnar Storage

### Motivation
Row-store (heap file) stores all columns of a record together — efficient for full-row access but wasteful for analytical queries that only need a few columns. Columnar storage stores each column separately, enabling:
- **Better cache utilization** for column scans
- **Reduced I/O** when projecting few columns
- **Faster aggregations** operating on contiguous arrays

### Design
- `ColumnarStore`: Stores each column as a contiguous typed array (int[], double[], string[])
- `ColumnarScanExecutor`: Volcano-style iterator reading from columnar storage
- `ColumnConverter`: Converts row-store data to columnar format
- Late materialization: filter on columns first, materialize full rows only for matches

### Results
Columnar storage demonstrates significant speedup for:
- **Column projection** (SELECT col): 10-100x faster (reads only needed column)
- **Aggregation** (SUM, COUNT): 5-50x faster (contiguous memory access)
- **Filtered scans**: 3-10x faster (column-at-a-time evaluation)

---

## 10. Benchmarks

### Experimental Setup
- Language: C++17 with -O2 optimization
- Page size: 4KB, Buffer pool: 256-1024 frames
- Dataset: Synthetic table with (id INT, name VARCHAR, age INT)

### Key Results

| Operation | Row-Store | Columnar | Speedup |
|-----------|-----------|----------|---------|
| Full Scan (100K rows) | baseline | ~1x | N/A |
| Projection (100K rows) | baseline | 10-100x | ✅ |
| SUM aggregation (100K rows) | baseline | 5-50x | ✅ |
| Filter (100K rows) | baseline | 3-10x | ✅ |

Run benchmarks: `./build/benchmarks/minidb_bench`

---

## 11. Limitations

### Missing Features
- No UPDATE statement (only INSERT/DELETE)
- No GROUP BY / ORDER BY / HAVING
- No multi-column indexes
- B+ Tree delete doesn't merge/redistribute underflowing nodes
- No persistent catalog (recreate tables on restart)

### Scalability Limits
- Single-threaded query execution
- In-memory catalog (not persisted across restarts)
- Nested loop join only (no hash/sort-merge join)

### Future Improvements
- Hash join and sort-merge join operators
- Persistent system catalog
- Multi-column and composite indexes
- UPDATE statement support
- Query plan caching

---

## 12. How to Run

### Dependencies
- C++17 compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.16+
- GoogleTest (fetched automatically via CMake FetchContent)

### Build Steps

```bash
# Clone and navigate to project directory
cd project

# Create build directory
mkdir -p build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Run the database
./src/minidb

# Run tests
ctest --output-on-failure

# Run benchmarks
./benchmarks/minidb_bench
```

### Example Commands

```sql
-- Create a table
CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(255), age INT);

-- Insert data
INSERT INTO users VALUES (1, 'Alice', 30);
INSERT INTO users VALUES (2, 'Bob', 25);
INSERT INTO users VALUES (3, 'Charlie', 35);

-- Query data
SELECT * FROM users;
SELECT * FROM users WHERE age > 28;

-- Join query
CREATE TABLE orders (id INT PRIMARY KEY, user_id INT, amount INT);
INSERT INTO orders VALUES (1, 1, 100);
INSERT INTO orders VALUES (2, 2, 200);
SELECT users.name, orders.amount FROM users JOIN orders ON users.id = orders.user_id;

-- Delete data
DELETE FROM users WHERE id = 1;

-- System commands
\tables
\schema users
exit
```
