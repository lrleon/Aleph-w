/**
 * @file string_utils_example.C
 * @brief Example demonstrating string manipulation utilities in Aleph-w.
 *
 * This program demonstrates `ah-string-utils.H` which provides:
 * - Trimming (ltrim, rtrim, trim)
 * - Case conversion (tolower, toupper, to_name)
 * - Splitting and joining
 * - Numeric validation
 * - Text formatting (justify, align)
 * - Character filtering
 *
 * ## Usage
 *
 * ```bash
 * ./string_utils_example           # Run all demos
 * ./string_utils_example -s trim   # Only trim demo
 * ```
 *
 * @author Leandro Rabindranath León
 * @ingroup Ejemplos
 * @date 2024
 * @copyright GNU General Public License
 *
 * @see ah-string-utils.H String manipulation utilities
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <tclap/CmdLine.h>

#include <htlist.H>
#include <ah-string-utils.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Helper functions
// =============================================================================

void print_section(const string& title)
{
  cout << "\n" << string(60, '=') << "\n";
  cout << "  " << title << "\n";
  cout << string(60, '=') << "\n\n";
}

void print_subsection(const string& title)
{
  cout << "\n--- " << title << " ---\n";
}

void show_string(const string& label, const string& s)
{
  cout << label << ": \"" << s << "\"" << endl;
}

// =============================================================================
// 1. Trimming
// =============================================================================

void demo_trimming()
{
  print_section("TRIMMING");
  
  string s1 = "   hello world   ";
  string s2 = "\t\n  whitespace everywhere  \r\n";
  
  show_string("Original s1", s1);
  show_string("Original s2", s2);
  
  // ltrim - left trim
  print_subsection("ltrim() - remove leading whitespace");
  string left = s1;
  ltrim(left);
  show_string("After ltrim", left);
  
  // rtrim - right trim
  print_subsection("rtrim() - remove trailing whitespace");
  string right = s1;
  rtrim(right);
  show_string("After rtrim", right);
  
  // trim - both sides (returns new string)
  print_subsection("trim() - both sides");
  show_string("trim(s1)", trim(s1));
  show_string("trim(s2)", trim(s2));
}

// =============================================================================
// 2. Case Conversion
// =============================================================================

void demo_case_conversion()
{
  print_section("CASE CONVERSION");
  
  string text = "Hola Mundo desde COLOMBIA";
  show_string("Original", text);
  
  // tolower
  print_subsection("tolower()");
  show_string("tolower", tolower(text));
  
  // toupper  
  print_subsection("toupper()");
  show_string("toupper", toupper(text));
  
  // to_name - capitalize first letter
  print_subsection("to_name() - capitalize first letter");
  show_string("to_name(\"colombia\")", to_name("colombia"));
  show_string("to_name(\"BOGOTA\")", to_name("BOGOTA"));
  
  // to_Pascalcase
  print_subsection("to_Pascalcase()");
  show_string("to_Pascalcase(\"hello world\")", to_Pascalcase("hello world"));
  show_string("to_Pascalcase(\"user_first_name\")", to_Pascalcase("user_first_name"));
}

// =============================================================================
// 3. Splitting
// =============================================================================

void demo_splitting()
{
  print_section("SPLITTING");
  
  string csv = "Bogota,Medellin,Cali,Barranquilla,Cartagena";
  show_string("CSV string", csv);
  
  // split
  print_subsection("split()");
  auto cities = split(csv, ',');
  cout << "Split by ',':" << endl;
  for (size_t i = 0; i < cities.size(); i++)
    cout << "  [" << i << "]: " << cities[i] << endl;
  
  // split with different delimiter
  print_subsection("Split with different delimiters");
  string path = "/home/user/documents/file.txt";
  auto parts = split(path, '/');
  cout << "Path: \"" << path << "\"" << endl;
  cout << "Split by '/':" << endl;
  for (const auto& p : parts)
    cout << "  \"" << p << "\"" << endl;
  
  // split_pos (split at position)
  print_subsection("split_pos() - split at position");
  string text = "Hello World";
  auto [left, right] = split_pos(text, 5);
  cout << "Text: \"" << text << "\"" << endl;
  cout << "Split at position 5:" << endl;
  cout << "  Left:  \"" << left << "\"" << endl;
  cout << "  Right: \"" << right << "\"" << endl;
}

// =============================================================================
// 4. Joining
// =============================================================================

void demo_joining()
{
  print_section("JOINING");
  
  DynList<string> words = {"Cafe", "colombiano", "es", "el", "mejor"};
  cout << "Words: ";
  for (auto it = words.get_it(); it.has_curr(); it.next())
    cout << "\"" << it.get_curr() << "\" ";
  cout << endl;
  
  // join with space
  print_subsection("join()");
  show_string("join(words, \" \")", join(words, " "));
  show_string("join(words, \"-\")", join(words, "-"));
  show_string("join(words, \", \")", join(words, ", "));
  
  // concat - variadic concatenation
  print_subsection("concat() - variadic");
  string result = concat("Nombre: ", "Juan", ", Edad: ", 25, ", Ciudad: ", "Bogota");
  show_string("concat result", result);
}

// =============================================================================
// 5. String Validation
// =============================================================================

void demo_validation()
{
  print_section("STRING VALIDATION");
  
  // is_double
  print_subsection("is_double()");
  vector<string> test_doubles = {"3.14", "-2.5", "1e10", "abc", "12.3.4"};
  for (const auto& s : test_doubles)
    cout << "  is_double(\"" << s << "\"): " << (is_double(s) ? "yes" : "no") << endl;
  
  // is_long
  print_subsection("is_long()");
  vector<string> test_longs = {"42", "-100", "9999999", "3.14", "12abc"};
  for (const auto& s : test_longs)
    cout << "  is_long(\"" << s << "\"): " << (is_long(s) ? "yes" : "no") << endl;
  
  // is_size_t
  print_subsection("is_size_t()");
  vector<string> test_sizes = {"100", "0", "-5", "abc"};
  for (const auto& s : test_sizes)
    cout << "  is_size_t(\"" << s << "\"): " << (is_size_t(s) ? "yes" : "no") << endl;
  
  // contains
  print_subsection("contains()");
  string text = "Hello from Colombia";
  cout << "Text: \"" << text << "\"" << endl;
  cout << "  contains(\"Colombia\"): " << (contains(text, "Colombia") ? "yes" : "no") << endl;
  cout << "  contains(\"Venezuela\"): " << (contains(text, "Venezuela") ? "yes" : "no") << endl;
  
  // is_prefix
  print_subsection("is_prefix()");
  cout << "  is_prefix(\"Hello world\", \"Hello\"): " 
       << (is_prefix("Hello world", "Hello") ? "yes" : "no") << endl;
  cout << "  is_prefix(\"Hello world\", \"World\"): " 
       << (is_prefix("Hello world", "World") ? "yes" : "no") << endl;
}

// =============================================================================
// 6. Character Filtering
// =============================================================================

void demo_filtering()
{
  print_section("CHARACTER FILTERING");
  
  string dirty = "  Hello123 World!@#  ";
  show_string("Original", dirty);
  
  // only_alpha
  print_subsection("only_alpha() - keep only letters");
  show_string("only_alpha", only_alpha(dirty));
  
  // remove_spaces
  print_subsection("remove_spaces()");
  show_string("remove_spaces", remove_spaces(dirty));
  
  // remove_symbols
  print_subsection("remove_symbols()");
  string with_symbols = "user@email.com";
  show_string("Original", with_symbols);
  show_string("remove_symbols('@.')", remove_symbols(with_symbols, "@."));
  
  // fill_string
  print_subsection("fill_string()");
  string to_fill = "***secret***";
  show_string("Before", to_fill);
  fill_string(to_fill, 'X');
  show_string("After fill_string('X')", to_fill);
}

// =============================================================================
// 7. Text Formatting
// =============================================================================

void demo_formatting()
{
  print_section("TEXT FORMATTING");
  
  string lorem = "Este es un texto de ejemplo que sera formateado de diferentes maneras para demostrar las capacidades de formateo de texto de Aleph-w.";
  
  cout << "Original text:" << endl;
  cout << "  " << lorem << endl;
  
  // justify_text
  print_subsection("justify_text() - width=40");
  string justified = justify_text(lorem, 40);
  cout << "Justified:" << endl;
  auto lines = split(justified, '\n');
  for (const auto& line : lines)
    cout << "  |" << line << "|" << endl;
  
  // align_text_to_left
  print_subsection("align_text_to_left() - width=35");
  string left_aligned = align_text_to_left(lorem, 35);
  lines = split(left_aligned, '\n');
  cout << "Left aligned:" << endl;
  for (const auto& line : lines)
    cout << "  |" << line << "|" << endl;
}

// =============================================================================
// 8. Conversion Utilities
// =============================================================================

void demo_conversion()
{
  print_section("CONVERSION UTILITIES");
  
  // to_string for vectors
  print_subsection("to_string(vector)");
  vector<int> nums = {1, 2, 3, 4, 5};
  cout << "vector<int>: " << Aleph::to_string(nums) << endl;
  
  vector<double> prices = {19.99, 29.99, 39.99};
  cout << "vector<double>: " << Aleph::to_string(prices) << endl;
  
  // to_string with precision
  print_subsection("to_string(double, precision)");
  double pi = 3.14159265358979;
  cout << "pi (2 decimals): " << Aleph::to_string(pi, 2) << endl;
  cout << "pi (6 decimals): " << Aleph::to_string(pi, 6) << endl;
  
  // build_pars_list
  print_subsection("build_pars_list() - parameter list");
  string params = build_pars_list("name", 42, 3.14, "test");
  show_string("Parameters", params);
}

// =============================================================================
// 9. Practical Example
// =============================================================================

void demo_practical()
{
  print_section("PRACTICAL: CSV Parser");
  
  string csv_data = R"(
    Nombre, Ciudad, Edad, Salario
    Juan Perez, Bogota, 35, 5000000
    Maria Garcia, Medellin, 28, 4500000
    Carlos Lopez, Cali, 42, 6000000
  )";
  
  cout << "Raw CSV data:" << endl;
  cout << csv_data << endl;
  
  // Parse CSV
  print_subsection("Parsing CSV");
  auto lines = split(trim(csv_data), '\n');
  
  bool is_header = true;
  vector<string> headers;
  
  for (const auto& raw_line : lines)
  {
    string line = trim(raw_line);
    if (line.empty()) continue;
    
    auto fields = split(line, ',');
    
    if (is_header)
    {
      cout << "Headers:" << endl;
      for (const auto& f : fields)
      {
        string h = trim(f);
        headers.push_back(h);
        cout << "  - " << h << endl;
      }
      is_header = false;
      cout << "\nRecords:" << endl;
    }
    else
    {
      cout << "  Record:" << endl;
      for (size_t i = 0; i < fields.size() and i < headers.size(); i++)
      {
        string value = trim(fields[i]);
        cout << "    " << headers[i] << ": " << value << endl;
      }
    }
  }
  
  // Validate salaries
  print_subsection("Validate numeric fields");
  string salary = "5000000";
  cout << "Salary \"" << salary << "\" is valid number? " 
       << (is_size_t(salary) ? "yes" : "no") << endl;
  
  string invalid = "abc123";
  cout << "Salary \"" << invalid << "\" is valid number? " 
       << (is_size_t(invalid) ? "yes" : "no") << endl;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "String utilities example for Aleph-w.\n"
      "Demonstrates trim, split, join, case conversion, and more.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<string> sectionArg(
      "s", "section",
      "Run only specific section: trim, case, split, join, validate, "
      "filter, format, convert, practical, or 'all'",
      false, "all", "section", cmd
    );
    
    cmd.parse(argc, argv);
    
    string section = sectionArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "        ALEPH-W STRING UTILITIES EXAMPLE\n";
    cout << "============================================================\n";
    
    if (section == "all" or section == "trim")
      demo_trimming();
    
    if (section == "all" or section == "case")
      demo_case_conversion();
    
    if (section == "all" or section == "split")
      demo_splitting();
    
    if (section == "all" or section == "join")
      demo_joining();
    
    if (section == "all" or section == "validate")
      demo_validation();
    
    if (section == "all" or section == "filter")
      demo_filtering();
    
    if (section == "all" or section == "format")
      demo_formatting();
    
    if (section == "all" or section == "convert")
      demo_conversion();
    
    if (section == "all" or section == "practical")
      demo_practical();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "String utilities demo completed!\n";
    cout << string(60, '=') << "\n\n";
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for argument " << e.argId() << endl;
    return 1;
  }
  catch (exception& e)
  {
    cerr << "Error: " << e.what() << endl;
    return 1;
  }
}

