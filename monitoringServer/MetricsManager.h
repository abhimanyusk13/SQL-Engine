#pragma once
#include <atomic>
#include <string>
#include <cstdint>

/// Singleton for collecting runtime metrics.
class MetricsManager {
public:
    /// Access the global instance.
    static MetricsManager &instance() {
        static MetricsManager inst;
        return inst;
    }

    /// Call when BufferManager finds a page in cache.
    void incBufferHit()   { bufferHits_.fetch_add(1, std::memory_order_relaxed); }
    /// Call when BufferManager loads a page from disk.
    void incBufferMiss()  { bufferMisses_.fetch_add(1, std::memory_order_relaxed); }

    /// Call at end of each query.
    void incQueryCount()  { queryCount_.fetch_add(1, std::memory_order_relaxed); }
    /// Record query latency in **microseconds**.
    void recordQueryLatencyUs(uint64_t us) {
        totalQueryLatencyUs_.fetch_add(us, std::memory_order_relaxed);
    }

    /// Call on COMMIT.
    void incTransactionsCommitted() { txCommitted_.fetch_add(1, std::memory_order_relaxed); }
    /// Call on ROLLBACK or abort.
    void incTransactionsAborted()   { txAborted_.fetch_add(1, std::memory_order_relaxed); }

    /// Produce a plain‐text dump of all metrics.
    std::string getMetrics() const {
        uint64_t hits      = bufferHits_.load();
        uint64_t misses    = bufferMisses_.load();
        uint64_t qcount    = queryCount_.load();
        uint64_t totalUs   = totalQueryLatencyUs_.load();
        uint64_t committed = txCommitted_.load();
        uint64_t aborted   = txAborted_.load();

        double avgLatMs = qcount>0
          ? (double)totalUs / (1000.0 * qcount)
          : 0.0;

        std::ostringstream oss;
        oss << "buffer_hits "      << hits      << "\n"
            << "buffer_misses "    << misses    << "\n"
            << "queries_executed " << qcount    << "\n"
            << "avg_query_ms "     << avgLatMs  << "\n"
            << "tx_committed "     << committed << "\n"
            << "tx_aborted "       << aborted   << "\n";
        return oss.str();
    }

private:
    MetricsManager() = default;
    ~MetricsManager() = default;

    std::atomic<uint64_t> bufferHits_{0};
    std::atomic<uint64_t> bufferMisses_{0};
    std::atomic<uint64_t> queryCount_{0};
    std::atomic<uint64_t> totalQueryLatencyUs_{0};
    std::atomic<uint64_t> txCommitted_{0};
    std::atomic<uint64_t> txAborted_{0};
};
