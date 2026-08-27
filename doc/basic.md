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
- [Filesystem Operations](#filesystem-operations)
- [Procedure Definitions](#procedure-definitions)
- [System & Utilities](#system--utilities)
- [Terminal Control](#terminal-control)

---

## I/O Operations

| Command | Syntax | Description |
|---------|--------|-------------|
| `puts` | `puts "text"` | Print text with newline |
| | `puts -nonewline "text"` | Print text without newline |
| `gets` | `gets stdin var` | Read one line from stdin into variable |

**Examples:**
```
puts "Hello, World!"
puts -nonewline "Enter name: "
gets stdin name
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
- `Compound` — List/vector type
- `ByteCode` — Compiled Focl code
- `Error` — Error object

**Examples:**
```
set x 42
typename x        ; returns "Integer"
typeid x          ; returns type ID (integer)

set y 3.14
typename y        ; returns "Float"
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
| `list` | `list [elem1] [elem2] ...` | Create a new list/compound |
| `llength` | `llength list` | Get list length |
| `lindex` | `lindex list index` | Get element at index |
| `lappend` | `lappend list value` | Append value to list |

**Examples:**
```
set mylist [list 10 20 30 40]
llength $mylist        ; returns 4
lindex $mylist 2       ; returns 30
lappend mylist 50      ; list becomes [10 20 30 40 50]
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
file isfile "myfile.txt"       ; returns true if normal file
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
| `exec` | `exec "command"` | Run system command |
| `sys::name` | `sys::name` | Get OS name (Windows/Linux/Mac OS/FreeBSD/Android) |
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
exec "ls -la"
exit 0
```

---

## Terminal Control

| Command | Syntax | Description |
|---------|--------|-------------|
| `term::clear` | `term::clear` | Clear terminal screen |
| `term::gotoxy` | `term::gotoxy x y` | Move cursor to (x, y) position |
| `term::getw` | `term::getw` | Get terminal width in columns |
| `term::geth` | `term::geth` | Get terminal height in rows |
| `term::hidecursor` | `term::hidecursor` | Hide terminal cursor |
| `term::showcursor` | `term::showcursor` | Show terminal cursor |

**Examples:**
```
term::clear
term::gotoxy 10 5    ; move cursor to column 10, row 5
set cols [term::getw]
set rows [term::geth]
puts "Terminal size: ${cols}x${rows}"
```
