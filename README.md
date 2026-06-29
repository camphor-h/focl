# Focl – Fast Optimized Command Language

**Focl** is a project that aims to create a Tcl dialect with strong typing and a more modern syntax.

---

## License

This project is licensed under the **MIT License**.

---

## Compilation

| Command             | Description                               |
|---------------------|-------------------------------------------|
| `make` / `make release` | Builds the program with `-O2 -flto` optimizations. |
| `make debug`        | Builds the program with debugging symbols (`-g`). |

---

## Usage

| Command                         | Description                         |
|---------------------------------|-------------------------------------|
| `focl`                          | Enters the Focl interactive REPL.   |
| `focl [Focl Source File]`       | Evaluates the specified Focl file.  |

---

## Embedding & Extending

- To embed Focl in your own project, include the `focl.h` header file.  
- To develop built‑in commands, use the `focl_dev.h` header.
