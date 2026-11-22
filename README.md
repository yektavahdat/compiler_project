\# B-Minor Compiler Front-End (Phase 1)



C++ implementation of the Phase 1 front-end for the \*\*B-Minor\*\* language, covering lexical analysis, LL-compatible recursive-descent parsing with AST construction and panic-mode error recovery, and AST-guided syntax highlighting (ANSI terminal \& standalone HTML/CSS).



\## Project Layout



\* `docs/grammar\_spec.ebnf` — Lexical rules and left-recursion-free EBNF grammar specification

\* `include/bm\_token.hpp` — Source position (`CodePos`), token kinds (`SymKind`), and lexeme structures

\* `include/bm\_scanner.hpp` \& `src/bm\_scanner.cpp` — Scanner with line/column tracking, keyword priority, maximal munch, and non-fatal lexical diagnostics

\* `include/bm\_ast.hpp` — Abstract Syntax Tree node definitions (`semType` reserved for semantic analysis)

\* `include/bm\_parser.hpp` \& `src/bm\_parser.cpp` — Recursive-descent parser with `panicSync()` error recovery

\* `include/bm\_colorizer.hpp` \& `src/bm\_colorizer.cpp` — AST-aware syntax highlighter supporting ANSI escape output and HTML/CSS export

\* `src/cli\_main.cpp` — Command-line driver (`--scan`, `--parse`, `--highlight`, `--html`)

\* `tests/sample\_ok.bminor` \& `tests/sample\_err.bminor` — Valid and erroneous sample inputs



\## Build \& Usage



Compile with a C++17 compiler (example using `g++`):



```bash

g++ -std=c++17 -Wall -Wextra -Iinclude src/bm\_scanner.cpp src/bm\_parser.cpp src/bm\_colorizer.cpp src/cli\_main.cpp -o bm\_front

