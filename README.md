# μSQLite MicroPython Module

**WARNING**: This project is in the beta development stage and may be subject to change.


`usqlite` is a SQL database module for MicroPython built on the popular [SQLite C library](https://sqlite.org/).

The `usqlite` SQL interface is designed to be a subset of the DB-API 2.0 specification as specified by [**PEP 249**](https://www.python.org/dev/peps/pep-0249). The `usqlite` API interface is also highly compatible with the standard [**`sqlite3`**](https://docs.python.org/3/library/sqlite3.html) library for Python with a few extra features.

Using the `usqlite` module in an application is relatively simple. The application imports the `usqlite` module, connects to a database and then executes SQL commands. For example:

```python
import usqlite

if not usqlite.mem_status():
    usqlite.mem_status(True) # Enable memory status monitoring

con = usqlite.connect("data.db")

con.executemany(
    "BEGIN TRANSACTION;"
    "CREATE TABLE IF NOT EXISTS data (name TEXT, year INT);"+
    "INSERT INTO data VALUES ('Larry', 1902);"+
    "INSERT INTO data VALUES ('Cury', 1903);"+
    "INSERT INTO data VALUES ('Moe', 1897);"+
    "COMMIT;")

with con.execute("SELECT * from data") as cur:
    for row in cur:
        print("stooge:", row)
        
con.close()

print("usqlite mem - current:", usqlite.mem_current(), "peak:", usqlite.mem_peak())

```

The SQLite database files created or used by `usqlite` are compatible with SQLite database files created by applications on other platforms and operating systems such as [`DB Browser for SQLite`](https://sqlitebrowser.org/).


## Getting Started

The `usqlite` module is designed so it can be easily compiled and included in MicroPython builds alongside other [external modules](https://docs.micropython.org/en/latest/develop/cmodules.html).


### SQLite C source code requirements

The `usqlite` module source code repository currently doesn't include the **SQLite** C source code files. You will need to supply the SQLite C source files to build `uqsqlite`. Fortunatley the required SQLite C source files are freely available for download from the 
**SQLite** [Download](https://sqlite.org/download.html) page. 

The [*amalagamated*](https://sqlite.org/amalgamation.html) version of the SQLite C source is required by `usqlite`. This simplifies the compilation requirements as only two of the SQLite C source files are needed by `usqlite`, namely `sqlite3.h` and `sqlite3.c` The default `usqlite` project configuration is `#include` these two SQLite C source files from a separate source directory.

### Project directory structure

The directory structure used in the develoment of this module is as shown below. The SQLite C source files are `sqlite3.h` and `sqlite3.c` should be placed in the `sqlite` directory. The `usqlite` project can easily be modified to suit your own alternate project structure requirements with minor changes to the code. 


```
~/<projectroot>
    /micropython            # MicroPython source code
    /modules
        micropython.cmake
        /usqlite3           # μSQLite source code 
        ...
        ...
    /sqlite                 # SQLite C amalgamated source code
        sqlite.h
        sqlite.c
```

`micropython.cmake`
```cmake
# This top-level micropython.cmake is responsible for listing
# the individual modules we want to include.
# Paths are absolute, and ${CMAKE_CURRENT_LIST_DIR} can be
# used to prefix subdirectories.

include(${CMAKE_CURRENT_LIST_DIR}/usqlite/micropython.cmake)
```


```sh
cd <projectroot>
git clone https://github.com/micropython/micropython.git
cd modules
mkdir modules
cd modules
git clone https://github.com/spatialdude/usqlite.git
```

### Compiling

#### **Unix/Linux**

The typical method of compiling `usqlite` on Unix/Linux systems is to use the `USER_C_MODULES` parameter with `make`.

e.g. Compiling on a Linux system targetting the Raspberry Pi Pico with a project root folder in the home directory named `pico`

```sh
cd ~/pico/micropython/ports/rp2
make clean
make USER_C_MODULES=~/pico/modules/micropython.cmake
```

#### **Windows**

Most of the development and debugging of `usqlite` is first done on **Windows** using **Visual Studio 2019** before cross compilation for other hardware such as the Raspberry Pi Pico.

`usqlite.props` is included in the source code as a convenience for inclusion in the Visual Studio project file `micropython.vcxproj`. 

To include `usqlite.props`, edit `micropython.vcxproj` in a text editor and add the following along side the other ```<Import.../>``` elements:

```xml
<Import Project="$(PyBaseDir)\..\modules\usqlite\usqlite.props"/>
```

### Custom Configurations

The default configuration of `usqlite` is intended to suit typical project requirements. This includes which **SQLite** components are included, memory allocation configuration and debug options.

The `usqlite` configuration settings can be found in the C header file [`usqlite_config.h`](https://github.com/spatialdude/usqlite/blob/main/usqlite_config.h). 


### Memory allocation configuration

MicroPython builds often need to account for constrained memory enviroments. Fortunatley the SQLite library is lightweight and has been designed to so that it can be configured to accomodate for many [different memory environment needs](https://sqlite.org/malloc.html).

**SQLite** does an excellent job of keeping memory usage as low as possible, so `usqlite` can be made to work well even in very tightly constrained memory spaces. The `usqlite` module provides functions that allow your application to monitor memory usage.

The default configuration of `usqlite` implements a custom dymanic memory allocator that uses MicroPython GC heap. Memory demands placed on the heap will vary greatly depending on the complexity of the SQL of your application. 

`usqlite` can be configured with an alternate memory configuration allocation fonfiguration that limits the memory to a fixed static heap size.

## `usqlite` module API

As the `usqlite` API interface is highly compatible with the standard [**`sqlite3`**](https://docs.python.org/3/library/sqlite3.html) library for for Python, much of the `sqlite3` documentation is also applicable to `usqlite`.

The details in this section will describe differences and API features unique to `usqlite`. Please also refer to the [**`sqlite3`**](https://docs.python.org/3/library/sqlite3.html) documentation as a general reference.

### **`usqlite`** global object

|Name|Type|Data type|Description|
|---|---|---|---|
|`version`|`const`|`str`|`usqlite` module version string e.g. `"0.1.0"`|
|`version_info`|`const`|`tuple`|`usqlite` module version tuple e.g `(0,1,0`)|
|`sqlite_version`|`const`|`str`|SQLite version string e.g. `"3.36.0"`|
|`sqlite_version_info`|`const`|`tuple`|SQLite version tuple e.g `(3,36,0`)|
|`sqlite_version_number`|`const`|`int`|SQLite version number e.g `3036000`|
|`connect`|`function`|`Connection`|`connect(<database>)`|
|`statement_complete`|`function`|`bool`||
|`mem_peak`|`function`|`int`|Peak `usqlite` module memory usage in bytes. Include optional `bool` parameter `True` to reset peak memory usage statistics.|
|`mem_current`|`function`|`int`|Current `usqlite` mmodule memory usage in bytes.|
|`mem_status`|`function`|`bool`| Set or returns current status of memeory usage monitoring. The memory usage status monitoring can be enabled or disabled via an optional `bool` parameter. The status can only be set on initialisation before the execution of any SQL.| 

### **Connection** object

A `Connection` is object is returned by the `usqlite.connect()` function.

|Name|Type|Data type|Description|
|---|---|---|---|
|`close`|`function`|`None`|Close the connection and all cursors associated with the connection.|
|`execute`|`function`|`Cursor`||
|`executemany`|`function`|`Cursor`||
|`set_trace_callback`|`function`|`None`||
|`row_type`|`attribute`|`str`|Get/set row data type `tuple` (default), `dict`, or `row`|
|`total_changes`|`attribute`|`int`||

### **Cursor** object

|Name|Type|Data type|Description|
|---|---|---|---|
|`arraysize`|`attribute`|`int`||
|`close`|`function`|`None`||
|`connection`|`attribute`|`Connection`||
|`description`|`attribute`|`list`|||
|`lastrowid`|`property`|`int`||
|`execute`|`function`|`self`||
|`executemany`|`function`|`self`||
|`fetchone`|`function`|`list`||
|`fetchmany`|`function`|`list`||
|`fetchall`|`function`|`list`||
|`rowcount`|`attribute`|`int`||


### Data row objects

The data type of rows returned by SQL statments is determined by the `Connection` object's `row_type` property. The default `row_type` is `tuple`.

If the `row_type` is `dict` then each row of data is returnened in as a `dict` object with the column value key names set to each value's respective column name.

The `row_type` row is a specialised type of `tuple` object with an the addional `keys` property that returns a `tuple` of column names.


### `execute` function parameter substition

`usqlite` has an extented range of [SQL expression](https://sqlite.org/lang_expr.html) parameter subsitution methods available.

#### `?` and `?NNN` indexed parameters

Indexed parameter values are be supplied as a `tuple` or `list`

For convenience, if the SQL statment contains a single `?` parameter, the paramater value can also supplied as a single value.

e.g.

```
con.execute("SELECT * FROM data WHERE year > ?", 1900)
```
is equivalent to
```
con.execute("SELECT * FROM data WHERE year > ?", (1900,))
```

#### `:AAAA`, `@AAAA` and `$AAAA` named parameters

Named parameters are passed as a `dict` object. The value keys must match the parameter names.

e.g.
```
con.execute("SELECT * FROM data WHERE name=:name", {"name":"Larry"})
```
