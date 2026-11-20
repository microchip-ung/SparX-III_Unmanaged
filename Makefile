# Copyright (c) 2006-2026 Microchip Technology Inc.
# SPDX-License-Identifier: MIT

# The firmware itself is built with Keil uVision (prj/luton26u.uvproj),
# not with make. This Makefile only produces the SBOM - see tools/sbom/README.txt.

.PHONY: all sbom clean

all: sbom

SBOM_DIR  := tools/sbom
SBOM      := sparx-iii-unmanaged-sbom.spdx.json
OUTLINE   := sparx-iii-unmanaged-sbom-outline.html

# Built images to checksum into the SBOM, relative to the repo root. Each glob
# that matches nothing is simply skipped, so 'make sbom' works on a fresh
# checkout too (the SBOM then describes the source only). Run it after the
# uVision build to include the images, or point it at them elsewhere, e.g.
#   make sbom IMAGES='release/*.hex'
IMAGES    ?= obj/*.hex

# Generate the SBOM at the repo root, plus a self-contained, browsable HTML
# outline of it (open it in any browser - no server, no network). Both are
# regenerated on every run, since they depend on the git revision and on
# whatever images are on disk, and both are git-ignored.
sbom:
	$(SBOM_DIR)/module_info_to_spdx.rb --root . --out $(SBOM) $(foreach g,$(IMAGES),--image '$(g)')
	$(SBOM_DIR)/spdx-outline-inline.rb --title 'SparX-III Unmanaged SBOM Outline Viewer' \
		--heading 'SparX-III Unmanaged SBOM Outline' $(SBOM) $(OUTLINE)

clean:
	rm -f $(SBOM) $(OUTLINE)
