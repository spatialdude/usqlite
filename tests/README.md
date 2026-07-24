# usqlite regression tests

Host-side tests, run against a MicroPython unix build that includes this
module. On the Pico Computer 3 tree that is the `pc3` variant, and the heap
size must match the board (8 MB) so memory behaviour is faithful:

    cd ports/unix
    ./build-pc3/micropython -X heapsize=8m ../../lib/usqlite/tests/test_smoke.py

All scripts print `... PASSED` on success and use `/tmp` for their databases.

| script            | covers |
|-------------------|--------|
| `test_smoke.py`   | basic CRUD, executemany error-path leak, connection.close() with live/closed cursors, GC finaliser reclaim, row/dict/named-parameter binding |
| `test_simreset.py`| the soft-reset session cycle: `usqlite.__init__()` + `gc.collect()` simulate what a bare-metal soft reset does (roots are NOT auto-zeroed; the module `__init__` clears them on the next import), then the new session must reinitialize and close cleanly |
| `test_fix456.py`  | 64-bit INTEGER bind/read roundtrip, `.description` with computed columns (NULL decltype) and before first fetch, VFS errors surfacing as `usqlite_Error` (not exceptions unwinding SQLite), raising trace callback swallowed |
| `test_crash.py`   | power-fail recovery: snapshots of db+journal taken mid-transaction and post-commit are reopened; mid-transaction must roll back exactly, post-commit must keep data, a garbage journal must be ignored (`PRAGMA integrity_check` throughout) |
| `test_churn.py`   | heavy fetchall + gc.collect churn under an open connection, then close (the historical GC-vs-SQLite corruption pattern) |
