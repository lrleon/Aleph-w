#!/usr/bin/env ruby
# frozen_string_literal: true

# What Doxygen says of the public declarations of headers, for the header
# docstring coverage gate: whether it publishes documentation for each one.
# Doxygen is the authority because its rules are not obvious: "\overload
# name(args)" documents the named function, not the one below it; a trailing
# "///<" between the parameters and the body documents the last parameter,
# but after noexcept or const it documents the function.
#
# It runs once over all the headers, with the project's Doxyfile, writing only
# XML, with every declaration extracted and no documentation inherited.

require 'open3'
require 'rexml/document'
require 'tmpdir'

module DoxygenDeclarations
  # line is that of the name; bodystart, that of the definition
  Entry = Struct.new(:path, :type, :name, :line, :bodystart, :documented, keyword_init: true)

  PROJECT_DOXYFILE = File.expand_path('../Doxyfile', __dir__)
  OVERRIDES = {
    'RECURSIVE' => 'NO', 'EXCLUDE' => '', 'EXCLUDE_PATTERNS' => '', 'USE_MDFILE_AS_MAINPAGE' => '',
    'CREATE_SUBDIRS' => 'NO',
    'GENERATE_HTML' => 'NO', 'GENERATE_LATEX' => 'NO', 'GENERATE_XML' => 'YES',
    'GENERATE_TAGFILE' => '', 'XML_PROGRAMLISTING' => 'NO', 'EXTRACT_ALL' => 'YES',
    'EXTRACT_PRIVATE' => 'YES', 'EXTRACT_STATIC' => 'YES', 'EXTRACT_ANON_NSPACES' => 'YES',
    'INHERIT_DOCS' => 'NO', 'HAVE_DOT' => 'NO', 'QUIET' => 'YES', 'WARNINGS' => 'NO',
    'FULL_PATH_NAMES' => 'YES', 'STRIP_FROM_PATH' => '/nonexistent' # print whole paths
  }.freeze
  TYPE_COMPOUNDS = %w[class struct union concept].freeze

  module_function

  # Doxygen and the caller may spell a path through a symbolic link: compare
  # real paths, or every declaration would silently be judged by the text
  def key(path)
    File.realpath(File.expand_path(path))
  end

  def configuration(paths, output, overrides = {})
    settings = OVERRIDES.merge('INPUT' => paths.map { |p| %("#{p}") }.join(' '),
                               'OUTPUT_DIRECTORY' => %("#{output}")).merge(overrides)
    File.read(PROJECT_DOXYFILE) + "\n" + settings.map { |k, v| "#{k} = #{v}\n" }.join
  end

  # The name without its template arguments, which may contain "::"
  def without_arguments(name)
    plain = name.dup
    nil while plain.sub!(/<[^<>]*>/, '')
    plain.split('::').map(&:strip).join('::')
  end

  def text_of(element)
    element ? element.to_a.map(&:to_s).join.gsub(/<[^>]*>/, '').strip : ''
  end

  def documented?(node)
    !(text_of(node.elements['briefdescription']) +
      text_of(node.elements['detaileddescription'])).empty?
  end

  # {key(path) => [Entry]} of the functions and types of the headers, public or
  # not (the gate decides what is public). Raises if Doxygen does not report
  # every header, rather than let the gate judge all of their declarations by
  # the text. overrides is for the tests.
  def read(paths, overrides = {})
    return {} if paths.empty?

    inputs = paths.map { |p| key(p) }.uniq
    Dir.mktmpdir('header-doc') do |dir|
      config = configuration(inputs, File.join(dir, 'out'), overrides)
      _out, err, status = Open3.capture3('doxygen', '-', stdin_data: config)
      raise "doxygen failed: #{err.lines.last(5).join.strip}" unless status.success?

      found, reported = entries(File.join(dir, 'out', 'xml'))
      missing = inputs - reported
      raise "doxygen did not report #{missing.size} headers, as #{missing.first}" unless missing.empty?

      found
    end
  end

  # [the entries by path, the paths of the files that Doxygen reports]
  def entries(xml_dir)
    reported = []
    found = []
    # one file at a time: the XML of many headers does not fit well at once
    Dir[File.join(xml_dir, '*.xml')].sort.each do |file|
      next if File.basename(file) == 'index.xml'

      REXML::Document.new(File.read(file, encoding: 'utf-8')).elements.each('doxygen/compounddef') do |compound|
        reported << key(compound.elements['location'].attributes['file']) if compound.attributes['kind'] == 'file'
        found << type_entry(compound) if TYPE_COMPOUNDS.include?(compound.attributes['kind'])
        compound.elements.each('sectiondef/memberdef') do |member|
          found << member_entry(member) if %w[function friend].include?(member.attributes['kind'])
        end
      end
    end
    # a member may be listed both in its file and in its namespace or class
    [found.uniq { |e| [e.path, e.line, e.name] }.group_by(&:path), reported]
  end

  def entry(node, location, type:, name:)
    Entry.new(path: key(location.attributes['declfile'] || location.attributes['file']),
              type: type, name: name,
              line: (location.attributes['declline'] || location.attributes['line']).to_i,
              bodystart: location.attributes['bodystart']&.to_i, documented: documented?(node))
  end

  def type_entry(compound)
    name = without_arguments(compound.elements['compoundname'].text).split('::').last
    entry(compound, compound.elements['location'], type: true, name: name)
  end

  def member_entry(member)
    name = member.elements['name'].text
    name = without_arguments(name) unless name.start_with?('operator')
    entry(member, member.elements['location'], type: false, name: name)
  end
end
