
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/**
 * @file mapping_example.C
 * @brief Comprehensive example demonstrating Aleph-w's AHMapping class
 *
 * This example demonstrates `AHMapping`, a bidirectional key-value mapping
 * container that provides efficient lookups in both directions. Unlike standard
 * maps that only support forward lookup (key → value), AHMapping enables
 * efficient reverse lookup (value → key) as well.
 *
 * ## What is AHMapping?
 *
 * `AHMapping` is a specialized container for bidirectional mappings:
 * - **Forward lookup**: O(log n) - find value given key
 * - **Reverse lookup**: O(log n) - find key given value (via inverse mapping)
 * - **Dual structure**: Maintains both key→value and value→key mappings
 * - **Functional operations**: Supports `for_each` and other functional ops
 *
 * ## Key Features
 *
 * ### Bidirectional Lookup
 * - **Forward**: `mapping[key]` → returns value
 * - **Reverse**: `mapping.inverse()[value]` → returns key
 * - Both operations are O(log n) efficient
 *
 * ### Data Structure
 * - Uses balanced BST internally (typically Red-Black tree)
 * - Maintains two mappings: forward and inverse
 * - Automatic synchronization between mappings
 *
 * ### Operations
 * - **Insert**: Add key-value pair (updates both mappings)
 * - **Lookup**: Find value by key or key by value
 * - **Iteration**: Iterate over all pairs
 * - **Functional**: Apply functions to all pairs
 *
 * ## Use Cases
 *
 * ### Translation Tables
 * - **Language codes ↔ names**: "en" ↔ "English", "es" ↔ "Spanish"
 * - **Currency codes ↔ symbols**: "USD" ↔ "$", "EUR" ↔ "€"
 * - **Country codes ↔ names**: "CO" ↔ "Colombia", "US" ↔ "United States"
 *
 * ### ID Mappings
 * - **User ID ↔ username**: 12345 ↔ "john_doe"
 * - **Product ID ↔ name**: 789 ↔ "Widget Pro"
 * - **Session ID ↔ user**: "abc123" ↔ user_object
 *
 * ### Configuration Parameters
 * - **Setting name ↔ value**: "theme" ↔ "dark", "language" ↔ "es"
 * - **Environment variables**: "PATH" ↔ "/usr/bin:/usr/local/bin"
 * - **Feature flags**: "new_ui" ↔ true, "beta_features" ↔ false
 *
 * ### Encoding/Decoding Schemes
 * - **Character encoding**: ASCII codes ↔ characters
 * - **Base conversion**: Decimal ↔ hexadecimal
 * - **Protocol mapping**: Internal codes ↔ external codes
 *
 * ### Colombian Theme
 *
 * Examples use Colombian data for cultural relevance:
 * - **Departments**: "ANT" ↔ "Antioquia", "CUN" ↔ "Cundinamarca"
 * - **Cities**: "BOG" ↔ "Bogotá", "MED" ↔ "Medellín"
 * - **Cultural elements**: Colombian regions, landmarks, traditions
 *
 * ## Comparison with Standard Maps
 *
 * | Feature | std::map | AHMapping |
 * |---------|----------|-----------|
 * | Forward lookup | O(log n) | O(log n) |
 * | Reverse lookup | O(n) | O(log n) |
 * | Memory | O(n) | O(n) |
 * | Bidirectional | No | Yes |
 *
 * **Advantage**: AHMapping provides efficient reverse lookup without
 * maintaining a separate reverse map manually.
 *
 * ## Complexity
 *
 * | Operation | Complexity | Notes |
 * |-----------|-----------|-------|
 * | Insert | O(log n) | Updates both mappings |
 * | Forward lookup | O(log n) | Key → value |
 * | Reverse lookup | O(log n) | Value → key |
 * | Iteration | O(n) | All pairs |
 * | Size | O(1) | Number of pairs |
 *
 * ## Usage Example
 *
 * ```cpp
 * AHMapping<string, string> translations;
 *
 * // Add translations
 * translations.insert("en", "English");
 * translations.insert("es", "Spanish");
 * translations.insert("fr", "French");
 *
 * // Forward lookup
 * string lang = translations["en"]; // "English"
 *
 * // Reverse lookup
 * string code = translations.inverse()["Spanish"]; // "es"
 * ```
 *
 * @see ah-mapping.H AHMapping implementation
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>

#include <ah-mapping.H>
#include <tpl_dynArray.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// Helper functions
// ============================================================================

void print_header(const string& title)
{
  cout << "\n";
  cout << "+" << string(70, '-') << "+" << endl;
  cout << "| " << left << setw(68) << title << " |" << endl;
  cout << "+" << string(70, '-') << "+" << endl;
}

void print_subheader(const string& subtitle)
{
  cout << "\n  " << subtitle << endl;
  cout << "  " << string(subtitle.length(), '-') << endl;
}

// ============================================================================
// Example 1: Basic Mapping - Colombian Department Codes
// ============================================================================

void demo_department_codes()
{
  print_header("Example 1: Colombian Department Codes");
  
  // Create mapping from DANE codes to department names
  AHMapping<string, string> dept_codes;
  
  // Insert Colombian departments (DANE codes)
  dept_codes.insert("05", "Antioquia");
  dept_codes.insert("08", "Atlantico");
  dept_codes.insert("11", "Bogota D.C.");
  dept_codes.insert("13", "Bolivar");
  dept_codes.insert("15", "Boyaca");
  dept_codes.insert("17", "Caldas");
  dept_codes.insert("19", "Cauca");
  dept_codes.insert("20", "Cesar");
  dept_codes.insert("23", "Cordoba");
  dept_codes.insert("25", "Cundinamarca");
  dept_codes.insert("27", "Choco");
  dept_codes.insert("41", "Huila");
  dept_codes.insert("44", "La Guajira");
  dept_codes.insert("47", "Magdalena");
  dept_codes.insert("50", "Meta");
  dept_codes.insert("52", "Narino");
  dept_codes.insert("54", "Norte de Santander");
  dept_codes.insert("63", "Quindio");
  dept_codes.insert("66", "Risaralda");
  dept_codes.insert("68", "Santander");
  dept_codes.insert("70", "Sucre");
  dept_codes.insert("73", "Tolima");
  dept_codes.insert("76", "Valle del Cauca");
  dept_codes.insert("91", "Amazonas");
  
  cout << "\n  Total departments registered: " << dept_codes.size() << endl;
  
  print_subheader("Forward lookup (code -> name)");
  
  DynArray<string> codes_to_lookup;
  codes_to_lookup.append("11");
  codes_to_lookup.append("05");
  codes_to_lookup.append("76");
  codes_to_lookup.append("91");
  
  for (size_t i = 0; i < codes_to_lookup.size(); ++i)
  {
    const string& code = codes_to_lookup(i);
    cout << "    Code " << code << " -> " << dept_codes[code] << endl;
  }
  
  print_subheader("Inverse mapping (name -> code)");
  
  // Create inverse mapping
  AHMapping<string, string> name_to_code = dept_codes.inverse();
  
  DynArray<string> names_to_lookup;
  names_to_lookup.append("Bogota D.C.");
  names_to_lookup.append("Antioquia");
  names_to_lookup.append("Valle del Cauca");
  names_to_lookup.append("Amazonas");
  
  for (size_t i = 0; i < names_to_lookup.size(); ++i)
  {
    const string& name = names_to_lookup(i);
    cout << "    " << left << setw(20) << name << " -> Code " 
         << name_to_code[name] << endl;
  }
  
  print_subheader("Check if key exists");
  
  cout << "    Has code '11'? " << (dept_codes.valid_key("11") ? "Yes" : "No") << endl;
  cout << "    Has code '99'? " << (dept_codes.valid_key("99") ? "Yes" : "No") << endl;
}

// ============================================================================
// Example 2: Variadic Constructor - Coffee Regions
// ============================================================================

void demo_variadic_constructor()
{
  print_header("Example 2: Variadic Constructor - Coffee Regions");
  
  // Create mapping using variadic constructor
  AHMapping<string, string> coffee_regions(
    "Huila", "Sur del pais, cafe con notas citricos",
    "Narino", "Alta montania, cafe suave y frutal",
    "Cauca", "Region volcanica, cafe de acidez brillante",
    "Tolima", "Clima templado, cafe balanceado",
    "Antioquia", "Tradicion cafetera, cafe con cuerpo",
    "Caldas", "Eje Cafetero, cafe clasico colombiano",
    "Quindio", "Paisaje Cultural Cafetero, cafe aromatico",
    "Risaralda", "Eje Cafetero, cafe de montania",
    "Santander", "Oriente, cafe organico y especial"
  );
  
  cout << "\n  Coffee regions: " << coffee_regions.size() << endl;
  
  print_subheader("Region profiles");
  
  // Use for_each to iterate
  coffee_regions.for_each([](const string& region, const string& profile) {
    cout << "    " << left << setw(12) << region << ": " << profile << endl;
  });
  
  print_subheader("Keys and values separately");
  
  cout << "\n  All regions: ";
  auto keys = coffee_regions.keys();
  for (auto it = keys.get_it(); it.has_curr(); it.next_ne())
    cout << it.get_curr() << " ";
  cout << endl;
}

// ============================================================================
// Example 3: Numeric Mappings - Population Data
// ============================================================================

void demo_numeric_mapping()
{
  print_header("Example 3: Numeric Mappings - City Population");
  
  // Mapping from city name to population (thousands)
  AHMapping<string, int> population;
  
  population.insert("Bogota", 8281);
  population.insert("Medellin", 2569);
  population.insert("Cali", 2228);
  population.insert("Barranquilla", 1274);
  population.insert("Cartagena", 1047);
  population.insert("Cucuta", 711);
  population.insert("Bucaramanga", 609);
  population.insert("Pereira", 488);
  population.insert("Santa Marta", 538);
  population.insert("Ibague", 580);
  
  print_subheader("Cities by population (thousands)");
  
  population.for_each([](const string& city, int pop) {
    cout << "    " << left << setw(15) << city << ": " 
         << right << setw(6) << pop << " mil habitantes" << endl;
  });
  
  print_subheader("Total population");
  
  int total = 0;
  auto values = population.values();
  for (auto it = values.get_it(); it.has_curr(); it.next_ne())
    total += it.get_curr();
  
  cout << "    Sum of registered cities: " << total << " mil habitantes" << endl;
  cout << "    (Approx. " << total / 1000.0 << " millones)" << endl;
  
  print_subheader("Contains value check");
  
  cout << "    Contains city with 2569k? " 
       << (population.contains_value(2569) ? "Yes (Medellin)" : "No") << endl;
  cout << "    Contains city with 5000k? " 
       << (population.contains_value(5000) ? "Yes" : "No") << endl;
}

// ============================================================================
// Example 4: Encoding/Decoding - Indigenous Languages
// ============================================================================

void demo_encoding_decoding()
{
  print_header("Example 4: Encoding/Decoding - Language Codes");
  
  // ISO 639-3 codes for Colombian indigenous languages
  AHMapping<string, string> lang_codes;
  
  lang_codes.insert("way", "Wayuunaiki");      // Wayuu
  lang_codes.insert("cag", "Embera");          // Chami
  lang_codes.insert("iku", "Arhuaco");         // Ika
  lang_codes.insert("snn", "Inga");            // Highland Inga
  lang_codes.insert("kwi", "Awa Pit");         // Awa-Cuaiquer
  lang_codes.insert("guc", "Guajiro");         // Wayuu variant
  lang_codes.insert("pbb", "Nasa Yuwe");       // Paez
  lang_codes.insert("mvt", "Motilon");         // Bari
  lang_codes.insert("cub", "Cubeo");           // Cubeo
  lang_codes.insert("tic", "Tikuna");          // Ticuna
  
  cout << "\n  Indigenous languages registered: " << lang_codes.size() << endl;
  
  print_subheader("Encode: Name -> Code");
  
  // Create decoder (inverse of encoder)
  AHMapping<string, string> decoder = lang_codes.inverse();
  
  DynArray<string> languages;
  languages.append("Wayuunaiki");
  languages.append("Nasa Yuwe");
  languages.append("Tikuna");
  languages.append("Embera");
  
  for (size_t i = 0; i < languages.size(); ++i)
  {
    const string& lang = languages(i);
    cout << "    " << left << setw(15) << lang << " -> code: " 
         << decoder[lang] << endl;
  }
  
  print_subheader("Decode: Code -> Name");
  
  DynArray<string> codes;
  codes.append("way");
  codes.append("pbb");
  codes.append("tic");
  codes.append("cub");
  
  for (size_t i = 0; i < codes.size(); ++i)
  {
    const string& code = codes(i);
    cout << "    Code " << code << " -> " << lang_codes[code] << endl;
  }
}

// ============================================================================
// Example 5: Modifiable Mapping - Currency Exchange
// ============================================================================

void demo_modifiable_mapping()
{
  print_header("Example 5: Modifiable Mapping - Exchange Rates");
  
  // Exchange rates (COP per unit of foreign currency)
  AHMapping<string, double> exchange;
  
  exchange["USD"] = 4150.0;
  exchange["EUR"] = 4520.0;
  exchange["GBP"] = 5280.0;
  exchange["MXN"] = 245.0;
  exchange["BRL"] = 830.0;
  exchange["ARS"] = 4.7;
  exchange["PEN"] = 1120.0;
  exchange["CLP"] = 4.5;
  
  print_subheader("Current exchange rates (COP per unit)");
  
  exchange.for_each([](const string& currency, double rate) {
    cout << "    1 " << currency << " = " << fixed << setprecision(2) 
         << rate << " COP" << endl;
  });
  
  print_subheader("Update rates");
  
  // Update some rates
  exchange["USD"] = 4200.0;  // Dollar strengthened
  exchange["EUR"] = 4480.0;  // Euro weakened
  
  cout << "    Updated USD: 1 USD = " << exchange["USD"] << " COP" << endl;
  cout << "    Updated EUR: 1 EUR = " << exchange["EUR"] << " COP" << endl;
  
  print_subheader("Remove currency");
  
  cout << "    Removing ARS..." << endl;
  exchange.remove("ARS");
  cout << "    Has ARS? " << (exchange.valid_key("ARS") ? "Yes" : "No") << endl;
  cout << "    Total currencies: " << exchange.size() << endl;
  
  print_subheader("Conversion example");
  
  double usd_amount = 100.0;
  double eur_amount = 50.0;
  
  cout << "    $" << usd_amount << " USD = $" 
       << fixed << setprecision(0) << (usd_amount * exchange["USD"]) << " COP" << endl;
  cout << "    " << eur_amount << " EUR = $" 
       << (eur_amount * exchange["EUR"]) << " COP" << endl;
}

// ============================================================================
// Example 6: Practical Application - City Coordinates
// ============================================================================

void demo_city_coordinates()
{
  print_header("Example 6: City Coordinates Lookup");
  
  // Simple coordinate structure
  struct Coord {
    double lat;
    double lon;
  };
  
  // Mapping from city to coordinates
  AHMapping<string, Coord> cities;
  
  cities.insert("Bogota", {4.711, -74.072});
  cities.insert("Medellin", {6.244, -75.574});
  cities.insert("Cali", {3.451, -76.532});
  cities.insert("Barranquilla", {10.964, -74.796});
  cities.insert("Cartagena", {10.391, -75.479});
  cities.insert("Cucuta", {7.893, -72.508});
  cities.insert("Santa Marta", {11.241, -74.199});
  cities.insert("Leticia", {-4.215, -69.940});
  cities.insert("San Andres", {12.584, -81.701});
  
  print_subheader("City coordinates (latitude, longitude)");
  
  cities.for_each([](const string& city, const Coord& c) {
    cout << "    " << left << setw(15) << city 
         << fixed << setprecision(3)
         << "(" << setw(8) << c.lat << ", " << setw(8) << c.lon << ")" << endl;
  });
  
  print_subheader("Distance approximation (Bogota to other cities)");
  
  Coord bogota = cities["Bogota"];
  
  // Simple approximation (not geodesic)
  auto approx_distance = [&](const string& city) {
    Coord other = cities[city];
    double dlat = (other.lat - bogota.lat) * 111.0;  // ~111 km per degree
    double dlon = (other.lon - bogota.lon) * 111.0 * cos(bogota.lat * M_PI / 180);
    return sqrt(dlat * dlat + dlon * dlon);
  };
  
  DynArray<string> target_cities;
  target_cities.append("Medellin");
  target_cities.append("Cali");
  target_cities.append("Barranquilla");
  target_cities.append("Leticia");
  target_cities.append("San Andres");
  
  for (size_t i = 0; i < target_cities.size(); ++i)
  {
    const string& city = target_cities(i);
    cout << "    Bogota -> " << left << setw(15) << city 
         << "~" << fixed << setprecision(0) << approx_distance(city) << " km" << endl;
  }
}

// ============================================================================
// Main
// ============================================================================

int main()
{
  cout << "\n";
  cout << "========================================================================" << endl;
  cout << "        ALEPH-W MAPPING EXAMPLE" << endl;
  cout << "        Bidirectional Key-Value Mappings" << endl;
  cout << "========================================================================" << endl;
  
  demo_department_codes();
  demo_variadic_constructor();
  demo_numeric_mapping();
  demo_encoding_decoding();
  demo_modifiable_mapping();
  demo_city_coordinates();
  
  cout << "\n";
  cout << "========================================================================" << endl;
  cout << "                    Example completed successfully!" << endl;
  cout << "========================================================================" << endl;
  cout << endl;
  
  return 0;
}

