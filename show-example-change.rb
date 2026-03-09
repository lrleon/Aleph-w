#!/usr/bin/env ruby

# Muestra cómo quedaría UN archivo después del cambio

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

file = ARGV[0] || 'ah-arena.H'

unless File.exist?(file)
  puts "Archivo no existe: #{file}"
  exit 1
end

content = File.read(file)
new_content = content.gsub(GPL_PATTERN, MIT_TEXT)

puts "="*80
puts "ARCHIVO: #{file}"
puts "="*80
puts "\nPRIMERAS 35 LÍNEAS DEL RESULTADO:"
puts "="*80
puts new_content.lines.first(35).join
puts "="*80
