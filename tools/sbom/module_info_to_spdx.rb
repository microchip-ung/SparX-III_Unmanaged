#!/usr/bin/env ruby
# Copyright (c) 2006-2026 Microchip Technology Inc.
# SPDX-License-Identifier: MIT

# Convert every .module_info file in the repo into a single SPDX 2.3 JSON SBOM.
#
# .module_info format
# -------------------
# Each file is a multi-document YAML stream (leading "#" comment header, then
# "---"-separated documents). Every document describes a piece of code in the
# directory that contains the .module_info file:
#
#   directory: ./base          # or `file:` / `files:` - location under this dir
#   license: ISC               # license type (human name); required
#   name: ISC DHCP
#   version: 4.1.0             # or `git`: the described revision, from
#                               # 'git describe --tags' (see release_version)
#   source: http://.../dhcp    # upstream origin (URL) or a vendor name
#   description: DHCP Relay
#   attribution: |             # verbatim license text ...
#     Copyright ...
#   attribution_file: COPYING  # ... or a file (relative to the repo top)
#   placeholder: true          # directory need not exist on disk
#   category: Host Tools       # present => not shipped source:
#                               #   Host Tools - runs on the build host and is
#                               #     in the repo, so it ships in the release
#                               #     (CONTAINed) but only BUILD_TOOL_OF images;
#                               #   Toolchain  - the compiler/linker, NOT in the
#                               #     repo (e.g. Keil C51): never CONTAINed, only
#                               #     BUILD_TOOL_OF the release and each image.
#   purl: pkg:github/u-boot/u-boot@v2025.10   # optional: explicit Package URL(s)
#   cpe_product: net-snmp:net-snmp            # optional: CPE [part:]vendor:product,
#                                             # one or a list (version from version:)
#   cpe: cpe:2.3:a:openssl:openssl:3.0.13:*:*:*:*:*:*:*  # optional: full CPE override
#
# Documents with `license: VTSS` are the Microsemi build-system's own markers
# and carry no third-party license; they are skipped.
#
# SPDX 2.3 mapping
# ----------------
# Each remaining document becomes a package. A `license:` field that is a valid
# SPDX expression is emitted verbatim as licenseConcluded / licenseDeclared (the
# SPDX License List defines its text). A custom/proprietary license (no SPDX id,
# e.g. the Keil C51 tools license) instead becomes a LicenseRef-* extracted
# license backed by the verbatim text and referenced from those fields. The
# copyright notice line(s) found in the license text populate copyrightText
# (NOASSERTION when the text carries none).
#
# For CVE matching each package also gets SPDX externalRefs: a
# PACKAGE-MANAGER/purl locator (derived from `source`/`version`, or an explicit
# `purl:` field) and, when the document declares one, a SECURITY/cpe23Type
# locator (from `cpe_product:` or a full `cpe:` field).
#
# Built images
# ------------
# The firmware is built by Keil uVision (prj/*.uvproj, on Windows), outside
# this tooling. Every file matching an --image glob (default obj/*.hex, the
# uVision output directory; none found is not an error, just nothing to add
# yet) becomes an SPDX `files` entry with a real SHA256 checksum, related by
# GENERATES from each shippable-source package and BUILD_TOOL_OF from each
# host-tool/toolchain package - derived generically from the category:
# markers, not hardcoded by name.
#
# Output: <out> (default sparx-iii-unmanaged-sbom.spdx.json), an SPDX 2.3 JSON
# document. See tools/sbom/README.txt for the tree it describes.
#
# Usage:
#   tools/sbom/module_info_to_spdx.rb                 # scan cwd (git), write sparx-iii-unmanaged-sbom.spdx.json
#   tools/sbom/module_info_to_spdx.rb --root DIR --out FILE [--image GLOB]...
#   tools/sbom/module_info_to_spdx.rb --stdout

require 'json'
require 'optparse'
require 'yaml'
require 'time'
require 'open3'
require 'cgi'
require 'digest'

# Product identity: the SPDX document name, the release node's label and the
# documentNamespace all derive from these.
PRODUCT_ID = 'sparx-iii-unmanaged'
PRODUCT_NAME = 'SparX-III Unmanaged'
DEFAULT_IMAGES = ['obj/*.hex'].freeze

# Matches a real download locator (URL / VCS). Non-URL `source:` values such as
# "Keil Elektronik GmbH ..." are recorded in the package comment instead of
# downloadLocation.
DOWNLOAD_URL = %r{\A(https?|ftp|git|ssh|svn|bzr|hg)([+:]|://)}i

# Stable, machine-readable marker line added to the comment of a `category:`
# component: one that runs on the build machine rather than being compiled into
# the target binary, so downstream tooling can tell tools from shipped source
# without parsing the relationships.
BUILD_TOOL_MARKER = 'SBOM role: build-tool'

# `category:` value -> role. Case-insensitive and tolerant of singular/plural;
# a present-but-unrecognized category still means "not shipped source" and
# defaults to a host tool, but is reported so a typo or a new category value is
# noticed rather than silently bucketed.
HOST_CATEGORY = /host/i
TOOLCHAIN_CATEGORY = /toolchain/i

def category_role(category)
    return :toolchain if TOOLCHAIN_CATEGORY.match?(category)
    return :host_tool if HOST_CATEGORY.match?(category)
    warn "module_info_to_spdx: unrecognized category #{category.inspect}; " \
         'treating as a host tool'
    :host_tool
end

# SPDX License List identifiers used across this repo's .module_info `license:`
# fields, plus a margin of common ones (and the SPDX license exceptions used
# after WITH, e.g. eCos-exception-2.0). A `license:` value whose every token is
# on this list is emitted verbatim in licenseConcluded/licenseDeclared (the SPDX
# License List is the authoritative source of the text); anything else is a
# custom/proprietary license carried as a LicenseRef with the verbatim text.
SPDX_LICENSE_IDS = %w[
    0BSD Apache-1.1 Apache-2.0 Artistic-1.0 Artistic-2.0
    BSD-1-Clause BSD-2-Clause BSD-2-Clause-Patent BSD-3-Clause BSD-4-Clause
    BSL-1.0 CC0-1.0 CC-BY-4.0 CC-BY-SA-4.0 curl EPL-1.0 EPL-2.0 HPND ISC
    GPL-2.0-only GPL-2.0-or-later GPL-3.0-only GPL-3.0-or-later
    AGPL-3.0-only AGPL-3.0-or-later
    LGPL-2.0-only LGPL-2.0-or-later LGPL-2.1-only LGPL-2.1-or-later
    LGPL-3.0-only LGPL-3.0-or-later
    MIT MIT-0 MIT-CMU MPL-1.0 MPL-1.1 MPL-2.0 OpenSSL
    PSF-2.0 Python-2.0 RSA-MD Unlicense WTFPL X11 Zlib
    eCos-exception-2.0
].freeze

# Deprecated / legacy SPDX identifiers mapped to their current replacement so a
# `license:` field written the old way still yields a valid expression.
SPDX_DEPRECATED = {
    'GPL-2.0' => 'GPL-2.0-only', 'GPL-2.0+' => 'GPL-2.0-or-later',
    'GPL-3.0' => 'GPL-3.0-only', 'GPL-3.0+' => 'GPL-3.0-or-later',
    'LGPL-2.1' => 'LGPL-2.1-only', 'LGPL-2.1+' => 'LGPL-2.1-or-later',
    'LGPL-3.0' => 'LGPL-3.0-only', 'LGPL-3.0+' => 'LGPL-3.0-or-later',
    'AGPL-3.0' => 'AGPL-3.0-only', 'AGPL-3.0+' => 'AGPL-3.0-or-later'
}.freeze

# `license:` values that are deliberately not SPDX identifiers (the Keil C51
# tools license has none). Listed so the LicenseRef warning below stays quiet
# for this expected case and only fires for a genuinely unrecognized label (a
# typo or a new license this list hasn't caught up with yet).
KNOWN_CUSTOM_LICENSES = ['Keil'].freeze

# Normalize a `license:` field into a valid SPDX license expression, or nil when
# any token is not a known SPDX identifier (i.e. a custom license). Understands
# the AND / OR / WITH operators, parentheses, and deprecated-id remapping.
def spdx_expression(raw)
    s = raw.to_s.strip
    return nil if s.empty?
    tokens = s.gsub(/[()]/) { |c| " #{c} " }.split(/\s+/)
    out = tokens.map do |t|
        next t if t == '(' || t == ')'
        next t.upcase if t =~ /\A(?:AND|OR|WITH)\z/i
        id = SPDX_DEPRECATED[t] || t
        return nil unless SPDX_LICENSE_IDS.include?(id)
        id
    end
    out.join(' ').gsub('( ', '(').gsub(' )', ')')
end

# SPDX license-id fragments allow only [A-Za-z0-9.-].
def slugify(text)
    s = text.to_s.gsub(/[^A-Za-z0-9.]+/, '-').gsub(/\A-+|-+\z/, '')
    s.empty? ? 'unknown' : s
end

# A stable, human-readable package SPDXID derived from the component name,
# rather than a running counter. `used` tracks ids already emitted in this
# document; a numeric suffix disambiguates the rare case of two components
# sharing a name.
def package_spdx_id(name, used)
    base = "SPDXRef-#{slugify(name)}"
    id = base
    n = 1
    while used.key?(id)
        n += 1
        id = "#{base}-#{n}"
    end
    used[id] = true
    id
end

# Locate .module_info files the same way the release tooling does: via git, top
# file first, then sub-folders. Falls back to a filesystem walk outside git.
# Uses capture3 (not capture2) so that git's own "fatal: not a git
# repository" stderr - expected and harmless outside a checkout, e.g. an
# unpacked release tarball - is swallowed instead of leaking to the terminal
# and making a successful fallback look like a failure.
def module_info_files(root)
    out, _err, status = Open3.capture3('git', '-C', root, 'ls-files', '.module_info', '*/.module_info')
    if status.success? && !out.strip.empty?
        out.split("\n").map { |f| File.join(root, f) }
    else
        Dir.glob(File.join(root, '**', '.module_info'))
    end
end

# Run git in root; the stripped stdout, or nil outside a checkout.
def git_out(root, *args)
    out, _err, status = Open3.capture3('git', '-C', root, *args)
    status.success? ? out.strip : nil
end

# Version of the revision being described: 'git describe --tags' with the tag's
# leading 'v' dropped (v1.0.4 -> 1.0.4; 1.0.4-2-g7b91c2a two commits past it;
# a '-dirty' suffix for uncommitted changes, so a dirty-tree SBOM never passes
# for the clean revision). A released tarball has no .git, so outside a checkout
# this falls back to a '.version' YAML file at the root ("version: <ver>"),
# then to the short sha of a tag-less clone, then 'unknown'.
def release_version(root)
    v = git_out(root, 'describe', '--tags', '--dirty')
    return v.sub(/\Av(?=\d)/, '') unless v.nil? || v.empty?

    stamp = File.join(root, '.version')
    if File.file?(stamp)
        y = YAML.safe_load(File.read(stamp)) rescue nil
        return y['version'].to_s if y.is_a?(Hash) && !y['version'].to_s.empty?
    end

    git_out(root, 'rev-parse', '--short', 'HEAD') || 'unknown'
end

# Read the verbatim license text for a document: inline `attribution`, else the
# contents of `attribution_file` (relative to the repo top).
def license_text(doc, root)
    return doc['attribution'].to_s.chomp unless doc['attribution'].nil?

    unless doc['attribution_file'].nil?
        path = File.join(root, doc['attribution_file'])
        return File.read(path).scrub.chomp if File.file?(path)
    end
    nil
end

# Does this (already reflowed to one line, border-stripped) paragraph contain
# the start of a copyright block, either at the very start or right after a
# sentence break ("<period-or-colon><space>") earlier in the string? Returns the
# matched substring (from "Copyright" to the end of the paragraph) when it also
# carries a year or a "(c)" marker - that requirement is what separates a real
# notice from the many license-body sentences that merely mention the word
# ("the above copyright notice", "COPYRIGHT HOLDERS ... LIABLE") - or nil.
# A "Copyright (c)" anywhere also qualifies, since Keil's headers run it on from
# the lead-in with no sentence break ("... C51 Compiler package Copyright (c)").
def notice_block(paragraph)
    m = paragraph.match(/(?:\A|(?<=[.:]\s))copyright\b.*/i) ||
        paragraph.match(/\bcopyright\s+\(c\).*/i)
    return nil if m.nil?
    text = m[0]
    return nil unless text =~ /\b(?:19|20)\d\d\b/ || text =~ /\([cC]\)/
    text
end

# Split one confirmed copyright block (see notice_block) into individual
# notices, right before every "Copyright" occurrence - a no-op (one piece) for
# the common single-holder case.
# A trailing "SPDX-License-Identifier: ..." (COPYING carries one right under its
# notice) is license metadata, not part of the notice, and is dropped.
def notices_in_block(block)
    block.split(/(?=\bcopyright\b)/i)
         .map { |n| n.sub(/\s*SPDX-License-Identifier:.*\z/i, '').strip }
         .reject(&:empty?)
end

# Pull the copyright notice(s) out of a license text for the SPDX copyrightText
# field. A run of consecutive non-blank lines (a paragraph) is reflowed into
# one string first, since headers wrap a single notice across lines, and a
# blank line always ends a copyright block. Returns the unique notices joined
# by newlines, or nil when the text has none.
def copyright_notices(text)
    return nil if text.nil?
    lines = text.lines.map { |l| l.strip.sub(%r{\A[#*/;>]+\s*}, '').rstrip }
    notices = []
    para = []
    lines.each do |l|
        if l.empty?
            block = notice_block(para.join(' '))
            notices.concat(notices_in_block(block)) unless para.empty? || block.nil?
            para = []
        else
            para << l
        end
    end
    block = notice_block(para.join(' '))
    notices.concat(notices_in_block(block)) unless para.empty? || block.nil?
    notices.uniq!
    notices.empty? ? nil : notices.join("\n")
end

# Location(s) this document's license covers, relative to the repo top.
def covered_paths(doc, module_info_path, root)
    rels = ['directory', 'file', 'files'].flat_map { |k| doc[k].nil? ? [] : Array(doc[k]) }
    rels = ['./'] if rels.empty?
    base_dir = File.dirname(module_info_path)
    top = File.expand_path(root)
    rels.map do |rel|
        rp = File.expand_path(File.join(base_dir, rel)).sub(/\A#{Regexp.escape(top)}\/?/, '')
        rp.empty? ? '.' : rp
    end
end

# Extract [owner, repo, ref] from a GitHub source URL, or nil. Handles both
# github.com/<owner>/<repo> and raw.githubusercontent.com/<owner>/<repo>/<ref>/...
def github_ref(source)
    if source =~ %r{//raw\.githubusercontent\.com/([^/]+)/([^/]+)/([^/]+)/}i
        [$1, $2, $3]
    elsif source =~ %r{//(?:www\.)?github\.com/([^/]+)/([^/]+?)(?:\.git)?(?:[/#?]|\z)}i
        [$1, $2, nil]
    end
end

# Best-effort Package URL(s) for a document. An explicit `purl:` field wins;
# otherwise derive pkg:github/... from a GitHub source, else pkg:generic/... .
def derive_purls(doc, name, source, version)
    return Array(doc['purl']).map(&:to_s) unless doc['purl'].nil?

    if (gh = github_ref(source))
        owner, repo, ref = gh
        ver = version.to_s.empty? ? ref.to_s : version
        purl = "pkg:github/#{owner.downcase}/#{repo.downcase}"
        purl += "@#{ver}" unless ver.to_s.empty?
        return [purl]
    end

    if DOWNLOAD_URL.match?(source)
        purl = "pkg:generic/#{slugify(name).downcase}"
        purl += "@#{version}" unless version.to_s.empty?
        purl += "?download_url=#{CGI.escape(source)}"
        return [purl]
    end
    []
end

# CPE 2.3 part codes: application, operating system, hardware.
CPE_PARTS = %w[a o h].freeze

# CPE 2.3 identifier(s) for a document, from the .module_info itself:
#   * a full `cpe:` field is used verbatim (highest priority), else
#   * a `cpe_product:` field, each entry `<vendor>:<product>` or
#     `<part>:<vendor>:<product>` (part a/o/h, default a), yielding
#     cpe:2.3:<part>:<vendor>:<product>:<version>, with the version taken from
#     the `version:` field (or * if absent) so it stays in one place.
# Both fields take a single value or a list. Returns [] when neither is present
# (better no CPE than a wrong one that yields false CVE matches).
def derive_cpes(doc, version)
    return Array(doc['cpe']).map(&:to_s) unless doc['cpe'].nil?
    return [] if doc['cpe_product'].nil?

    # NVD CPE versions have no leading 'v' (git tags like v2025.10 do) - strip it.
    ver = version.to_s.empty? ? '*' : version.sub(/\Av(?=\d)/i, '')

    Array(doc['cpe_product']).map do |entry|
        fields = entry.to_s.split(':')
        fields.unshift('a') if fields.length == 2
        part, vendor, product = fields
        # A typo here silently drops the package out of CVE scanning, which
        # looks exactly like "no CVEs" - so complain rather than emit nothing.
        unless fields.length == 3 && CPE_PARTS.include?(part) && !vendor.empty? && !product.empty?
            warn "skip (malformed cpe_product: #{entry.inspect} - want [<part>:]<vendor>:<product>)"
            next
        end
        "cpe:2.3:#{part}:#{vendor}:#{product}:#{ver}:*:*:*:*:*:*:*"
    end.compact
end

# Assemble SPDX externalRefs (cpe23Type + purl) from the derived identifiers.
def external_refs(doc, name, source, version)
    refs = []
    derive_cpes(doc, version).each do |cpe|
        refs << { 'referenceCategory' => 'SECURITY',
                  'referenceType' => 'cpe23Type',
                  'referenceLocator' => cpe }
    end
    derive_purls(doc, name, source, version).each do |purl|
        refs << { 'referenceCategory' => 'PACKAGE-MANAGER',
                  'referenceType' => 'purl',
                  'referenceLocator' => purl }
    end
    refs
end

def rel(from, type, to)
    { 'spdxElementId' => from, 'relatedSpdxElement' => to, 'relationshipType' => type }
end

def build_document(root, image_globs)
    created = Time.now.utc.strftime('%Y-%m-%dT%H:%M:%SZ')
    version = release_version(root)
    sha = git_out(root, 'rev-parse', '--short', 'HEAD') || version
    packages = []
    files = []
    relationships = []
    extracted = []
    used_ids = {}
    idx = 0

    # Bucket each package's [SPDXID, licenseConcluded] by role, from the
    # category: markers - so the per-image file entries added below can relate
    # to "whatever is shippable source" / "whatever builds it" generically,
    # without hardcoding a package name.
    source = []
    host_tools = []
    toolchain = []

    module_info_files(root).sort.each do |path|
        stream =
            begin
                YAML.load_stream(File.read(path).scrub)
            rescue StandardError => e
                warn "skip (YAML error in #{path}): #{e.message}"
                next
            end

        stream.each do |doc|
            next unless doc.is_a?(Hash)
            next if doc['license'].nil? || doc['license'].to_s == 'VTSS'

            idx += 1
            license_name = doc['license'].to_s.chomp
            pkg_name = (doc['name'] || license_name).to_s
            spdx_id = package_spdx_id(pkg_name, used_ids)
            text = license_text(doc, root)

            # A standard license goes in verbatim as its SPDX identifier; the
            # SPDX License List is the authoritative source of its text, so we do
            # not embed it. A custom/proprietary license (no SPDX id) is carried
            # as a LicenseRef backed by the verbatim text, or NOASSERTION if the
            # document ships no text.
            expr = spdx_expression(license_name)
            if expr.nil?
                if text && !KNOWN_CUSTOM_LICENSES.include?(license_name)
                    warn "module_info_to_spdx: license #{license_name.inspect} is not a " \
                         'known SPDX expression; emitting it as a LicenseRef'
                end
                if text
                    license_ref = "LicenseRef-#{idx}-#{slugify(license_name)}"
                    extracted << {
                        'licenseId' => license_ref,
                        'name' => license_name,
                        'extractedText' => text
                    }
                    expr = license_ref
                else
                    expr = 'NOASSERTION'
                end
            end

            src = doc['source'].to_s
            download = DOWNLOAD_URL.match?(src) ? src : 'NOASSERTION'
            role = doc['category'].nil? ? :source : category_role(doc['category'])

            comment = []
            unless doc['category'].nil?
                comment << "Category: #{doc['category']}"
                comment << BUILD_TOOL_MARKER
            end
            if role == :toolchain
                comment << 'Not in this repository; used to build the release images.'
            else
                comment << "Covers: #{covered_paths(doc, path, root).join(', ')}"
            end
            comment << "Origin: #{src}" if !src.empty? && download == 'NOASSERTION'
            comment << "license_url: #{doc['license_url']}" unless doc['license_url'].nil?

            ver = doc['version'].nil? ? '' : doc['version'].to_s
            ver = version if ver == 'git'

            # Kind prefix on the display name (SRC:/TOOL:, alongside BIN:/DIR:/
            # Group: for the generated nodes) so a node's kind is readable on its own, wherever it surfaces in the tree - a
            # tool in particular also appears under each image it built.
            display_name = "#{role == :source ? 'SRC' : 'TOOL'}: #{pkg_name}"
            pkg = { 'SPDXID' => spdx_id, 'name' => display_name }
            pkg['versionInfo'] = ver unless ver.empty?
            pkg['downloadLocation'] = download
            pkg['filesAnalyzed'] = false
            pkg['licenseConcluded'] = expr
            pkg['licenseDeclared'] = expr
            pkg['copyrightText'] = copyright_notices(text) || 'NOASSERTION'
            pkg['description'] = doc['description'].to_s unless doc['description'].nil?
            pkg['comment'] = comment.join("\n")

            # Security identifiers for automated CVE matching.
            refs = external_refs(doc, pkg_name, src, ver)
            pkg['externalRefs'] = refs unless refs.empty?

            packages << pkg
            { source: source, host_tool: host_tools, toolchain: toolchain }[role] << [spdx_id, expr]
        end
    end

    # Relate each built image (produced by a prior Keil uVision build - none is
    # not an error, it just means there is nothing to add here yet) to the
    # source it was compiled from and the tools that built it, with a real
    # checksum so the SBOM can answer "what exactly is in this binary".
    top = File.expand_path(root)
    images = image_globs.flat_map { |g| Dir.glob(File.join(top, g)) }.uniq.sort
    # A compound expression ("GPL-2.0-or-later WITH eCos-exception-2.0") is
    # parenthesized when ANDed with the others, for readability.
    source_license = source.map(&:last).uniq.map { |e| e.include?(' ') ? "(#{e})" : e }.join(' AND ')
    source_license = 'NOASSERTION' if source_license.empty?
    images.each do |img_path|
        rel_img = img_path.sub(/\A#{Regexp.escape(top)}\/?/, '')
        file_id = "SPDXRef-File-#{slugify(rel_img)}"

        # SPDX gives a file only 'fileName' (a path, kept verbatim), so the BIN:
        # label is an extra 'name' field - harmless to the scanners, and what
        # spdx-outline.html displays in preference to the bare SPDXID.
        files << {
            'SPDXID' => file_id,
            'name' => "BIN: #{File.basename(img_path)}",
            'fileName' => "./#{rel_img}",
            'checksums' => [{ 'algorithm' => 'SHA256', 'checksumValue' => Digest::SHA256.file(img_path).hexdigest }],
            'licenseConcluded' => source_license,
            'copyrightText' => 'NOASSERTION'
        }

        # GENERATES, not the more obvious "file GENERATED_FROM source": the two
        # are inverses in SPDX and mean the same thing, but syft/grype DROPS a
        # package that is the *target* of a GENERATED_FROM edge, silently
        # removing it from a CVE scan. Keep the source on the left-hand side.
        source.each { |id, _| relationships << rel(id, 'GENERATES', file_id) }
        (host_tools + toolchain).each { |id, _| relationships << rel(id, 'BUILD_TOOL_OF', file_id) }
    end

    # Top-level node the whole tree hangs off: SPDXRef-DOCUMENT DESCRIBES exactly one root package (the release), which CONTAINS a
    # grouping package per subtree (the built images, the source components and
    # the host tools - each emitted only when non-empty), which in turn
    # CONTAINS the individual packages/files.
    #
    # The toolchain is the exception: it is not in the repo, so it ships in
    # neither the release nor the images and is never CONTAINed. It hangs off
    # the release (and each image) by BUILD_TOOL_OF alone - attached to the
    # release too, so it stays in the tree when no images have been built.
    #
    # The packages array is re-ordered into the order the tree renders in (see
    # spdx-outline.html): the release first, then each grouping node followed by
    # what hangs under it. SPDX attaches no meaning to that order; it only makes
    # the raw .spdx.json read top-to-bottom the way the outline displays it.
    release_id = 'SPDXRef-release'
    root_pkg = lambda do |id, name, description|
        { 'SPDXID' => id, 'name' => name, 'versionInfo' => version, 'downloadLocation' => 'NOASSERTION',
          'filesAnalyzed' => false, 'licenseConcluded' => 'NOASSERTION', 'licenseDeclared' => 'NOASSERTION',
          'copyrightText' => 'NOASSERTION', 'description' => description }
    end

    by_id = packages.to_h { |p| [p['SPDXID'], p] }
    ordered = []
    release = root_pkg.call(release_id, "#{PRODUCT_NAME} release (#{PRODUCT_ID}-#{version})",
                            "The #{PRODUCT_NAME} release: the built images plus the source components.")
    release['supplier'] = 'Organization: Microchip Technology Inc.'
    release['primaryPackagePurpose'] = 'FIRMWARE'
    ordered << release
    relationships << rel('SPDXRef-DOCUMENT', 'DESCRIBES', release_id)

    unless files.empty?
        dirs = files.map { |f| File.dirname(f['fileName'].sub(%r{\A\./}, '')) }.uniq
        dirs.each do |dir|
            dir_id = "SPDXRef-dir-#{slugify(dir)}"
            ordered << root_pkg.call(dir_id, "DIR: #{dir}/", "Built images in #{dir}/.")
            relationships << rel(release_id, 'CONTAINS', dir_id)
            files.select { |f| File.dirname(f['fileName'].sub(%r{\A\./}, '')) == dir }.each do |f|
                relationships << rel(dir_id, 'CONTAINS', f['SPDXID'])
            end
        end
    end

    groups = [
        ['SPDXRef-group-source', 'Group: Source components',
         'The source compiled into the release images, and its third-party components.', source],
        ['SPDXRef-group-host-tools', 'Group: Host tools',
         'Host-side tools that run on the build machine to produce the release, ' \
         'rather than being compiled into the images.', host_tools]
    ]
    groups.each do |group_id, name, description, members|
        next if members.empty?
        ordered << root_pkg.call(group_id, name, description)
        relationships << rel(release_id, 'CONTAINS', group_id)
        members.each do |id, _|
            relationships << rel(group_id, 'CONTAINS', id)
            ordered << by_id.delete(id)
        end
    end

    toolchain.each do |id, _|
        relationships << rel(id, 'BUILD_TOOL_OF', release_id)
        ordered << by_id.delete(id)
    end

    # Belt and braces: every package lands in exactly one bucket above, so
    # this is empty today - but a future role that misses all of them must
    # still end up in the document rather than being silently dropped.
    ordered.concat(by_id.values)

    doc = {
        'spdxVersion' => 'SPDX-2.3',
        'dataLicense' => 'CC0-1.0',
        'SPDXID' => 'SPDXRef-DOCUMENT',
        'name' => PRODUCT_ID,
        # Unique per revision (SPDX requires it): the version from git describe
        # already carries the sha past a tag, and the sha is appended for an
        # exact tag, so two revisions never claim the same identity.
        'documentNamespace' => "https://microchip.com/#{PRODUCT_ID}/#{PRODUCT_ID}-#{version}" \
                               "#{version.include?(sha) ? '' : "-#{sha}"}",
        'creationInfo' => {
            'created' => created,
            'creators' => ['Tool: module_info_to_spdx.rb', 'Organization: Microchip Technology Inc.']
        },
        'packages' => ordered.compact,
        'relationships' => relationships
    }
    doc['files'] = files unless files.empty?
    doc['hasExtractedLicensingInfos'] = extracted unless extracted.empty?
    [doc, idx]
end

def main
    opts = { root: '.', out: "#{PRODUCT_ID}-sbom.spdx.json", stdout: false, images: [] }
    OptionParser.new do |o|
        o.banner = 'Usage: module_info_to_spdx.rb [--root DIR] [--out FILE] [--stdout] [--image GLOB]...'
        o.on('--root DIR', 'repository root to scan (default: .)') { |v| opts[:root] = v }
        o.on('--out FILE', "output file (default: #{opts[:out]})") { |v| opts[:out] = v }
        o.on('--stdout', 'print JSON to stdout instead of writing a file') { opts[:stdout] = true }
        o.on('--image GLOB', 'built image(s) to checksum, relative to --root (repeatable; ' \
             "default: #{DEFAULT_IMAGES.join(' ')})") { |v| opts[:images] << v }
    end.parse!
    opts[:images] = DEFAULT_IMAGES if opts[:images].empty?

    document, n = build_document(opts[:root], opts[:images])
    text = "#{JSON.pretty_generate(document)}\n"
    if opts[:stdout]
        $stdout.write(text)
    else
        File.write(opts[:out], text)
        nfiles = document['files']&.size || 0
        puts "#{opts[:root]} -> #{opts[:out]}  (#{n} packages, #{nfiles} files)"
    end
end

main if $PROGRAM_NAME == __FILE__
