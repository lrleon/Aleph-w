#!/usr/bin/env ruby

# Script para reemplazar headers GPL con MIT en archivos C++

require 'fileutils'

# Texto GPL a buscar (con regex flexible para espacios)
GPL_PATTERN = /This program is free software: you can redistribute it and\/or modify\s+it under the terms of the GNU General Public License as published by\s+the Free Software Foundation, either version 3 of the License, or\s+\(at your option\) any later version\.\s+This program is distributed in the hope that it will be useful, but\s+WITHOUT ANY WARRANTY; without even the implied warranty of\s+MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE\. See the GNU\s+General Public License for more details\.\s+You should have received a copy of the GNU General Public License\s+along with this program\. If not, see <https:\/\/www\.gnu\.org\/licenses\/>\./m

# Texto MIT de reemplazo
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

# Directorios a procesar
dirs = ['.', 'Tests', 'Examples']

# Extensiones de archivos a procesar
extensions = ['.H', '.C', '.h', '.cpp', '.cc', '.cxx']

changed_files = []
unchanged_files = []
errors = []

dirs.each do |dir|
  dir_path = File.join(Dir.pwd, dir)
  next unless Dir.exist?(dir_path)
  
  Dir.glob(File.join(dir_path, '**', '*')).each do |file|
    next unless File.file?(file)
    next unless extensions.include?(File.extname(file))
    
    begin
      content = File.read(file)
      original_content = content.dup
      
      # Reemplazar el patrón GPL con MIT
      content.gsub!(GPL_PATTERN, MIT_TEXT)
      
      if content != original_content
        File.write(file, content)
        changed_files << file
        puts "✓ Modificado: #{file}"
      else
        unchanged_files << file
      end
      
    rescue => e
      errors << "Error en #{file}: #{e.message}"
      puts "✗ Error: #{file} - #{e.message}"
    end
  end
end

puts "\n" + "="*80
puts "RESUMEN"
puts "="*80
puts "Archivos modificados: #{changed_files.size}"
puts "Archivos sin cambios: #{unchanged_files.size}"
puts "Errores: #{errors.size}"

if changed_files.any?
  puts "\nArchivos modificados:"
  changed_files.each { |f| puts "  - #{f}" }
end

if errors.any?
  puts "\nErrores:"
  errors.each { |e| puts "  - #{e}" }
end
