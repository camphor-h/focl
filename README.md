# Focl – Fast Optimized Command Language

**Focl** is a project that aims to create a Tcl dialect with strong typing and a more modern syntax.

---

## License

To prevent the abuse of MIT license (BSD better if you want a super free license!) and also make sure that we could get feedback from the user. Focl use a customed License, check the LICENSE under the directory to understand it.

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
