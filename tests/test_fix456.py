import usqlite, gc, os

try:
    os.remove("/tmp/t456.db")
except OSError:
    pass

db = usqlite.connect("/tmp/t456.db")

# --- fix 4: 64-bit INTEGER bind + read ------------------------------------
db.execute("CREATE TABLE big (id INTEGER PRIMARY KEY, v INTEGER)")
vals = [2**40, 1753305600123, -2**45, 2**62, -1, 0, 2**31, -(2**31) - 1]
for i, v in enumerate(vals):
    db.execute("INSERT INTO big VALUES (?, ?)", (i, v))
back = [r[0] for r in db.execute("SELECT v FROM big ORDER BY id")]
assert back == vals, (back, vals)
row = db.execute("SELECT SUM(v) FROM big").fetchone()
assert row[0] == sum(vals), row
print("4: 64-bit roundtrip ok:", back[:3], "...")

# --- fix 5: decltype NULL + description before fetch ----------------------
cur = db.cursor()
cur.execute("SELECT COUNT(*), v + 1, v FROM big")
d = cur.description          # computed columns -> decltype NULL; was a crash
assert len(d) == 3, d
assert d[0][1] is None and d[1][1] is None and d[2][1] == "INTEGER", d
cur.fetchall()
cur2 = db.execute("SELECT id, v FROM big")
d2 = cur2.description        # before any fetch consumed rows -- was empty
assert len(d2) == 2 and d2[0][0] == "id", d2
cur2.fetchall()
print("5: description/decltype ok")

# --- fix 6: VFS errors as codes, not exceptions ---------------------------
try:
    usqlite.connect("/tmp/nosuchdir/x.db")
    raise SystemExit("expected connect to fail")
except usqlite.usqlite_Error as e:
    print("6: bad-path connect -> usqlite_Error:", e)
# engine must still be fully usable afterwards
assert db.execute("SELECT COUNT(*) FROM big").fetchone()[0] == len(vals)

def bad_trace(stmt):
    raise ValueError("boom")
db.set_trace_callback(bad_trace)
assert db.execute("SELECT COUNT(*) FROM big").fetchone()[0] == len(vals)
db.set_trace_callback(None)
print("6: raising trace callback swallowed ok")

db.close()
os.remove("/tmp/t456.db")
print("FIX 4/5/6 TESTS PASSED")
