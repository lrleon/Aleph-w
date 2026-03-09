#!/usr/bin/env ruby

# Versión de PRUEBA - solo muestra qué cambiaría sin modificar archivos

require 'fileutils'

GPL_PATTERN = /This program is free software: you can redistribute it and\/or modify\s+it under the terms of the GNU General Public License as published by\s+the Free Software Foundation, either version 3 of the License, or\s+\(at your option\) any later version\.\s+This program is distributed in the hope that it will be useful, but\s+WITHOUT ANY WARRANTY; without even the implied warranty of\s+MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE\. See the GNU\s+General Public License for more details\.\s+You should have received a copy of the GNU General Public License\s+along with this program\. If not, see <https:\/\/www\.gnu\.org\/licenses\/>\./m

MIT_TEXT = "Permission is hereby granted, free of charge, to any person obtaining a copy\n" +
  "  of this software and associated documentation files (the \"Software\"), to deal\n" +
  "  in the Software without restriction, including without limitation the rights\n" +
  "  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n" +
  "  copies of the Software, and to permit persons to whom the Software is\n" +
  "  furnished to do so, subject to the following conditions:\n\n" +
  "  The above copyright notice and this permission notice shall be included in all\n" +
  "  copies or substantial portions of the Software.\n\n" +
  "  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n" +
  "  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n" +
  "  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n" +
  "  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n" +
  "  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n" +
  "  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n" +
  "  SOFTWARE."

dirs = ['.', 'Tests', 'Examples']
extensions = ['.H', '.C', '.h', '.cpp', '.cc', '.cxx']

would_change = []
no_match = []
errors = []

dirs.each do |dir|
  dir_path = File.join(Dir.pwd, dir)
  next unless Dir.exist?(dir_path)
  
  Dir.glob(File.join(dir_path, '**', '*')).each do |file|
    next unless File.file?(file)
    next unless extensions.include?(File.extname(file))
    
    begin
      content = File.read(file)
      
      if content =~ GPL_PATTERN
        would_change << file
        puts "CAMBIARÍA: #{file}"
      else
        no_match << file
      end
      
    rescue => e
      errors << "Error en #{file}: #{e.message}"
      puts "ERROR: #{file} - #{e.message}"
    end
  end
end

puts "\n" + "="*80
puts "MODO PRUEBA - NO SE MODIFICÓ NINGÚN ARCHIVO"
puts "="*80
puts "Archivos que SERÍAN modificados: #{would_change.size}"
puts "Archivos sin patrón GPL: #{no_match.size}"
puts "Errores: #{errors.size}"

puts "\nPrimeros 10 archivos que SERÍAN modificados:"
would_change.first(10).each { |f| puts "  - #{f}" }

if would_change.size > 10
  puts "  ... y #{would_change.size - 10} más"
end
