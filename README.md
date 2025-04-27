Client SQL →  [Connection Manager] 
               ↓
            [Parser] 
               ↓
        [Binder & Semantic Analyzer]
               ↓
       [Logical Plan Generator]
               ↓
         [Query Optimizer]
               ↓
      [Physical Plan Generator]
               ↓
         [Execution Engine]
               ↓
         [Storage Engine]
               ↓
         [Result Set ← Client]

1. Low-Level I/O & Buffering
1.1 File Manager
Minimum Requirements

Fixed-size pages (e.g. 8 KB) on disk

Ability to open, close, read, and write pages by page-ID

Space management: allocate/deallocate new pages

Integration Points

Buffer Manager calls readPage()/writePage() to load/store pages

Recovery Manager uses writeAheadLog() pages to disk

Functionalities to implement

openFile(filePath) → fileHandle

closeFile(fileHandle)

readPage(fileHandle, pageId, buffer)

writePage(fileHandle, pageId, buffer)

allocatePage(fileHandle) → newPageId

deallocatePage(fileHandle, pageId)

(Optional) getPageCount(fileHandle)

1.2 Buffer Manager
Minimum Requirements

In-memory pool of N page frames

Pin/unpin semantics to prevent eviction of in-use pages

Replacement policy (LRU or CLOCK)

Dirty-page tracking + flush

Integration Points

Uses File Manager to load/store pages

Exposes pages to B⁺-Tree, Storage Engine, and WAL

Functionalities to implement

initPool(numFrames)

fetchPage(fileHandle, pageId) → framePtr

pinPage(framePtr) / unpinPage(framePtr)

markDirty(framePtr)

flushPage(framePtr)

flushAllPages()

Internal: selectVictimFrame()

2. B⁺-Tree Index
Minimum Requirements

Search by key

Insert with leaf-split and propagation

Delete with merge/redistribute

Ordered range scan

Integration Points

Stores each node in a page via Buffer Manager

Returns RecordIDs → used by Storage Engine

Catalog holds index metadata

Functionalities to implement

Node structures: InternalNode, LeafNode (in-page layout)

search(key) → RecordID or NOT_FOUND

insert(key, RecordID)

splitLeaf(), splitInternal()

remove(key)

mergeOrRedistribute()

rangeScan(lowKey, highKey) → iterator over RecordIDs

serializeNode(node, framePtr) / deserializeNode(framePtr) → node

3. Storage Engine & Record Layout
3.1 Record Serialization
Minimum Requirements

Pack/unpack fixed schema to/from a byte buffer

Integration Points

Called by Storage Engine when writing/reading records

Uses schema from Catalog

Functionalities

serialize(schema, fieldValues[]) → byte[]

deserialize(schema, byte[] ) → fieldValues[]

3.2 Heap Table Manager
Minimum Requirements

Insert/delete/update records in heap pages

Maintain free-space directory on each page

Integration Points

Uses Buffer Manager to fetch pages

Returns new RecordIDs for B⁺-tree index building

Functionalities

openTable(tableName) / createTable(tableName, schema)

insertRecord(tableName, fieldValues[]) → RecordID

deleteRecord(RecordID)

updateRecord(RecordID, newValues[])

tableScan(tableName) → iterator over RecordIDs

3.3 Primary-Key Index Integration
Minimum Requirements

After insert/delete/update, keep B⁺-tree in sync

Integration Points

On insertRecord, call bpt.insert(pk, RecordID)

On deleteRecord, call bpt.remove(pk)

Functionalities

Hook methods in insert/delete/update to update index

4. Catalog & Metadata Manager
Minimum Requirements

Keep in-memory map of table schemas, column types, indexes, stats

Persist metadata on disk

Integration Points

Binder and Optimizer read schema/types

Storage Engine reads schema to know record layout

Functionalities to implement

loadCatalog(catalogPath)

saveCatalog(catalogPath)

getTableSchema(tableName) → Schema

addTable(tableName, Schema) / dropTable(tableName)

getColumnInfo(tableName, columnName)

registerIndex(tableName, indexName, columnList)

getIndexInfo(tableName)

updateTableStats(tableName, rowCount, histograms…)

5. SQL Frontend (Parser + Binder)
5.1 Parser
Minimum Requirements

Tokenize SQL text

Parse basic statements: SELECT, INSERT, UPDATE, DELETE, CREATE TABLE, BEGIN/COMMIT

Integration Points

Produces an AST consumed by Binder

Functionalities

tokenize(sqlText) → tokens[]

parseStatement(tokens[]) → ASTNode*

parseSelect(), parseInsert(), etc.

Error reporting with position

5.2 Binder / Semantic Analyzer
Minimum Requirements

Resolve table/column names

Type-check expressions

Expand *, validate aggregates

Integration Points

Queries Catalog for schemas/types

Annotates AST with resolved column IDs and types

Functionalities

bindAST(ASTNode*)

resolveTables(ASTNode*)

resolveColumns(ASTNode*, Schema*)

checkTypes(exprNode*)

expandStar(selectNode*)

validateAggregate(selectNode*)

6. Simple Executor & Physical Operators
6.1 PhysicalOperator Interface
Minimum Requirements

open(), next() → Row, close()

Functionalities

Define base class with these methods

6.2 Core Operators
Minimum Requirements & Integration

TableScan: pulls RecordIDs via Storage Engine → fetches records

Filter: applies predicates on rows

Project: computes output columns

Functionalities

TableScan(tableName, optionalIndexScanParams)

Filter(childOp, predicateExpr)

Project(childOp, outputExprList)

(Bonus) Limit, Sort, Aggregate

6.3 Executor
Minimum Requirements

Walk physical plan tree, call open/next/close

Materialize rows into result set

Functionalities

execute(PhysicalOperator* root) → vector<Row>

7. Query Planner & Optimizer
7.1 Logical Plan Builder
Functionalities

buildLogicalPlan(ASTNode*) → LogicalPlanNode*

7.2 Rule-Based Rewriter
Functionalities

pushDownPredicates(LogicalPlanNode*)

pruneProjections(LogicalPlanNode*)

flattenSubqueries(...)

7.3 Cost-Based Optimizer
Functionalities

estimateCardinality(node) using catalog stats

costModel(joinType, leftStats, rightStats)

enumerateJoinOrders(tables[]) (DP or heuristics)

7.4 Physical Plan Generator
Functionalities

chooseScanOperator(table, predicates) (table vs index scan)

chooseJoinOperator(joinTypeHint, stats) (hash vs nested-loop)

generatePhysicalPlan(logicalPlan)

8. Transactions, Concurrency & Recovery
8.1 Transaction Manager
Functionalities

beginTransaction() → txId

commit(txId)

rollback(txId)

8.2 Lock (or MVCC) Manager
Functionalities

2PL: lockShared(txId, resource), lockExclusive(txId, resource)

unlock(txId, resource)

8.3 Write-Ahead Log (WAL)
Functionalities

logInsert(txId, pageId, offset, oldBytes, newBytes)

logDelete(...), logUpdate(...)

logCommit(txId), logAbort(txId)

8.4 Recovery Manager
Functionalities

redo(log), undo(log) after a crash

checkpoint()

9. Connection Manager & SQL Protocol
Minimum Requirements

Accept TCP connections, read SQL strings, send back row results

Maintain per-session state (current TX, schema)

Functionalities

startListener(port)

acceptConnection() → Session*

Session::readQuery() → sqlText

Session::sendResult(rows)

Session::handleBegin/Commit/Rollback

(Optional) Wire-protocol implementation for PostgreSQL/MySQL clients

10. Extensions & Polish
10.1 UDF / Function Registry
registerFunction(name, signature, implementationPtr)

In Binder: recognize function names

In Executor: invoke function on each row

10.2 Statistics Collector
Background job to sample tables, build histograms

Expose ANALYZE tableName

10.3 Monitoring & Metrics
Track buffer hit ratio, query latencies, transaction throughput

Expose via a diagnostics API or simple HTTP server
