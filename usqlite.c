
#include "usqlite_config.h"

#ifdef _MSC_VER
#pragma warning(push)
#endif

#if __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

//------------------------------------------------------------------------------

#include "../../sqlite/src/sqlite3.c"

//------------------------------------------------------------------------------

#if __GNUC__
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif

#ifdef SQLITE_OMIT_ANALYZE
SQLITE_PRIVATE void sqlite3Analyze(Parse* pParse, Token* pName1, Token* pName2) {}

#if __GNUC__
SQLITE_PRIVATE int sqlite3AnalysisLoad(sqlite3* db, int iDB) {}
SQLITE_PRIVATE void sqlite3DeleteIndexSamples(sqlite3* db, Index* pIdx) {}
#endif
#endif

#ifdef SQLITE_OMIT_ATTACH
SQLITE_PRIVATE void sqlite3Vacuum(Parse*p, Token*t, Expr*e) {}
SQLITE_PRIVATE int sqlite3DbIsNamed(sqlite3*db, int iDb, const char* zName) { return 0; }
SQLITE_PRIVATE void sqlite3Attach(Parse*p, Expr*e1, Expr*e2, Expr*e3) {}
SQLITE_PRIVATE void sqlite3Detach(Parse*p, Expr*e) {}

#ifdef __GNUC__
SQLITE_PRIVATE int sqlite3RunVacuum(char**c, sqlite3*db, int i, sqlite3_value*v) {}
#endif
#endif

//------------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if __GNUC__
#pragma GCC diagnostic pop
#endif

//------------------------------------------------------------------------------
