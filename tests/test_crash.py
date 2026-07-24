import usqlite, os

DB = "/tmp/crash.db"

def rm(p):
    try:
        os.remove(p)
    except OSError:
        pass

def cp(src, dst):
    rm(dst)
    try:
        with open(src, "rb") as s, open(dst, "wb") as d:
            while True:
                b = s.read(4096)
                if not b:
                    break
                d.write(b)
    except OSError:
        pass  # source may not exist (no journal)

def snapshot(tag):
    cp(DB, "/tmp/%s.db" % tag)
    cp(DB + "-journal", "/tmp/%s.db-journal" % tag)

def content(path):
    db = usqlite.connect(path)
    ic = db.execute("PRAGMA integrity_check").fetchone()[0]
    row = db.execute("SELECT COUNT(*), SUM(v), SUM(LENGTH(pad)) FROM t").fetchone()
    db.close()
    return ic, row

for f in os.listdir("/tmp"):
    if f.startswith("crash") and (f.endswith(".db") or f.endswith("-journal")):
        rm("/tmp/" + f)

# --- baseline: committed state -------------------------------------------
db = usqlite.connect(DB)
db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY, v INTEGER, pad TEXT)")
db.execute("BEGIN")
for i in range(100):
    db.execute("INSERT INTO t VALUES (?, ?, ?)", (i, i, "a" * 200))
db.execute("COMMIT")
db.close()
baseline = content(DB)
print("baseline:", baseline)
assert baseline[0] == "ok" and baseline[1][0] == 100

# --- big uncommitted transaction, snapshot mid-flight ---------------------
db = usqlite.connect(DB)
db.execute("PRAGMA cache_size=10")     # tiny cache -> spill writes db mid-txn
db.execute("BEGIN")
for i in range(100):
    db.execute("UPDATE t SET v = v + 1000000, pad = ? WHERE id = ?",
               ("z" * 2000, i))
jsz = os.stat(DB + "-journal")[6]
print("mid-txn journal bytes:", jsz)
assert jsz > 0, "journal must exist mid-transaction"
snapshot("crashA")                     # power cut mid-transaction

db.execute("COMMIT")
snapshot("crashB")                     # power cut just after commit
db.close()
committed = content(DB)
print("committed:", committed)
assert committed[0] == "ok" and committed[1][1] == sum(range(100)) + 100 * 1000000

# --- recovery A: mid-transaction cut must roll back to baseline -----------
recA = content("/tmp/crashA.db")
print("crashA recovered:", recA)
assert recA == baseline, (recA, baseline)
try:
    os.stat("/tmp/crashA.db-journal")
    print("note: crashA journal still present after recovery")
except OSError:
    print("crashA journal cleaned up")

# --- recovery B: post-commit cut must keep the committed data -------------
recB = content("/tmp/crashB.db")
print("crashB recovered:", recB)
assert recB == committed, (recB, committed)

# --- garbage journal next to a healthy db is ignored ----------------------
cp(DB, "/tmp/crashC.db")
with open("/tmp/crashC.db-journal", "wb") as f:
    f.write(bytes(range(256)) * 16)
recC = content("/tmp/crashC.db")
print("crashC (garbage journal):", recC)
assert recC == committed, (recC, committed)

print("CRASH RECOVERY TESTS PASSED")
