# Packaging

Distribution entry points for SubornoIME, following the fcitx5
distro-distribution practice. Platform-neutral directory names keep room for
future rpm and Flatpack manifests without another reorganization.

| Directory          | Target            | Contents                                        |
| ------------------ | ----------------- | ----------------------------------------------- |
| `debian/`          | deb (Ubuntu/Debian) | `control`, `rules`, `changelog`, `copyright`, `install` |
| `appstream/`       | AppStream metainfo | `org.subornoIME.metainfo.xml`                   |
| Windows            | InnoSetup `.iss`  | **`windows/`** (source of truth)                |

## Windows

The InnoSetup script lives in `windows/` (`suborno_installer.iss`) and its
PowerShell lifecycle verification in
[`../scripts/test-windows-installer.ps1`](../scripts/test-windows-installer.ps1).
Build the installer from the repository root with
`iscc packaging/windows/suborno_installer.iss`. The CMake
build generates `suborno_version.iss.inc` next to the build directory, which
the installer script includes for the `AppVersion` define.

## Debian

Build a package from a clean checkout:

```sh
# copy packaging/debian as the debian/ metadata directory
cp -R packaging/debian debian/
dpkg-buildpackage -us -uc -b
```

`rules` enables the hardening flags, builds with the system SQLite3/ICU
packages (no vcpkg) and runs the `suborno.*` CTest suite unless
`DEB_BUILD_OPTIONS=nocheck` is set.

## AppStream

Validate before release (release checklist step):

```sh
appstreamcli validate packaging/appstream/org.subornoIME.metainfo.xml
```

The `<releases>` block must be updated together with `CHANGELOG.md` — the
release workflow treats a missing CHANGELOG entry as a tag blocker
(`docs/decisions/ADR-0003-repository-governance.md`).
