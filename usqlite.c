/*
MIT License

Copyright(c) 2021-2023 Elvin Slavik

Permission is hereby granted, free of charge, to any person obtaining a copy
of this softwareand associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright noticeand this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include "usqlite_config.h"

#ifdef _MSC_VER
#pragma warning(push)
#endif

#if __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wempty-body"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#ifdef __APPLE__
// GCC on macOS 
#endif
#endif

// ------------------------------------------------------------------------------

#include "sqlite3.c"

// ------------------------------------------------------------------------------

#if __GNUC__
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif

#ifdef SQLITE_OMIT_ANALYZE
SQLITE_PRIVATE void sqlite3Analyze(Parse* pParse, Token* pName1, Token* pName2) {
}

#if __GNUC__
SQLITE_PRIVATE int sqlite3AnalysisLoad(sqlite3* db, int iDB) {
    return SQLITE_OK;
}
SQLITE_PRIVATE void sqlite3DeleteIndexSamples(sqlite3* db, Index* pIdx) {
}
#endif
#endif

#ifdef SQLITE_OMIT_ATTACH
SQLITE_PRIVATE void sqlite3Vacuum(Parse* p, Token* t, Expr* e) {
}
SQLITE_PRIVATE int sqlite3DbIsNamed(sqlite3* db, int iDb, const char* zName) {
    return 0;
}
SQLITE_PRIVATE void sqlite3Attach(Parse* p, Expr* e1, Expr* e2, Expr* e3) {
}
SQLITE_PRIVATE void sqlite3Detach(Parse* p, Expr* e) {
}

#ifdef __GNUC__
SQLITE_PRIVATE int sqlite3RunVacuum(char** c, sqlite3* db, int i, sqlite3_value* v) {
    return SQLITE_OK;
}
#endif
#endif

// ------------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if __GNUC__
#pragma GCC diagnostic pop
#endif

// ------------------------------------------------------------------------------
