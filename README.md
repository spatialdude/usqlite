

# μSQLite library module for MicroPython

  

**WARNING**: This project is in the beta development stage and may be subject to change.

  
  

`usqlite` is a SQL database library module for [MicroPython](https://github.com/micropython/micropython) built on the popular [SQLite C library](https://sqlite.org/).

  

The `usqlite` SQL interface is designed to be a subset of the DB-API 2.0 specification as specified by [**PEP 249**](https://www.python.org/dev/peps/pep-0249). The `usqlite` API interface is also highly compatible with the standard [**`sqlite3`**](https://docs.python.org/3/library/sqlite3.html) library for Python with a few extra features.

  

Using the `usqlite` module in a MicroPython application is relatively simple. The application imports the `usqlite` library, connects to a database and then executes SQL commands. For example:

  

```python

import usqlite

  

if  not usqlite.mem_status():

usqlite.mem_status(True) # Enable memory usage monitoring

  

con = usqlite.connect("data.db")

  

con.executemany(

"BEGIN TRANSACTION;"

"CREATE TABLE IF NOT EXISTS data (name TEXT, year INT);"+

"INSERT INTO data VALUES ('Larry', 1902);"+

"INSERT INTO data VALUES ('Curly', 1903);"+

"INSERT INTO data VALUES ('Moe', 1897);"+

"INSERT INTO data VALUES ('Shemp', 1895);"+

"COMMIT;")

  

with con.execute("SELECT * from data") as cur:

for row in cur:

print("stooge:", row)

con.close()

  

print("usqlite mem - current:", usqlite.mem_current(), "peak:", usqlite.mem_peak())

  

```

  

The database files created or used by `usqlite` are compatible with SQLite database files created by applications on other platforms and operating systems such as [`DB Browser for SQLite`](https://sqlitebrowser.org/).

  

---

## Getting Started

  

The `usqlite` module is designed so that it can be easily compiled and included in MicroPython builds alongside other [external modules](https://docs.micropython.org/en/latest/develop/cmodules.html).

  

Version [**3.47.0**](https://sqlite.org/releaselog/3_47_0.html) of the [**SQLite** amalgamated source](https://sqlite.org/amalgamation.html) files `sqlite3.h` and `sqlite3.c` are included with the `usqlite` source code. These may be replaced with alternate or custom amalgamated versions built from the [canonical SQLite source code](https://sqlite.org/download.html).

  

### Project directory structure

  

The directory structure used in the develoment of this module is as shown below. The `usqlite` project can easily be modified to suit your own alternate project structure requirements with minor changes to the code.

  
  

```

~/<projectroot>

├── micropython # MicroPython source code

└── modules

├── micropython.cmake

├── usqlite # μSQLite source code

├── ...

└── ...

```

  
  

```sh

cd <projectroot>

mkdir  modules

mkdir  sqlite

git  clone  https://github.com/micropython/micropython.git

cd  modules

git  clone  https://github.com/spatialdude/usqlite.git

```

  

Typical `micropython.cmake`

  

```cmake

# This top-level micropython.cmake is responsible for listing

# the individual modules we want to include.

# Paths are absolute, and ${CMAKE_CURRENT_LIST_DIR} can be

# used to prefix subdirectories.

  

include(${CMAKE_CURRENT_LIST_DIR}/usqlite/micropython.cmake)

```

  

### Compiling

  
Refer to the [Wiki] for build instructions.-(https://github.com/spatialdude/usqlite/wiki)
Refer to the MicroPython's [Getting Started](https://github.com/micropython/micropython/wiki/Getting-Started) wiki and documentation for more details on setting up a build environment.

  

#### Ports

  

* [ESP32](https://github.com/spatialdude/usqlite/wiki/esp32)

* [Raspberry Pi Pico](https://github.com/spatialdude/usqlite/wiki/rp2)

* [Unix](https://github.com/spatialdude/usqlite/wiki/unix)

* [Windows](https://github.com/spatialdude/usqlite/wiki/windows)

* [Pybricks for smart LEGO® hubs](https://github.com/spatialdude/usqlite/wiki/pybricks)

  
  

### Custom Configurations

  

The default configuration of `usqlite` is intended to suit typical project requirements. This includes which **SQLite** components are included, memory allocation configuration and debug options.

  

The `usqlite` configuration settings can be found in the C header file [`usqlite_config.h`](https://github.com/spatialdude/usqlite/blob/main/usqlite_config.h).

  
  

### Memory allocation configuration

  

MicroPython builds often need to account for constrained memory enviroments. Fortunately the SQLite library is lightweight and has been designed so that it can be configured to accomodate many [different memory environment needs](https://sqlite.org/malloc.html).

  

**SQLite** does an excellent job of keeping memory usage as low as possible, so `usqlite` can be made to work well even in very tightly constrained memory spaces. The `usqlite` module provides functions that allow your application to monitor memory usage.

  

The default configuration of `usqlite` implements a custom dymanic memory allocator that uses MicroPython's GC heap. Memory demands placed on the heap will vary greatly depending on the complexity of the SQL of your application.

  

`usqlite` can be configured with an alternate memory configuration allocation that limits the memory to a fixed static heap size.

  

---

  

## `usqlite` library API

  

As the `usqlite` API interface is highly compatible with the standard [**`sqlite3`**](https://docs.python.org/3/library/sqlite3.html) library for for Python, much of the `sqlite3` documentation is also applicable to `usqlite`.

  

The details in this section will describe differences and API features unique to `usqlite`. Please also refer to the [**`sqlite3`**](https://docs.python.org/3/library/sqlite3.html) documentation as a general reference.

  

### **`usqlite`** global object

  

#### Constants

  

|Name|Type|Description|

|---|---|---|

|`version`|`str`|`usqlite` module version string e.g. `"0.1.8"`|

|`version_info`|`tuple`|`usqlite` module version tuple e.g `(0,1,8`)|

|`sqlite_version`|`str`|SQLite version string e.g. `"3.47.0"`|

|`sqlite_version_info`|`tuple`|SQLite version tuple e.g `(3,47,0`)|

|`sqlite_version_number`|`int`|SQLite version number e.g `3047000`|

  

#### Functions

  

|Signature|Return type|Description|

|---|---|---|

|`connect(<database>)`|`Connection`||

|`statement_complete(<sql>)`|`bool`||

|`mem_current()`|`int`|Current `usqlite` module memory usage in bytes.|

|`mem_peak()`|`int`|Peak `usqlite` module memory usage in bytes. Include optional `bool` parameter `True` to reset peak memory usage statistics.|

|`mem_status(<enable>)`|`bool`| Set or returns current status of memory usage monitoring. The memory usage status monitoring can be enabled or disabled via an optional `bool` parameter. The status can only be set on initialisation before the execution of any SQL.|

  

### **Connection** object

  

A `Connection` object is returned by the `usqlite.connect()` function.

  

#### Attributes

  

|Name|Type|Access|Description|

|---|---|---|---|

|`row_type`|`str`|`R/W`|Get/set row data type: `tuple` (default), `dict`, or `row`|

|`total_changes`|`int`|`R`||

  

#### Functions

  

|Name|Return type|Description|

|---|---|---|

|`close()`|`None`|Close the connection and all cursors associated with the connection.|

|`execute(<sql>)`|`Cursor`||

|`executemany(<sql>)`|`Cursor`||

|`set_trace_callback(<callback>)`|`None`||

  

### **Cursor** object

  

#### Attributes

  

|Name|Type|Access|Description|

|---|---|---|---|

|`arraysize`|`int`|`R/W`||

|`connection`|`Connection`|`R`||

|`description`|`list`|`R`||

|`lastrowid`|`int`|`R`||

|`rowcount`|`int`|`R`||

  

#### Functions

  

|Name|Return type|Description|

|---|---|---|

|`close()`|`None`||

|`execute(<sql>)`|`self`||

|`executemany(<sql>)`|`self`||

|`fetchone()`|`Any`||

|`fetchmany(<size>)`|`list`||

|`fetchall()`|`list`||

  
  

### Data row objects

  

The data type of rows returned by SQL statments is determined by the `Connection` object's `row_type` property. The default `row_type` is `tuple`.

  

If the `row_type` is `dict` then each row of data is returnened in as a `dict` object with the column value key names set to each value's respective column name.

  

The `row_type` row is a specialised type of `tuple` object with an the addional `keys` property that returns a `tuple` of column names.

  
  

### `execute` function parameter substitution

  

`usqlite` has an extended range of [SQL expression](https://sqlite.org/lang_expr.html) parameter substitution methods available.

  

#### `?` and `?NNN` indexed parameters

  

Indexed parameter values are be supplied as a `tuple` or `list`

  

For convenience, if the SQL statment contains a single `?` parameter, the parameter value can also be supplied as a single value.

  

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
