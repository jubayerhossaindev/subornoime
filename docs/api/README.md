# API documentation

Doxygen configuration for the public API surface (36 headers under
`include/suborno/`, umbrella header `include/suborno/suborno.h`). Modeled on
fcitx5's `Doxyfile.in`.

## Generate locally

```sh
mkdir -p build/api-docs
doxygen docs/api/Doxyfile.in
# output: build/api-docs/html/index.html
```

The `@PROJECT_NAME@`/`@PROJECT_VERSION@` placeholders are substituted by
CMake (`configure_file`) or by `sed` in CI; run Doxygen from a configured
build tree, or substitute manually before running.

## Publishing

The audit roadmap (phase 2, task 9) wires this job into CI as `docs/api`:
generate, validate zero warnings, publish `html/` as a workflow artifact
and to GitHub Pages on tagged releases. Keep the header list authoritative —
new public headers must be added to `INPUT` (or covered by the
`include/suborno/` wildcard, which is the default).
