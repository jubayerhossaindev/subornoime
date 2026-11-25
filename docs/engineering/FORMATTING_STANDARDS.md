# Formatting Standards

The single formatter is **clang-format**, driven by the repository-root
`.clang-format`. Formatting is deterministic: the same input always produces
the same output, and the CI gate rejects unformatted code.

## Policy highlights (`.clang-format`)

- Base: Google style, adapted — 4-space indent, 100-column limit, no tabs.
- Braces attach; short blocks never collapse onto one line.
- Pointers/references left-aligned (`Type* name`, `Type& name`).
- Includes are regrouped: main-file header, C system, C++ standard, angle
  brackets (Windows/ICU/SQLite), quoted project headers.
- `MaxEmptyLinesToKeep: 1`, `InsertNewlineAtEOF: true`,
  `ReflowComments: false` (comment text is never rewrapped by the machine).

## Usage

```bash
# check (CI + pre-push)
find src include tests benchmarks scripts examples cmake \
  -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' -o -name '*.inc' \
  | grep -v build | xargs clang-format --style=file --dry-run -Werror

# apply
… | xargs clang-format --style=file -i
```

VS Code: `clangd` formats C/C++ on save with `clang-format.style: "file"`.
EditorConfig (`.editorconfig`) keeps line endings, indentation and trailing
whitespace consistent for every other file type.

## Rules

1. Never hand-format — the formatter owns whitespace.
2. Never disable the formatter with `// clang-format off` except for genuinely
   alignment-sensitive data tables (and justify it in a comment).
3. The CI `naming-and-format` job fails on any formatting violation.
4. `.gitattributes` enforces LF in the repository; `.editorconfig` mirrors it
   for editors.
