===============================================================================
SBOM tooling
===============================================================================

Tooling for a Software Bill of Materials of the code this repo owns, for CRA
(Cyber Resilience Act) compliance. SparX-III_Unmanaged is one small 8051
firmware code base built for a handful of Luton26 targets (L25, L16, L10,
L25UN) from the same source set, so the tooling is deliberately simple: one
repo-wide SBOM driven by the top-level .module_info.

The SBOM is generated ON THE FLY at the repo root as
sparx-iii-unmanaged-sbom.spdx.json, plus a browsable HTML rendering of it
(sparx-iii-unmanaged-sbom-outline.html) - both git-ignored, never committed.


What gets generated
-------------------------------------------------------------------------------

    sparx-iii-unmanaged-sbom.spdx.json  (git-ignored, on the fly, repo root)
                                        the code this repo owns (the firmware
                                        source, its eCos-licensed I2C header
                                        and the vendored Keil startup code),
                                        the Keil toolchain that
                                        builds it, plus a `files` entry per
                                        built image found on disk (see below).

    sparx-iii-unmanaged-sbom-outline.html
                                        (git-ignored, on the fly, repo root)
                                        the same document baked into a
                                        browsable standalone page (no server,
                                        no network - just open it), so a
                                        customer can read the SBOM without any
                                        SPDX tooling of their own.

The SBOM is built from the top-level .module_info (a multi-document YAML
manifest describing each component with its license/version/source), plus a
`files` entry per built image present on disk - a real SHA256 checksum,
related to the source it was compiled from (GENERATES) and the tools that
built it (BUILD_TOOL_OF), so the SBOM can also answer "what's in this specific
binary", not just "what source is in the repo".

The firmware is built by Keil uVision on Windows (prj/*.uvproj), not by make,
so the SBOM never triggers a build: it picks up whatever images the last
uVision build left in obj/ (the project's output directory, obj/*.hex by
default - override with IMAGES=, see below). With no images on disk the SBOM
describes the source only; that is not an error.

Every uVision target writes the same obj/luton26.hex, so only the target built
last is on disk. To record several targets in one SBOM, copy each image to its
own name after building it and point IMAGES= at the copies.

The document has a single top-level node, SPDXRef-release (what
SPDXRef-DOCUMENT DESCRIBES), representing the release, with a grouping package
per subtree hanging under it via CONTAINS:

    SparX-III Unmanaged release (sparx-iii-unmanaged-<ver>)  SPDXRef-release
      DIR: obj/                                     SPDXRef-dir-obj (if images)
        BIN: luton26.hex                            one `files` entry per image
      Group: Source components                      SPDXRef-group-source
        SRC: SparX-III Unmanaged switch firmware    (MIT, src/)
        SRC: VCore-III I2C driver header            (eCos license, i2c_h.h)
        SRC: Keil C51 startup code                  (startup.a51)
      Group: Host tools                             (only if any are declared)
        TOOL: <name>                                `category: Host Tools`
      TOOL: Keil C51 / uVision (MCS-51)             BUILD_TOOL_OF, not CONTAINed

<ver> is 'git describe --tags --dirty' with the tag's 'v' dropped, e.g.
3.11.0 on the v3.11.0 tag, 3.11.0-2-gbe4e30f two commits past it, and a -dirty
suffix when the tree has uncommitted changes - so an SBOM made from a modified
tree never passes for the clean revision. A .module_info entry with
`version: git` (the firmware's) gets the same value.

The `packages` array is emitted in that same order - SPDXRef-release first,
then each grouping node followed by what hangs under it - so the raw JSON
reads top-to-bottom the way the outline displays it. (SPDX itself attaches no
meaning to the order; the images are `files` entries, in their own array.)

Every node's name carries a kind prefix (SRC:/TOOL:/BIN:, alongside the
generated DIR:/Group: nodes) so it reads correctly wherever it surfaces - a
tool in particular also appears under each image it built. Since SPDX gives a
file only a 'fileName' path, the BIN: label is carried in an extra 'name'
field on the file entry (scanners ignore it); SPDXID and fileName are
untouched.

Two kinds of tool, told apart by the .module_info `category:`:

  - Host Tools: in the repo, run on the build machine (none today; the CLI
    text generator src/cli/maketxt.bat calls is not part of this repo).
    CONTAINed by the release, since they ship in its source, and BUILD_TOOL_OF
    each image.
  - Toolchain: the Keil C51 compiler/assembler/linker. Proprietary and NOT in
    this repo, so it ships in neither the release nor the images and is never
    CONTAINed - it hangs off the release and each image by BUILD_TOOL_OF
    alone. (Attached to the release too, so it stays in the tree when no
    images have been built.)

Both cross-cutting edges deliberately put the PACKAGE on the left-hand side
(source GENERATES image, tool BUILD_TOOL_OF image) rather than the more obvious
"image GENERATED_FROM source". GENERATES and GENERATED_FROM are inverses in
SPDX and mean the same thing, but syft/grype DROPS a package that is the
target of a GENERATED_FROM edge - it disappears from a CVE scan with no
warning. Don't flip these around. (The HTML outline displays the flip for
reading only - see spdx-outline.html below.)


Licensing picture
-------------------------------------------------------------------------------

    SRC: SparX-III ...        MIT (COPYING). Every source file carries
                              'SPDX-License-Identifier: MIT', apart from a few
                              generated/plain files (src/cli/l26_txtdef.*)
                              with no header, and the two below.
    SRC: VCore-III I2C ...    GPL-2.0-or-later WITH eCos-exception-2.0.
                              src/main/i2c_h.h carries the eCos license
                              header (Copyright Free Software Foundation), not
                              MIT. The exception keeps the GPL from extending
                              to the firmware it is compiled into, but the
                              file's own source must stay available.
    SRC: Keil ... code        LicenseRef (Keil). Keil's own STARTUP.A51,
                              copied in and adapted for Luton26; Keil
                              copyright, NOT MIT. Its attribution text
                              in .module_info is a summary - replace it with
                              the exact Keil license wording if legal requires.
    TOOL: Keil C51 / uVision  LicenseRef (Keil). Proprietary, licensed per
                              developer; not distributed.

The built images therefore conclude "MIT AND (GPL-2.0-or-later WITH
eCos-exception-2.0) AND <Keil LicenseRef>".


Make targets
-------------------------------------------------------------------------------

From the top-level Makefile:

    make sbom               Generate sparx-iii-unmanaged-sbom.spdx.json at
    (or just 'make')        the repo root plus the browsable
                            sparx-iii-unmanaged-sbom-outline.html beside
                            it. Run it after the uVision build to include
                            the image.
    make sbom IMAGES='dir/*.hex other/*.bin'
                            Same, checksumming images from elsewhere (globs
                            relative to the repo root; a glob matching nothing
                            is skipped).
    make clean              Remove both generated files.

Or run the scripts directly:

    tools/sbom/module_info_to_spdx.rb [--root DIR] [--out FILE] [--stdout] \
        [--image GLOB]...
    tools/sbom/spdx-outline-inline.rb [--title TEXT] [--heading TEXT] \
        <spdx.json> [output.html]


Scripts
-------------------------------------------------------------------------------

--- SBOM generation ---

module_info_to_spdx.rb
    Converts .module_info files in the repo into a single SPDX 2.3 JSON SBOM.
    .module_info is a multi-document YAML stream; each document describes a
    piece of code (license:, name:, version:, source:, optional
    purl:/cpe_product:/cpe:, category:, placeholder:, etc. - the format is
    documented at the top of the script). Each becomes an SPDX package -
    named "SRC: <name>", or "TOOL: <name>" when it declares a category: -
    with an SPDX license id, or its verbatim license text as a LicenseRef-*
    extracted license for a custom one, plus externalRefs (PURL and, when
    declared, CPE) for CVE matching. Then adds a `files` entry per built image
    (--image globs, default obj/*.hex) with a real SHA256 checksum, its
    GENERATES / BUILD_TOOL_OF edges, and the containment hierarchy described
    above.

--- SBOM browsing ---

spdx-outline.html
    Standalone, dependency-free SPDX viewer - open it directly in a browser
    (drag/drop a .spdx.json onto it, or "Open file..."). Renders the
    packages/files/relationships as a collapsible, searchable tree; click a
    node for its full metadata (license, checksums, externalRefs, etc.).
    Nothing is uploaded - it's pure client-side JS. Generic: works on any
    SPDX 2.3 JSON document, not just this repo's.

    Provenance is drawn from the artifact's side regardless of which way the
    document states it: a "source GENERATES image" edge is displayed flipped,
    as "image GENERATED_FROM source" under the image, so each BIN: node shows
    what it was built from. (Not flipped if the artifact has no other place in
    the tree, so the flip can never hide a node.)

spdx-outline-inline.rb
    Bakes one SPDX document into a standalone copy of spdx-outline.html, so
    it renders that SBOM immediately with no file picker. Drag/drop still
    works in the shipped copy to load a different document. --title names
    the generated page and --heading its on-page <h1>; once the document
    loads, the viewer relabels both after the release node ("SparX-III
    Unmanaged Outline").


Keeping it accurate
-------------------------------------------------------------------------------

    - Add a .module_info document when third-party code is vendored into the
      repo (anything whose copyright is not Microchip's), and when a host
      tool is checked in.
    - The SBOM is regenerated from scratch on every 'make sbom', so it never
      goes stale by itself - but it only knows what .module_info tells it.
    - Ruby scripts are MIT-licensed, 4-space indent, executable, and invoked
      directly (no 'ruby' prefix).


Not (yet) here
-------------------------------------------------------------------------------

    A CVE scan with VEX (Vulnerability Exploitability eXchange) triage of
    the findings is not included. The firmware has no third-party
    open-source components with CVE identities today (its purl is
    pkg:github/microchip-ung/sparx-iii_unmanaged, the eCos-licensed I2C
    header holds only this firmware's own prototypes, not upstream eCos
    code, and Keil publishes no CPE for the startup code), so a scan would
    have nothing to match. Add one if that changes.
