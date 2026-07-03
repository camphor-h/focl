# Focl Built-in Commands

## I/O
| Command | Syntax | Description |
|---------|--------|-------------|
| `puts` | `puts "text"` | Print with newline |
| | `puts -nonewline "text"` | Print without newline |
| `gets` | `gets stdin var` | Read from stdin into variable |

## Variables
| Command | Syntax | Description |
|---------|--------|-------------|
| `set` | `set name value` | Create/assign variable (type-strict) |
| `unset` | `unset name` | Delete variable |
| `incr` | `incr name` | Increment by 1 |
| | `incr name n` | Increment by n |

## Control Flow
| Command | Syntax | Description |
|---------|--------|-------------|
| `if` | `if {cond} {block} ...` | Conditional with `elseif`/`else` |
| `while` | `while {cond} {block}` | Loop while condition true |
| `for` | `for {init} {cond} {update} {body}` | C-style for loop |
| `break` | `break` | Exit loop |
| `continue` | `continue` | Skip to next iteration |

## Types
| Command | Syntax | Description |
|---------|--------|-------------|
| `typename` | `typename var` | Get type name as string |
| `typeid` | `typeid var` | Get type ID as integer |

## Type Conversion
| Command | Syntax | Description |
|---------|--------|-------------|
| `asstring` | `asstring var` | Convert to string |
| `asint` | `asint var` | Convert to integer |
| `asfloat` | `asfloat var` | Convert to float |

## Math & Random
| Command | Syntax | Description |
|---------|--------|-------------|
| `srand` | `srand` | Seed with time |
| | `srand n` | Seed with n |
| `randi` | `randi min max` | Random integer in [min, max] |
| `randf` | `randf min max` | Random float in [min, max] |

## String
| Command | Syntax | Description |
|---------|--------|-------------|
| `string` | `string length s` | Get length |
| | `string index s i` | Get char at index |
| | `string range s i j` | Get substring |
| | `string compare s1 s2` | Compare (-1/0/1) |
| | `string equal s1 s2` | Check equality |
| `append` | `append var s` | Append string to var |

## File
| Command | Syntax | Description |
|---------|--------|-------------|
| `file` | `file exists path` | Check existence |
| | `file isfile path` | Check normal file |
| | `file isdirectory path` | Check directory |
| | `file size path` | Get file size |
| | `file mkdir path` | Create directory |
| | `file dirname path` | Get directory name |
| | `file realpath path` | Get absolute path |

## Procedures
| Command | Syntax | Description |
|---------|--------|-------------|
| `proc` | `proc name {args} {body}` | Define procedure |
| `return` | `return` | Return void |
| | `return value` | Return value |
| `upvar` | `upvar outer local` | Link outer var locally |

## System
| Command | Syntax | Description |
|---------|--------|-------------|
| `eval` | `eval "code"` | Execute code string |
| `expr` | `expr "1+2"` | Evaluate expression |
| `exec` | `exec "cmd"` | Run system command |
| `curtime` | `curtime` | Get current time string |
| `exit` | `exit` | Exit with code 0 |
| | `exit n` | Exit with code n |
| `isbuildin` | `isbuildin cmd` | Check if built-in |
