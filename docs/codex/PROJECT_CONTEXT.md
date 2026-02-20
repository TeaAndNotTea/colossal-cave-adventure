# PROJECT_CONTEXT.md

## Overview

This project compiles legacy C (originating from FORTRAN) into WebAssembly using Emscripten.
The WASM module exports selected functions for use by a JavaScript visual layer.

The game logic is NOT modified.
Only export wrappers and JS integration are modified.

---

## Build Command

Example:

emcc main.c \
  -O2 \
  --preload-file advent/advent.dat@/advent.dat \
  -sMODULARIZE=1 \
  -sEXPORT_NAME='createAdventModule' \
  -sASYNCIFY \
  -sASYNCIFY_IMPORTS='["get_js_input"]' \
  -sFORCE_FILESYSTEM=1 \
  --js-library src_js/library_getin.js \
  -o dist/advent.js

Important:
- advent.dat must appear at WASM root (/advent.dat)
- Asyncify is required for JS input bridging

---

## Architecture

C/WASM:
- Provides core logic
- Exports object location and state functions
- Uses Emscripten virtual FS

JS Layer:
- play.js is entry point
- Responsible for:
  - Initializing WASM module
  - Wrapping exported functions
  - Rendering visual state
  - Managing UI state

No game logic duplication in JS.

---

## Constraints

- Do not modify legacy C logic unless explicitly requested.
- All JS must tolerate missing WASM exports gracefully.
- Visual layer must not block on synchronous FS access.

---

## Known Risks

- Changing export signatures breaks JS interop.
- Editing minified emcc output directly is unsafe.
- Asyncify misconfiguration causes silent hangs.

---

## Development Environment

- Code may be edited from multiple machines.
- Codex state stored in .codex/state/
- VS Code workspace may be subfolder-scoped.
