# UI String Resources (i18n)

Localized UI strings for SubornoIME, introduced by structure audit #2
(gap 2). The layout mirrors how mature projects separate translatable
resources from code (Blender's `locale/`, Firefox's `intl/`): code carries
stable dotted keys, resources carry locale-specific values.

```
data/i18n/
├── en/ui.json   # English (reference locale)
└── bn/ui.json   # Bengali
```

## Contract

- **Flat dotted keys** (`topbar.menu.settings`, `settings.action.save`) are
  stable identifiers: never rename a key to change wording — rename means a
  migration, wording changes only touch values.
- **`en/ui.json` is the reference**: every other locale must carry exactly
  the same key set. The schema marker (`meta.schema:
  suborno.ui-strings/1.0`) and `meta.locale` stay in each file for
  tooling.
- **Groups** (`meta`, `topbar`, `settings`, `about`) are organizational only;
  flatten them when loading.

## Workflow

1. Add/change a string: edit `en/ui.json` first, mirror the key into
   `bn/ui.json` in the same change (missing keys are a review blocker).
2. Consumption: settings/topbar code currently shows English literals; the
   migration to `LoadString("group.key")` happens per-window as the desktop
   settings schema gains consumers. Do not ship a half-migrated window —
   move all literals of one window at once.
3. Packaging: ship `data/i18n/**` with the desktop application resources;
   the Inno Setup installer manifest picks up resource files centrally
   (see `packaging/windows/`).
4. Validation: `python3 -c "import json,sys;[json.load(open(p,encoding='utf-8')) for p in sys.argv[1:]]" data/i18n/*/ui.json`

## Adding a locale

Copy `en/ui.json` to `<locale>/ui.json`, translate values, set
`meta.locale`, and register the locale in the loader's locale list. Keep
locale codes BCP-47 lowercase (`bn`, `en`).
