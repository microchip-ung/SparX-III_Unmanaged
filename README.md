VSC5609EV VSC5610EV-UN repository

## Software Bill of Materials (SBOM)

The repository can produce an SBOM in [SPDX 2.3](https://spdx.github.io/spdx-spec/v2.3/)
JSON format, describing the firmware source, the third-party code it contains,
and the toolchain it is built with.

### Generating it

Requirements: `make`, `ruby` and `git` (Linux, macOS or WSL).

From the repository root:

```sh
make sbom        # or just: make
```

The firmware itself is built with Keil uVision (`prj/luton26u.uvproj`), not
with make, so `make sbom` does not build anything. To include the built image
in the SBOM, run it after the uVision build: it picks up the `obj/luton26.hex`
that build produces (any `obj/*.hex`). All targets (L25, L16, L10, L25UN) write
that same file, so only the target built last is on disk; copy each image to
its own name to record several, and name them explicitly:

```sh
make sbom IMAGES='release/*.hex'
```

Without an image on disk the SBOM covers the source only. `make clean`
removes the generated files.

### What you get

Two files at the repository root (both git-ignored, regenerated on every run):

| File | Contents |
| --- | --- |
| `sparx-iii-unmanaged-sbom.spdx.json` | The machine-readable SBOM, for SBOM/CVE tools such as grype or osv-scanner. |
| `sparx-iii-unmanaged-sbom-outline.html` | The same SBOM as a standalone web page: open it in any browser (no server or network needed) to browse it as a collapsible, searchable tree. Click a node for its license, copyright, checksums and identifiers. |

The SBOM is organised as one release with these parts:

- **Built images**: each `.hex` found, with its SHA256 checksum and what it
  was built from and with.
- **Source components**:
  - the SparX-III Unmanaged switch firmware (`src/`, MIT license)
  - the VCore-III I2C driver header (`src/main/i2c_h.h`, GPL-2.0-or-later
    with the eCos exception)
  - Keil C51 startup code (`src/main/startup.a51`, Keil license)
- **Toolchain**: Keil C51 / uVision, recorded as the build tool. It is not part
  of this repository.

The release version is taken from `git describe --tags`, e.g. `3.11.0`.
A `-dirty` suffix means the SBOM was made from a tree with uncommitted changes.

The components are declared in `.module_info`; update it when third-party code
is added. See `tools/sbom/README.txt` for details of the tooling and the SBOM
structure.
