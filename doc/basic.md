# Focl Built-in Commands Reference

## Table of Contents
- [I/O Operations](#io-operations)
- [Variable Operations](#variable-operations)
- [Control Flow](#control-flow)
- [Type System](#type-system)
- [Type Conversion](#type-conversion)
- [Math & Random](#math--random)
- [String Operations](#string-operations)
- [List Operations](#list-operations)
- [Dict Operations](#dict-operations)
- [Filesystem Operations](#filesystem-operations)
- [Procedure Definitions](#procedure-definitions)
- [System & Utilities](#system--utilities)
- [Terminal Control](#terminal-control)

---

## I/O Operations

| Command | Syntax | Description |
|---------|--------|-------------|
| `puts` | `puts value` | Print value to stdout with newline |
| | `puts -nonewline value` | Print value to stdout without newline |
| | `puts $file value` | Write value to a file handle with newline |
| | `puts -nonewline $file value` | Write value to a file handle without newline |
| `gets` | `gets stdin` | Read one line from stdin, return it as a string |
| | `gets $file` | Read one line from a file handle, return it |
| | `gets stdin var` | Read one line from stdin into `var` |
| | `gets $file var` | Read one line from a file handle into `var` |
| `scan` | `scan stdin` | Read one line from stdin and parse it (int/float/string) |
| | `scan $file` | Read one line from a file handle and parse it |
| | `scan stdin var` | Read from stdin and assign the parsed value to `var` |
| | `scan $file var` | Read from a file handle and assign the parsed value to `var` |
| `open` | `open path mode` | Open a file, return a file handle (fopen modes) |
| `close` | `close handle` | Close a file handle |

**Examples:**
```
puts "Hello, World!"
puts -nonewline "Enter name: "
gets stdin name

set f [open "data.txt" w]
puts $f "line one"
puts -nonewline $f "no newline"
close $f

set g [open "data.txt" r]
set line [gets $g]      ; read one line
scan $g n               ; parse next line into n
close $g
```

---

## Variable Operations

| Command | Syntax | Description |
|---------|--------|-------------|
| `set` | `set name value` | Create or assign variable (strongly typed) |
| `unset` | `unset name` | Delete variable |
| `incr` | `incr name` | Increment variable by 1 |
| | `incr name n` | Increment variable by n |
| `upvar` | `upvar outer local` | Link an outer variable into current scope |
| `global` | `global name` | Link a global variable into current scope |
| `append` | `append var "text"` | Append string to variable |
| `isint` | `isint var` | Check if variable is integer type |

**Examples:**
```
set count 10
incr count        ; count becomes 11
incr count 5      ; count becomes 16
unset count
global x          ; access global x from local scope
```

---

## Control Flow

| Command | Syntax | Description |
|---------|--------|-------------|
| `if` | `if {condition} {block} ...` | Conditional with `elseif`/`else` clauses |
| `while` | `while {condition} {block}` | Loop while condition is true |
| `for` | `for {init} {condition} {update} {body}` | C-style for loop |
| `break` | `break` | Exit current loop immediately |
| `continue` | `continue` | Skip to next loop iteration |
| `return` | `return` | Return void from procedure |
| | `return value` | Return a value from procedure |

**Examples:**
```
if {x > 5} {
    puts "x is greater than 5"
} elseif {x == 5} {
    puts "x is exactly 5"
} else {
    puts "x is less than 5"
}

while {i < 10} {
    puts $i
    incr i
}

for {set i 0} {i < 5} {incr i} {
    puts "Iteration: $i"
}
```

---

## Type System

| Command | Syntax | Description |
|---------|--------|-------------|
| `typename` | `typename var` | Get type name as string |
| `typeid` | `typeid var` | Get type ID as integer |
| `isbuildin` | `isbuildin cmd` | Check if command is built-in |

**Supported Types:**
- `Integer` — 64-bit signed integer
- `Float` — Double-precision floating point (64-bit)
- `Boolean` — `true` or `false`
- `String` — UTF-8 encoded string
- `Void` — No value
- `List` — Ordered sequence of values
- `Dict` — Key/value map
- `ByteCode` — Compiled Focl code
- `Error` — Error object
- C Pointer — Opaque native handle; `typename` returns its registered name (e.g. file handles report `FILE`)

**Examples:**
```
set x 42
typename x        ; returns "Integer"
typeid x          ; returns type ID (integer)

set y 3.14
typename y        ; returns "Float"

set f [open "a.txt" w]
typename f        ; returns "FILE"
```

---

## Type Conversion

| Command | Syntax | Description |
|---------|--------|-------------|
| `asstring` | `asstring var` | Convert to string type |
| `asint` | `asint var` | Convert to integer type |
| `asfloat` | `asfloat var` | Convert to float type |

**Examples:**
```
set x 100
asstring x        ; converts 100 to "100"

set s "42"
asint s           ; converts "42" to integer 42

set f "3.14"
asfloat f         ; converts "3.14" to float 3.14
```

---

## Math & Random

| Command | Syntax | Description |
|---------|--------|-------------|
| `math::sin` | `math::sin x` | Sine (radians) |
| `math::cos` | `math::cos x` | Cosine (radians) |
| `math::tan` | `math::tan x` | Tangent (radians) |
| `math::log` | `math::log x` | Natural logarithm |
| `math::log10` | `math::log10 x` | Base-10 logarithm |
| `math::sqrt` | `math::sqrt x` | Square root |
| `math::abs` | `math::abs x` | Absolute value |
| `math::exp` | `math::exp x` | Exponential (e^x) |
| `math::degtorad` | `math::degtorad deg` | Degrees to radians |
| `math::radtodeg` | `math::radtodeg rad` | Radians to degrees |
| `math::inttofloat` | `math::inttofloat n` | Integer to float |
| `math::floattoint` | `math::floattoint f` | Float to integer (truncates) |
| `srand` | `srand` | Seed random with current time |
| | `srand n` | Seed random with n |
| `randi` | `randi min max` | Random integer in [min, max] |
| `randf` | `randf min max` | Random float in [min, max] |

**Examples:**
```
set pi 3.14159
math::sin $pi     ; sin(π) ≈ 0

math::degtorad 180  ; converts 180° to π radians

srand 12345
randi 1 100      ; random integer between 1 and 100
randf 0.0 1.0    ; random float between 0 and 1
```

---

## String Operations

| Command | Syntax | Description |
|---------|--------|-------------|
| `string` | `string length s` | Get character count (UTF-8 aware) |
| | `string index s i` | Get character at index |
| | `string range s i j` | Get substring from i to j (inclusive) |
| | `string compare s1 s2` | Compare (-1/0/1) |
| | `string equal s1 s2` | Check equality (boolean) |
| `append` | `append var "text"` | Append string to variable |

**Examples:**
```
set msg "Hello"
string length $msg    ; returns 5
string index $msg 0   ; returns "H"
string range $msg 1 3 ; returns "ell"

append msg " World!"  ; msg becomes "Hello World!"
```

---

## List Operations

| Command | Syntax | Description |
|---------|--------|-------------|
| `list` | `list [elem1] [elem2] ...` | Create a new list |
| `llength` | `llength list` | Get list length |
| `lindex` | `lindex list index` | Get element at index |
| `lappend` | `lappend list value` | Append value to list variable |
| `lrange` | `lrange list first last` | Get sublist from `first` to `last` (inclusive) |
| `lreverse` | `lreverse list` | Get a reversed copy of the list |
| `lsearch` | `lsearch list value` | Index of first matching value, or `-1` |
| `linsert` | `linsert list index elem ...` | Insert elements before `index` |
| `lreplace` | `lreplace list first last ?elem ...?` | Replace range `[first, last]` with the given elements |
| `lset` | `lset listVar index value` | Set the element at `index` in the list variable |
| `lassign` | `lassign list var1 var2 ...` | Assign elements to variables; returns the remaining list |

**Examples:**
```
set mylist [list 10 20 30 40]
llength $mylist         ; returns 4
lindex $mylist 2        ; returns 30
lappend mylist 50       ; list becomes [10 20 30 40 50]
lrange $mylist 1 3      ; returns [20 30 40]
lreverse $mylist        ; returns [50 40 30 20 10]
lsearch $mylist 30      ; returns 2
linsert $mylist 1 15    ; returns [10 15 20 30 40 50]
lreplace $mylist 1 2 x y; returns [10 x y 40 50]
lset mylist 0 99        ; mylist becomes [99 20 30 40 50]
set a 0
set b 0
set rest [lassign $mylist a b]   ; a=99, b=20, rest=[30 40 50]
```

---

## Dict Operations

| Command | Syntax | Description |
|---------|--------|-------------|
| `dict` | `dict create [key value ...]` | Create a new dict from key/value pairs |
| | `dict set dictVar key value` | Set `key` to `value` in the dict variable |
| | `dict get dict key` | Get the value for `key` |
| | `dict exists dict key` | Check whether `key` exists (boolean) |
| | `dict keys dict` | Get a list of all keys |
| | `dict values dict` | Get a list of all values |
| | `dict size dict` | Get the number of entries |
| | `dict unset dictVar key` | Remove `key` from the dict variable |

Keys are converted to strings; `dict set`/`dict unset` modify the named variable in place.

**Examples:**
```
set d [dict create a 1 b 2 c 3]
dict size $d            ; returns 3
dict get $d b           ; returns 2
dict exists $d a        ; returns true

dict set d e 5          ; d now has a=1 b=2 c=3 e=5
dict keys $d            ; e.g. [c e a b]
dict values $d          ; e.g. [3 5 1 2]
dict unset d a          ; remove key "a"
```

---

## Filesystem Operations

| Command | Syntax | Description |
|---------|--------|-------------|
| `file` | `file exists path` | Check if file/dir exists |
| | `file isfile path` | Check if normal file |
| | `file isdirectory path` | Check if directory |
| | `file size path` | Get file size in bytes |
| | `file mkdir path` | Create directory |
| | `file dirname path` | Get parent directory path |
| | `file realpath path` | Get absolute/real path |
| `sys::cp` | `sys::cp src dst` | Copy file to directory |
| | `sys::cp -r src dst` | Copy recursively (file or dir) |
| `sys::rm` | `sys::rm path` | Remove file |
| | `sys::rm -r path` | Remove recursively |
| `sys::mv` | `sys::mv src dst` | Move/rename file or directory |
| `sys::cat` | `sys::cat path` | Read and display file contents |
| `sys::edit` | `sys::edit path` | Edit file using external editor |

**Examples:**
```
file exists "/tmp/test.txt"   ; returns true/false
file isfile "myfile.txt"      ; returns true if normal file
file mkdir "mydir"
sys::cp "src.txt" "destdir/"
sys::rm -r "old_folder/"
sys::cat "config.txt"
```

---

## Procedure Definitions

| Command | Syntax | Description |
|---------|--------|-------------|
| `proc` | `proc name {args} {body}` | Define a procedure/function |
| `return` | `return` | Return void |
| | `return value` | Return a value |

**Examples:**
```
proc greet {name} {
    puts "Hello, $name!"
}

greet "Alice"

proc add {a b} {
    return [expr "$a + $b"]
}

set result [add 5 3]    ; result = 8
```

---

## System & Utilities

| Command | Syntax | Description |
|---------|--------|-------------|
| `eval` | `eval "code"` | Execute code string |
| `expr` | `expr "1 + 2 * 3"` | Evaluate arithmetic expression |
| `source` | `source path` | Execute a Focl source file |
| `error` | `error "message"` | Raise an error with the given message |
| `sys::exec` | `sys::exec "command"` | Run a system command |
| `sys::name` | `sys::name` | Get OS name (Windows/Linux/Mac OS/FreeBSD/Android) |
| `sys::sleepms` | `sys::sleepms n` | Sleep for n milliseconds |
| `curtime` | `curtime` | Get current time string |
| `exit` | `exit` | Exit with code 0 |
| | `exit n` | Exit with code n |
| `namespace` | `namespace import ns` | Import namespace |
| `proc` | See [Procedure Definitions](#procedure-definitions) | Define procedures |

**Examples:**
```
sys::name          ; returns "Linux" or "Windows", etc.
eval "puts Hello"
expr "2 + 3 * 4"   ; returns 14
source "lib.focl"
sys::exec "ls -la"
sys::sleepms 500
error "something went wrong"
exit 0
```

---

## Terminal Control

| Command | Syntax | Description |
|---------|--------|-------------|
| `term::clear` | `term::clear` | Clear terminal screen |
| `term::gotoxy` | `term::gotoxy col row` | Move cursor to column `col`, row `row` (both ≥ 1) |
| `term::getw` | `term::getw` | Get terminal width in columns |
| `term::geth` | `term::geth` | Get terminal height in rows |
| `term::hidecursor` | `term::hidecursor` | Hide terminal cursor |
| `term::showcursor` | `term::showcursor` | Show terminal cursor |
| `term::color` | `term::color list` | List all available color names |
| | `term::color name` | Set the output color (see names below) |
| | `term::color clear` | Reset colors back to default |
| `term::stringwidth` | `term::stringwidth s` | Get the display width of a string |

**Color names:** `black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`, `lightblack`, `lightred`, `lightgreen`, `lightyellow`, `lightblue`, `lightmagenta`, `lightcyan`, `lightwhite`, plus `reverse` / `noreverse` for inverse video.

**Examples:**
```
term::clear
term::gotoxy 10 5    ; move cursor to column 10, row 5
set cols [term::getw]
set rows [term::geth]
puts "Terminal size: ${cols}x${rows}"

term::color red
puts "this is red"
term::color reverse
puts "this is reversed"
term::color clear
```
