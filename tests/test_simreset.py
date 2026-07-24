import usqlite, gc, os

try:
    os.remove("/tmp/simreset.db")
except OSError:
    pass

# --- session 1: mirror sqltest_a end state ---------------------------------
db = usqlite.connect("/tmp/simreset.db")
db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT, v REAL)")
db.execute("BEGIN")
for i in range(100):
    db.execute("INSERT INTO t (name, v) VALUES (?, ?)", ("row%d" % i, i * 1.5))
db.execute("COMMIT")
dbopen = db     # connection deliberately left open across the "reset"
print("session 1 up, mem:", usqlite.mem_current())

# --- simulated soft reset ---------------------------------------------------
# Faithful to rp2: root pointers are NOT zeroed by the reset itself. What a
# new session does is (a) sweep all objects (finalisers run) and (b) call the
# module __init__ on its first import, which forgets the dead session's state.
del db, dbopen
gc.collect()        # ~ gc_sweep_all() at soft_reset_exit
usqlite.__init__()  # ~ first `import usqlite` of the new session
gc.collect()        # old pool is now unreferenced garbage
print("simulated soft reset done")

# --- session 2: mirror sqltest_b -------------------------------------------
db = usqlite.connect("/tmp/simreset.db")
row = db.execute("SELECT COUNT(*), SUM(v) FROM t").fetchone()
print("B1 count/sum:", row)
assert row == (100, 7425.0), row
for k in range(5):
    x = ["pad%d" % i for i in range(2000)]
    rows = db.execute("SELECT * FROM t ORDER BY v DESC").fetchall()
    assert len(rows) == 100 and rows[0][2] == 148.5
    gc.collect()
print("B2 churn ok, mem:", usqlite.mem_current())
print("B3 closing...")
db.close()
print("B3 closed ok")
os.remove("/tmp/simreset.db")
print("SIM-RESET TEST PASSED")
