import usqlite, gc, os

try:
    os.remove("/tmp/phaseb.db")
except OSError:
    pass

db = usqlite.connect("/tmp/phaseb.db")
db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT, v REAL)")
db.execute("BEGIN")
for i in range(100):
    db.execute("INSERT INTO t (name, v) VALUES (?, ?)", ("row%d" % i, i * 1.5))
db.execute("COMMIT")
print("count:", db.execute("SELECT COUNT(*) FROM t").fetchone()[0])

# exact phase-B shape: COUNT fetchone + 20x (fetchall + churn + collect)
row = db.execute("SELECT COUNT(*), SUM(v) FROM t").fetchone()
assert row == (100, 7425.0), row
for k in range(20):
    x = ["pad%d" % i for i in range(2000)]
    rows = db.execute("SELECT * FROM t ORDER BY v DESC").fetchall()
    assert len(rows) == 100 and rows[0][2] == 148.5
    gc.collect()
print("churn done, mem:", usqlite.mem_current())
print("closing...")
db.close()
print("closed ok")
os.remove("/tmp/phaseb.db")
print("PHASE-B REPLICA PASSED")
