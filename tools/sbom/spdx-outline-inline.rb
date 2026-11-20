#!/usr/bin/env ruby
# Copyright (c) 2006-2026 Microchip Technology Inc.
# SPDX-License-Identifier: MIT

# Bake an SPDX JSON document into a standalone copy of spdx-outline.html.
#
# The generic viewer (spdx-outline.html) opens with a drag/drop picker. This
# tool produces a self-contained HTML file with one SPDX document inlined, so it
# renders that SBOM immediately with no server and no file picker - handy for
# shipping a browsable SBOM alongside a per-binary deliverable. Drag/drop still
# works in the shipped copy if you want to load a different document.
#
# Usage:
#   spdx-outline-inline.rb [--title TEXT] [--heading TEXT] <spdx.json> [output.html]
#
# If output.html is omitted, writes <spdx-basename>-outline.html next to the
# input. --title names the generated page (browser tab / bookmark) and
# --heading its on-page <h1>; the template keeps its own generic wording for
# both, since it is a viewer for any SPDX document.

require 'json'
require 'optparse'
require 'cgi'

HERE        = File.dirname(File.expand_path(__FILE__))
TEMPLATE    = File.join(HERE, 'spdx-outline.html')
PLACEHOLDER = '<script id="spdx-data" type="application/json"></script>'

title = nil
heading = nil
parser = OptionParser.new do |o|
    o.banner = "Usage: #{$0} [--title TEXT] [--heading TEXT] <spdx.json> [output.html]"
    o.on('--title TEXT', "browser-tab title (default: the template's own)") { |v| title = v }
    o.on('--heading TEXT', "on-page <h1> (default: the template's own)") { |v| heading = v }
end
if ARGV.empty?
    STDERR.puts parser.banner
    exit 2
end
parser.parse!

src = ARGV[0]
out = ARGV[1] || File.join(File.dirname(src),
                           "#{File.basename(src, '.*').sub(/\.spdx$/, '')}-outline.html")

template = File.read(TEMPLATE, encoding: 'UTF-8')
unless template.include?(PLACEHOLDER)
    raise "placeholder not found in template #{TEMPLATE} (is it an up-to-date spdx-outline.html?)"
end

# Wording overridden per generated page, each keyed by the element that holds
# it. A tag the template no longer has is a hard error rather than a silently
# ignored option (same reasoning as the placeholder check above).
subs = { 'title' => title, 'h1' => heading }.compact.map do |tag, value|
    re = %r{<#{tag}>[^<]*</#{tag}>}
    unless template.match?(re)
        raise "<#{tag}> not found in template #{TEMPLATE} (is it an up-to-date spdx-outline.html?)"
    end
    [re, "<#{tag}>#{CGI.escapeHTML(value)}</#{tag}>"]
end

# Validate it is real JSON, then re-serialize compactly.
doc  = JSON.parse(File.read(src, encoding: 'UTF-8'))
data = JSON.generate(doc)

# Neutralize any "</..." that would prematurely close the <script> tag.
# "<\/" is still valid JSON (\/ is an escaped '/') and parses back identically.
data.gsub!('</', '<\/')

filled = %(<script id="spdx-data" type="application/json">#{data}</script>)
# Block form: avoids String#sub interpreting backslash escapes in the replacement.
html   = template.sub(PLACEHOLDER) { filled }
# Each tag sits in the <head>/<header>, i.e. ahead of the inlined data, so the
# first match is always the template's own - never something from the document.
subs.each { |re, replacement| html = html.sub(re) { replacement } }

File.write(out, html)
puts "wrote #{out} (#{html.bytesize} bytes, from #{src})"
