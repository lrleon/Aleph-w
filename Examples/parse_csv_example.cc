/**
 * @file parse_csv_example.cc
 * @brief Educational examples for CSV parsing and manipulation
 * 
 * WHAT IS CSV?
 * ============
 * Comma-Separated Values: Simple text format for tabular data
 * Each line = one row, fields separated by commas
 * Standard format for data exchange (Excel, databases, etc.)
 * 
 * WHY USE ALEPH-W CSV PARSER?
 * ============================
 * - RFC 4180 compliant (handles quoted fields, escaping)
 * - Header-based access: row["column_name"]
 * - Type conversion: csv_to_number<int>(field)
 * - Filtering, sorting, joining operations
 * - Memory-efficient streaming for large files
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o parse_csv_example parse_csv_example.cc
 * ./parse_csv_example
 */

#include <iostream>
#include <sstream>
#include <parse-csv.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== CSV Parsing: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: Basic CSV Reading
  // =========================================================================
  // CONCEPT: Parse simple CSV data into rows and fields
  {
    cout << "--- Example 1: Basic CSV Parsing ---\n\n";
    
    // Sample CSV data (typically from file)
    stringstream csv_data(
      "name,age,city\n"
      "Alice,30,NYC\n"
      "Bob,25,LA\n"
      "Charlie,35,Chicago\n"
    );
    
    cout << "CSV Data:\n";
    cout << "name,age,city\n";
    cout << "Alice,30,NYC\n";
    cout << "Bob,25,LA\n";
    cout << "Charlie,35,Chicago\n\n";
    
    // STEP 1: Create CSV reader
    CsvReader reader(csv_data);
    
    // STEP 2: Read header row
    Array<string> header = reader.read_header();
    cout << "Header columns: " << header.size() << "\n";
    
    // STEP 3: Read data rows
    cout << "\nParsing rows:\n";
    int row_num = 1;
    while (reader.has_next())
    {
      CsvRow row = reader.next_row();
      
      // Access by column name
      cout << "  Row " << row_num++ << ": "
           << row["name"] << ", age " << row["age"]
           << ", from " << row["city"] << "\n";
    }
    
    cout << "\nKEY FEATURE: Access fields by name, not index!\n";
    cout << "             More readable and maintainable code\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: Type Conversion
  // =========================================================================
  // CONCEPT: CSV fields are strings, but we often need numbers
  {
    cout << "--- Example 2: Type Conversion ---\n\n";
    
    stringstream csv_data(
      "product,price,quantity\n"
      "Widget,19.99,100\n"
      "Gadget,29.99,50\n"
      "Doohickey,9.99,200\n"
    );
    
    CsvReader reader(csv_data);
    reader.read_header();
    
    cout << "Calculating total inventory value...\n\n";
    
    double total_value = 0.0;
    while (reader.has_next())
    {
      CsvRow row = reader.next_row();
      
      // Convert string fields to numbers
      double price = row.get<double>("price");
      int quantity = row.get<int>("quantity");
      double item_value = price * quantity;
      
      total_value += item_value;
      
      cout << "  " << row["product"] 
           << ": $" << price << " x " << quantity
           << " = $" << item_value << "\n";
    }
    
    cout << "\nTotal inventory value: $" << total_value << "\n";
    cout << "\nTYPE SAFETY: get<T>() throws exception if conversion fails\n";
    cout << "             Catches data errors early\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 3: Handling Quoted Fields
  // =========================================================================
  // CONCEPT: CSV fields with commas/quotes must be escaped
  {
    cout << "--- Example 3: Quoted Fields (RFC 4180) ---\n\n";
    
    // CSV with quoted fields containing commas and quotes
    stringstream csv_data(
      "name,description,price\n"
      "\"Smith, John\",\"Consultant, Senior\",150\n"
      "\"O'Brien, Mary\",\"Director, \"\"Special\"\" Projects\",200\n"
    );
    
    cout << "CSV with special characters:\n";
    cout << "name,description,price\n";
    cout << "\"Smith, John\",\"Consultant, Senior\",150\n";
    cout << "\"O'Brien, Mary\",\"Director, \"\"Special\"\" Projects\",200\n\n";
    
    CsvReader reader(csv_data);
    reader.read_header();
    
    cout << "Parsed correctly:\n";
    while (reader.has_next())
    {
      CsvRow row = reader.next_row();
      cout << "  Name: " << row["name"] << "\n";
      cout << "  Role: " << row["description"] << "\n";
      cout << "  Rate: $" << row["price"] << "/hr\n\n";
    }
    
    cout << "RFC 4180 RULES:\n";
    cout << "  1. Fields with commas → enclosed in quotes\n";
    cout << "  2. Quotes inside field → doubled \"\"\n";
    cout << "  3. Parser handles this automatically!\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Filtering Data
  // =========================================================================
  // CONCEPT: Select subset of rows based on criteria
  {
    cout << "--- Example 4: Filtering Data ---\n\n";
    
    stringstream csv_data(
      "employee,department,salary\n"
      "Alice,Engineering,80000\n"
      "Bob,Marketing,60000\n"
      "Charlie,Engineering,90000\n"
      "Diana,Sales,70000\n"
      "Eve,Engineering,85000\n"
    );
    
    // Read all data
    auto all_rows = csv_read_all(csv_data);
    
    cout << "Total employees: " << (all_rows.size() - 1) << "\n\n";
    
    // Filter: Only Engineering department
    cout << "Engineering employees:\n";
    auto eng_filter = [](const Array<string>& row) {
      return row.size() > 1 && row[1] == "Engineering";
    };
    
    auto eng_rows = csv_filter(all_rows, eng_filter);
    
    // Skip header, show results
    for (size_t i = 1; i < eng_rows.size(); ++i)
    {
      cout << "  " << eng_rows[i][0] 
           << " - $" << eng_rows[i][2] << "\n";
    }
    
    cout << "\nPOWERFUL FEATURE: Lambda-based filtering\n";
    cout << "                  Can combine multiple conditions\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 5: Data Analysis
  // =========================================================================
  // CONCEPT: Compute statistics from CSV data
  {
    cout << "--- Example 5: Data Analysis ---\n\n";
    
    stringstream csv_data(
      "month,revenue,expenses\n"
      "Jan,50000,30000\n"
      "Feb,55000,32000\n"
      "Mar,48000,31000\n"
      "Apr,62000,35000\n"
    );
    
    CsvReader reader(csv_data);
    reader.read_header();
    
    double total_revenue = 0.0;
    double total_expenses = 0.0;
    int months = 0;
    
    cout << "Monthly P&L:\n";
    cout << "Month | Revenue | Expenses | Profit\n";
    cout << "------|---------|----------|--------\n";
    
    while (reader.has_next())
    {
      CsvRow row = reader.next_row();
      
      double revenue = row.get<double>("revenue");
      double expenses = row.get<double>("expenses");
      double profit = revenue - expenses;
      
      total_revenue += revenue;
      total_expenses += expenses;
      months++;
      
      cout << row["month"] << "   | $" << revenue 
           << " | $" << expenses << " | $" << profit << "\n";
    }
    
    cout << "\nSUMMARY:\n";
    cout << "  Total Revenue: $" << total_revenue << "\n";
    cout << "  Total Expenses: $" << total_expenses << "\n";
    cout << "  Net Profit: $" << (total_revenue - total_expenses) << "\n";
    cout << "  Avg Monthly Revenue: $" << (total_revenue / months) << "\n\n";
    
    cout << "REAL-WORLD: Financial reporting, data analytics dashboards\n\n";
  }
  
  cout << "=== SUMMARY: CSV Best Practices ===\n";
  cout << "\n1. ALWAYS READ HEADERS:\n";
  cout << "   Use reader.read_header() before processing rows\n";
  cout << "   Access by name: row[\"column\"] not row[index]\n";
  cout << "\n2. TYPE CONVERSION:\n";
  cout << "   Use row.get<T>() for type-safe conversion\n";
  cout << "   Catches invalid data early with exceptions\n";
  cout << "\n3. HANDLE SPECIAL CHARACTERS:\n";
  cout << "   Parser automatically handles RFC 4180:\n";
  cout << "   - Quoted fields with commas\n";
  cout << "   - Escaped quotes (\"\")\n";
  cout << "   - Line breaks in fields\n";
  cout << "\n4. MEMORY EFFICIENCY:\n";
  cout << "   Use CsvReader for streaming (large files)\n";
  cout << "   Use csv_read_all() for small datasets\n";
  cout << "\n5. COMMON OPERATIONS:\n";
  cout << "   - Filter: csv_filter() with lambda\n";
  cout << "   - Sort: csv_sort_by_column_numeric()\n";
  cout << "   - Join: csv_inner_join() on key column\n";
  cout << "   - Group: csv_group_by() for aggregation\n";
  cout << "\n6. ERROR HANDLING:\n";
  cout << "   - Wrap file reads in try-catch\n";
  cout << "   - Check row.size() before access\n";
  cout << "   - Validate data types with get<T>()\n";
  
  return 0;
}
