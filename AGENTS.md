# AGENTS.md

## Scope Boundary

Treat the VS Code workspace as the hard boundary.
Do not read or modify files outside the workspace.

Do not scan or inspect secrets:
- .env
- key files
- private tokens
If encountered, stop and request clarification.

---

## Project Structure

Primary source directories:
- src/advent_C           (C / core logic, compiled to WASM with emscripten)
- src/js                 (JS visual layer, WASM loader and integration)
- plugin_skel/           (WordPress integration skeleton)
- advent                 (advent game data file)

Build artifacts:
- dist/
- build/*.wasm
- build/*.js (generated via emcc, other js files are manually created)
- libs_C      (generated with separate toolchain, yet to be integrated - ignore for now)

Large or irrelevant directories:
- assets/
- archive/
- dist.old/

Random junk files to ignore:
- package.json
- package-lock.json

Do NOT scan large directories unless explicitly required.

---

## Build Workflow

WASM is compiled via Emscripten.
If modifying C exports or interop:

1. Rebuild via documented build command (see PROJECT_CONTEXT.md)
2. Verify exported symbols
3. Confirm JS bridge compatibility

Do not refactor generated JS from emcc directly.

---

## Editing Rules

- Prefer minimal, reviewable diffs.
- Never rewrite large files unless explicitly requested.
- Avoid splitting files into excessive micro-modules without justification.
- Preserve existing build flags and linker flags unless asked.

---

## Persistent Knowledge

When architectural changes are made:
- Propose updates to docs/codex/PROJECT_CONTEXT.md
- Keep that file concise and factual.
