/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file dispatcher_example.C
 * @brief Comprehensive example demonstrating Aleph-w's dispatcher classes.
 *
 * This example shows how to use AHDispatcher and AhHashDispatcher for
 * dynamic command dispatching based on runtime keys.
 *
 * ## What is a Dispatcher?
 *
 * A dispatcher maps keys to operations (functions, lambdas, or functors).
 * At runtime, you provide a key and the dispatcher executes the
 * corresponding operation. This pattern is useful for:
 *
 * - Command-line interfaces
 * - Event handling systems
 * - State machines
 * - Plugin architectures
 *
 * ## Examples Covered
 *
 * 1. Basic calculator with function pointers
 * 2. Text processor with lambdas
 * 3. Colombian regions menu system
 * 4. State machine for order processing
 * 5. Hash-based dispatcher for high-volume operations
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <functional>
#include <sstream>

#include <ah-dispatcher.H>
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
// Example 1: Calculator with function pointers
// ============================================================================

/**
 * @brief Basic arithmetic operations for the calculator.
 */
namespace Calculator
{
  double add(double a, double b) { return a + b; }
  double sub(double a, double b) { return a - b; }
  double mul(double a, double b) { return a * b; }
  double div(double a, double b) 
  { 
    if (b == 0) throw runtime_error("Division by zero");
    return a / b; 
  }
  double mod(double a, double b) 
  { 
    return static_cast<int>(a) % static_cast<int>(b); 
  }
  double pow(double a, double b) { return std::pow(a, b); }
}

void demo_calculator()
{
  print_header("Example 1: Calculator with Function Pointers");
  
  // Create dispatcher with character keys and function pointers
  using CalcOp = double(*)(double, double);
  AHDispatcher<char, CalcOp> calc;
  
  // Register operations
  calc.insert('+', Calculator::add);
  calc.insert('-', Calculator::sub);
  calc.insert('*', Calculator::mul);
  calc.insert('/', Calculator::div);
  calc.insert('%', Calculator::mod);
  calc.insert('^', Calculator::pow);
  
  cout << "\n  Registered operations: + - * / % ^\n" << endl;
  
  // Test cases
  struct TestCase { double a; char op; double b; };
  DynArray<TestCase> tests;
  tests.append({10, '+', 5});
  tests.append({10, '-', 3});
  tests.append({7, '*', 8});
  tests.append({100, '/', 4});
  tests.append({17, '%', 5});
  tests.append({2, '^', 10});
  
  cout << "  Expression          Result" << endl;
  cout << "  " << string(35, '-') << endl;
  
  for (size_t i = 0; i < tests.size(); ++i)
  {
    auto& t = tests(i);
    if (calc.valid_key(t.op))
    {
      double result = calc.run(t.op, t.a, t.b);
      cout << "  " << setw(6) << t.a << " " << t.op << " " 
           << setw(6) << t.b << "  =  " << setw(10) << result << endl;
    }
  }
  
  // Show that invalid keys are detected
  cout << "\n  Checking for unregistered operation '!':" << endl;
  cout << "  valid_key('!') = " << (calc.valid_key('!') ? "true" : "false") << endl;
}

// ============================================================================
// Example 2: Text processor with lambdas
// ============================================================================

void demo_text_processor()
{
  print_header("Example 2: Text Processor with Lambdas");
  
  using TextOp = function<string(const string&)>;
  AHDispatcher<string, TextOp> processor;
  
  // Register text transformations
  processor.insert("upper", [](const string& s) {
    string result = s;
    for (char& c : result) c = toupper(c);
    return result;
  });
  
  processor.insert("lower", [](const string& s) {
    string result = s;
    for (char& c : result) c = tolower(c);
    return result;
  });
  
  processor.insert("reverse", [](const string& s) {
    return string(s.rbegin(), s.rend());
  });
  
  processor.insert("length", [](const string& s) {
    return to_string(s.length()) + " characters";
  });
  
  processor.insert("words", [](const string& s) {
    int count = 0;
    bool in_word = false;
    for (char c : s) {
      if (isspace(c)) in_word = false;
      else if (!in_word) { count++; in_word = true; }
    }
    return to_string(count) + " words";
  });
  
  processor.insert("vowels", [](const string& s) {
    int count = 0;
    for (char c : s) {
      char lower = tolower(c);
      if (lower == 'a' || lower == 'e' || lower == 'i' || 
          lower == 'o' || lower == 'u')
        count++;
    }
    return to_string(count) + " vowels";
  });
  
  // Test text - Colombian theme
  string text = "Colombia es un pais de gente trabajadora";
  
  cout << "\n  Original text: \"" << text << "\"\n" << endl;
  
  cout << "  Transformation      Result" << endl;
  cout << "  " << string(55, '-') << endl;
  
  DynArray<string> operations;
  operations.append("upper");
  operations.append("lower");
  operations.append("reverse");
  operations.append("length");
  operations.append("words");
  operations.append("vowels");
  
  for (size_t i = 0; i < operations.size(); ++i)
  {
    const string& op = operations(i);
    string result = processor.run(op, text);
    cout << "  " << left << setw(18) << op << result << endl;
  }
  
  // Show available keys
  cout << "\n  Available operations: ";
  auto keys = processor.keys();
  for (auto it = keys.get_it(); it.has_curr(); it.next_ne())
    cout << it.get_curr() << " ";
  cout << endl;
}

// ============================================================================
// Example 3: Colombian Regions Information System
// ============================================================================

void demo_regions_menu()
{
  print_header("Example 3: Colombian Regions Information System");
  
  using RegionInfo = function<void()>;
  AHDispatcher<string, RegionInfo> regions;
  
  regions.insert("andina", []() {
    cout << "\n  REGION ANDINA" << endl;
    cout << "  Capital: Bogota" << endl;
    cout << "  Departments: Cundinamarca, Boyaca, Santander, Antioquia..." << endl;
    cout << "  Climate: Temperate to cold (varies with altitude)" << endl;
    cout << "  Products: Coffee, flowers, potatoes, emeralds" << endl;
  });
  
  regions.insert("caribe", []() {
    cout << "\n  REGION CARIBE" << endl;
    cout << "  Major cities: Barranquilla, Cartagena, Santa Marta" << endl;
    cout << "  Departments: Atlantico, Bolivar, Magdalena, La Guajira..." << endl;
    cout << "  Climate: Tropical hot" << endl;
    cout << "  Products: Bananas, coal, tourism, fishing" << endl;
  });
  
  regions.insert("pacifica", []() {
    cout << "\n  REGION PACIFICA" << endl;
    cout << "  Major cities: Cali, Buenaventura, Quibdo" << endl;
    cout << "  Departments: Valle del Cauca, Choco, Narino, Cauca" << endl;
    cout << "  Climate: Very humid tropical" << endl;
    cout << "  Products: Sugar cane, timber, gold, platinum" << endl;
  });
  
  regions.insert("orinoquia", []() {
    cout << "\n  REGION ORINOQUIA (Los Llanos)" << endl;
    cout << "  Major cities: Villavicencio, Yopal" << endl;
    cout << "  Departments: Meta, Casanare, Arauca, Vichada" << endl;
    cout << "  Climate: Tropical with dry season" << endl;
    cout << "  Products: Cattle, oil, rice, palm oil" << endl;
  });
  
  regions.insert("amazonia", []() {
    cout << "\n  REGION AMAZONIA" << endl;
    cout << "  Major cities: Leticia, Florencia" << endl;
    cout << "  Departments: Amazonas, Caqueta, Putumayo, Guaviare" << endl;
    cout << "  Climate: Humid equatorial" << endl;
    cout << "  Products: Timber, rubber, ecotourism, biodiversity" << endl;
  });
  
  regions.insert("insular", []() {
    cout << "\n  REGION INSULAR" << endl;
    cout << "  Islands: San Andres, Providencia, Santa Catalina" << endl;
    cout << "  Location: Caribbean Sea" << endl;
    cout << "  Climate: Tropical maritime" << endl;
    cout << "  Products: Tourism, coconut, fishing" << endl;
  });
  
  // Display all regions
  DynArray<string> region_codes;
  region_codes.append("andina");
  region_codes.append("caribe");
  region_codes.append("pacifica");
  region_codes.append("orinoquia");
  region_codes.append("amazonia");
  region_codes.append("insular");
  
  cout << "\n  Colombia's Natural Regions:" << endl;
  cout << "  " << string(40, '=') << endl;
  
  for (size_t i = 0; i < region_codes.size(); ++i)
    regions.run(region_codes(i));
}

// ============================================================================
// Example 4: Order State Machine
// ============================================================================

void demo_state_machine()
{
  print_header("Example 4: Order Processing State Machine");
  
  // Order state
  struct Order {
    int id;
    string state;
    string customer;
    double amount;
    string history;
    
    void log(const string& action) {
      history += "  -> " + action + " (from " + state + ")\n";
    }
  };
  
  Order order = {1001, "created", "Juan Perez", 250000.0, ""};
  
  using StateAction = function<void(Order&)>;
  AHDispatcher<string, StateAction> state_machine;
  
  // Define state transitions
  state_machine.insert("confirm", [](Order& o) {
    if (o.state != "created") {
      cout << "  [ERROR] Cannot confirm - order not in 'created' state" << endl;
      return;
    }
    o.log("confirmed");
    o.state = "confirmed";
    cout << "  [OK] Order " << o.id << " confirmed" << endl;
  });
  
  state_machine.insert("pay", [](Order& o) {
    if (o.state != "confirmed") {
      cout << "  [ERROR] Cannot pay - order not confirmed" << endl;
      return;
    }
    o.log("paid");
    o.state = "paid";
    cout << "  [OK] Payment of $" << fixed << setprecision(0) << o.amount 
         << " COP received" << endl;
  });
  
  state_machine.insert("ship", [](Order& o) {
    if (o.state != "paid") {
      cout << "  [ERROR] Cannot ship - order not paid" << endl;
      return;
    }
    o.log("shipped");
    o.state = "shipped";
    cout << "  [OK] Order shipped to " << o.customer << endl;
  });
  
  state_machine.insert("deliver", [](Order& o) {
    if (o.state != "shipped") {
      cout << "  [ERROR] Cannot deliver - order not shipped" << endl;
      return;
    }
    o.log("delivered");
    o.state = "delivered";
    cout << "  [OK] Order delivered successfully!" << endl;
  });
  
  state_machine.insert("cancel", [](Order& o) {
    if (o.state == "delivered") {
      cout << "  [ERROR] Cannot cancel delivered order" << endl;
      return;
    }
    o.log("cancelled");
    o.state = "cancelled";
    cout << "  [OK] Order cancelled" << endl;
  });
  
  // Process the order through its lifecycle
  cout << "\n  Order #" << order.id << " - Customer: " << order.customer << endl;
  cout << "  Amount: $" << fixed << setprecision(0) << order.amount << " COP" << endl;
  cout << "\n  Processing order:" << endl;
  cout << "  " << string(40, '-') << endl;
  
  // Normal flow
  state_machine.run("confirm", order);
  state_machine.run("pay", order);
  state_machine.run("ship", order);
  state_machine.run("deliver", order);
  
  // Try invalid transition
  cout << "\n  Attempting invalid transition:" << endl;
  state_machine.run("cancel", order);  // Should fail
  
  // Show history
  cout << "\n  Order History:" << endl;
  cout << order.history;
  cout << "  Final state: " << order.state << endl;
}

// ============================================================================
// Example 5: High-performance Hash Dispatcher
// ============================================================================

void demo_hash_dispatcher()
{
  print_header("Example 5: Hash-based Dispatcher Performance");
  
  // For very frequent lookups, AhHashDispatcher provides O(1) access
  using MetricHandler = function<double(double)>;
  AhHashDispatcher<string, MetricHandler> metrics;
  
  // Colombian economic indicators transformations
  metrics.insert("usd_to_cop", [](double usd) {
    return usd * 4150.0;  // Approximate rate
  });
  
  metrics.insert("cop_to_usd", [](double cop) {
    return cop / 4150.0;
  });
  
  metrics.insert("inflation_adjust", [](double value) {
    return value * 1.12;  // 12% annual inflation adjustment
  });
  
  metrics.insert("vat", [](double price) {
    return price * 1.19;  // 19% VAT
  });
  
  metrics.insert("income_tax", [](double income) {
    // Simplified Colombian income tax
    if (income < 4500000) return 0.0;
    if (income < 10000000) return (income - 4500000) * 0.19;
    if (income < 25000000) return (income - 10000000) * 0.28 + 1045000;
    return (income - 25000000) * 0.33 + 5245000;
  });
  
  cout << "\n  Colombian Economic Calculations:" << endl;
  cout << "  " << string(50, '-') << endl;
  
  // Currency conversions
  double usd_amount = 100.0;
  cout << "\n  Currency Conversion:" << endl;
  cout << "  $" << usd_amount << " USD = $" << fixed << setprecision(0)
       << metrics.run("usd_to_cop", usd_amount) << " COP" << endl;
  
  double cop_amount = 1000000.0;
  cout << "  $" << cop_amount << " COP = $" << setprecision(2)
       << metrics.run("cop_to_usd", cop_amount) << " USD" << endl;
  
  // Price calculations
  cout << "\n  Price Calculations:" << endl;
  double base_price = 500000.0;
  cout << "  Base price: $" << setprecision(0) << base_price << " COP" << endl;
  cout << "  With VAT (19%): $" << metrics.run("vat", base_price) << " COP" << endl;
  cout << "  Inflation adjusted: $" << metrics.run("inflation_adjust", base_price) 
       << " COP" << endl;
  
  // Income tax examples
  cout << "\n  Monthly Income Tax Examples:" << endl;
  DynArray<double> incomes;
  incomes.append(3000000);
  incomes.append(6000000);
  incomes.append(15000000);
  incomes.append(30000000);
  
  for (size_t i = 0; i < incomes.size(); ++i)
  {
    double income = incomes(i);
    double tax = metrics.run("income_tax", income);
    cout << "  Income $" << setw(12) << income << " COP -> Tax: $" 
         << setw(10) << tax << " COP" << endl;
  }
}

// ============================================================================
// Example 6: Dispatcher with Variadic Arguments
// ============================================================================

void demo_variadic_dispatcher()
{
  print_header("Example 6: Variadic Arguments Dispatcher");
  
  // Dispatcher that handles different argument counts
  using FlexOp = function<string(const DynArray<string>&)>;
  AHDispatcher<string, FlexOp> flex;
  
  flex.insert("join", [](const DynArray<string>& args) {
    string result;
    for (size_t i = 0; i < args.size(); ++i) {
      if (i > 0) result += " ";
      result += args(i);
    }
    return result;
  });
  
  flex.insert("concat", [](const DynArray<string>& args) {
    string result;
    for (size_t i = 0; i < args.size(); ++i)
      result += args(i);
    return result;
  });
  
  flex.insert("first", [](const DynArray<string>& args) {
    return args.size() > 0 ? args(0) : "";
  });
  
  flex.insert("last", [](const DynArray<string>& args) {
    return args.size() > 0 ? args(args.size() - 1) : "";
  });
  
  flex.insert("count", [](const DynArray<string>& args) {
    return to_string(args.size()) + " arguments";
  });
  
  // Test with Colombian city names
  DynArray<string> cities;
  cities.append("Bogota");
  cities.append("Medellin");
  cities.append("Cali");
  cities.append("Barranquilla");
  cities.append("Cartagena");
  
  cout << "\n  Arguments: Bogota, Medellin, Cali, Barranquilla, Cartagena\n" << endl;
  
  cout << "  Operation      Result" << endl;
  cout << "  " << string(50, '-') << endl;
  
  cout << "  join:          " << flex.run("join", cities) << endl;
  cout << "  concat:        " << flex.run("concat", cities) << endl;
  cout << "  first:         " << flex.run("first", cities) << endl;
  cout << "  last:          " << flex.run("last", cities) << endl;
  cout << "  count:         " << flex.run("count", cities) << endl;
}

// ============================================================================
// Main
// ============================================================================

int main()
{
  cout << "\n";
  cout << "========================================================================" << endl;
  cout << "        ALEPH-W DISPATCHER EXAMPLE" << endl;
  cout << "        Dynamic Command Dispatching" << endl;
  cout << "========================================================================" << endl;
  
  demo_calculator();
  demo_text_processor();
  demo_regions_menu();
  demo_state_machine();
  demo_hash_dispatcher();
  demo_variadic_dispatcher();
  
  cout << "\n";
  cout << "========================================================================" << endl;
  cout << "                    Example completed successfully!" << endl;
  cout << "========================================================================" << endl;
  cout << endl;
  
  return 0;
}

