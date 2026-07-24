import usqlite, gc, os

try:
    os.remove("/tmp/smoke.db")
except OSError:
    pass

# --- basic sanity ---------------------------------------------------------
db = usqlite.connect("/tmp/smoke.db")
db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT, v REAL)")
db.executemany(
    "INSERT INTO t (name, v) VALUES ('a', 1.5);"
    "INSERT INTO t (name, v) VALUES ('b', 2.5)")
cur = db.execute("SELECT * FROM t ORDER BY id")
rows = cur.fetchall()
assert rows == [(1, 'a', 1.5), (2, 'b', 2.5)], rows
print("basic ok")

# --- fix 9: failed executemany must not leak the error message ------------
gc.collect()
base = usqlite.mem_current()
for i in range(50):
    try:
        db.executemany("INSERT INTO nosuch VALUES (1)")
        raise SystemExit("expected an error")
    except usqlite.usqlite_Error:
        pass
gc.collect()
leak = usqlite.mem_current() - base
print("executemany 50-error leak bytes:", leak)
assert leak < 512, leak

# --- fix 2: connection.close() with live / explicitly-closed cursors ------
cur2 = db.execute("SELECT * FROM t")   # unexhausted -> still registered
cur3 = db.execute("SELECT * FROM t")
cur3.close()                           # explicitly closed -> must untrack
db.close()
gc.collect()
for i in range(2000):                  # churn the heap: UAF would corrupt/crash
    x = [i] * 8
print("post-close rowcounts:", cur2.rowcount, cur3.rowcount)
assert cur2.fetchone() is None
assert list(cur2) == []
print("close-with-live-cursors ok")

# --- fix 8: dropped connection is closed by the finaliser -----------------
db2 = usqlite.connect("/tmp/smoke.db")
db2.execute("SELECT COUNT(*) FROM t").fetchone()
mid = usqlite.mem_current()
db2 = None
gc.collect()
after = usqlite.mem_current()
print("mem with-open-conn/after-collect:", mid, after)
assert after < mid, (mid, after)
print("finaliser ok")

# --- row_type + named params regression sweep -----------------------------
db3 = usqlite.connect("/tmp/smoke.db")
db3.row_type = "row"
r = db3.execute("SELECT name, v FROM t WHERE id = :i", {"i": 2}).fetchone()
assert tuple(r) == ('b', 2.5), r
assert r.keys == ('name', 'v'), r.keys
db3.row_type = "dict"
r = db3.execute("SELECT name FROM t WHERE id = ?", (1,)).fetchone()
assert r == {"name": "a"}, r
db3.close()
print("row/dict/named-params ok")

os.remove("/tmp/smoke.db")
print("ALL SMOKE TESTS PASSED")
