#include <cassert>
#include <cstdio>
#include "contact_tracker.hpp"

int main() {
    constexpr int64_t quiet = 10000, timeout = 1000000;
    ContactTracker t;
    t.reset(1000, 1, 0);
    assert(!t.complete(20000, 1, quiet, timeout)); // absent relay
    t.on_edge(5000, 0);
    assert(!t.complete(14999, 0, quiet, timeout));
    assert(t.complete(15000, 0, quiet, timeout));
    assert(t.first_target_us - t.start_us == 4000);
    assert(t.last_edge_us - t.start_us == 4000); // no debounce bias
    assert(!t.complete(15000, 1, quiet, timeout)); // changed before ISR

    t.reset(1000, 1, 0);
    t.on_edge(5000, 0);
    t.on_edge(5200, 1);
    assert(!t.complete(20000, 1, quiet, timeout)); // transient isn't success
    t.on_edge(6000, 0);
    t.on_edge(6100, 1);
    t.on_edge(8000, 0);
    assert(!t.complete(17999, 0, quiet, timeout));
    assert(t.complete(18000, 0, quiet, timeout));
    assert(t.edges == 5 && t.first_target_us == 5000 && t.last_edge_us == 8000);
    t.on_edge(9000, 0); // duplicate sampled level still resets quiet window
    assert(!t.complete(18000, 0, quiet, timeout));
    assert(t.complete(19000, 0, quiet, timeout));

    t.reset(100000, 0, 1); // release polarity and no stale previous edges
    assert(t.edges == 0 && !t.saw_target);
    t.on_edge(106000, 1);
    t.on_edge(107000, 0);
    t.on_edge(109000, 1);
    assert(t.complete(119000, 1, quiet, timeout));
    assert(t.first_target_us - t.start_us == 6000);
    assert(t.last_edge_us - t.start_us == 9000);

    t.reset(0, 1, 0);
    t.on_edge(timeout - quiet + 1, 0);
    assert(!t.complete(timeout + 100, 0, quiet, timeout)); // too late
    t.reset(0, 1, 0);
    t.on_edge(timeout - quiet, 0);
    assert(t.complete(timeout + 100, 0, quiet, timeout)); // delayed polling

    const int64_t long_uptime = 5000000000LL;
    t.reset(long_uptime, 0, 1);
    assert(t.stable(long_uptime + quiet, 1, quiet) == false);
    t.on_edge(long_uptime + 3000, 1);
    assert(t.complete(long_uptime + 13000, 1, quiet, timeout));
    assert(t.first_target_us - t.start_us == 3000); // beyond 32-bit micros

    t.reset(0, 1, 1); // baseline can be stable without a new edge
    assert(t.stable(quiet, 1, quiet));
    assert(!t.complete(quiet, 1, quiet, timeout));
    std::puts("PASS: bounce, polarity, baseline, timeout, timestamp and reset cases");
}
