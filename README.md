## SQL Engine Roadmap

1. **Low‐Level I/O & Buffering**  
   Page‐oriented FileManager plus in‐memory BufferManager with pin/unpin and replacement policy.  
2. **B⁺-Tree Index**  
   Persistent, page-based B⁺-tree supporting search, insert (with splits), delete, and range scans.  
3. **Storage Engine & Record Layout**  
   Heap‐file manager with slotted pages, record (de)serialization, and PK index integration.  
4. **Catalog & Metadata**  
   In‐memory catalog of tables, columns, indexes, and statistics, with on‐disk persistence.  
5. **SQL Frontend (Parser & Binder)**  
   Recursive‐descent parser → AST, plus name‐resolution, type‐checking, `*`‐expansion, and semantic validation.  
6. **Executor & Physical Operators**  
   Iterator‐style operators (Scan, Filter, Project, Join) and a top‐level Executor driving `open/next/close`.  
7. **Query Planner & Optimizer**  
   AST→logical plan, rule-based rewrites (push-down, pruning), cost-based join ordering, and physical plan generation.  
8. **Transactions, Concurrency & Recovery**  
   2PL TransactionManager + LockManager, WAL logging, crash Recovery (REDO/UNDO), and ACID enforcement.  
9. **Connection Manager & SQL Protocol**  
   TCP listener reading semicolon-terminated SQL, dispatching to QueryEngine, and streaming results/errors.  
10. **Extensions & Monitoring**  
    UDF registry, `ANALYZE` table statistics, and a metrics HTTP server exposing buffer, query, and transaction stats.

---

## Future Work to Reach Production Grade

- **Full SQL Feature Support**: `JOIN … ON`, subqueries, window functions, DDL (ALTER, DROP), prepared statements, views, transactions savepoints.  
- **Advanced Concurrency**: MVCC for snapshot isolation, deadlock detection/avoidance, row-level locking.  
- **Query Optimization**: Cost model refinements, histograms, selectivity estimation, multi-way and bushy joins, parallel execution.  
- **Storage Enhancements**: Write-optimized storage (log-structured or columnar), compression, pluggable storage engines.  
- **High Availability & Scaling**: Replication, sharding, distributed query processing, fault tolerance.  
- **Security & Auditing**: Authentication/authorization, roles/privileges, audit logging, encryption at rest/in transit.  
- **Monitoring & Tooling**: SQL profiler, execution plan visualizer, comprehensive metrics dashboard, integration with standard clients (PostgreSQL/MySQL wire protocols).  
