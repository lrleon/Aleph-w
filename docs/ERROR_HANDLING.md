# Error Handling Policy for Aleph-w

This document describes the unified error handling strategy used in Aleph-w.

## Overview

Aleph-w uses the `ah-errors.H` macro system for consistent error handling across all components. This system provides:

- Automatic source location tracking (file:line)
- Formatted error messages using stream operators
- Compiler hints for optimization (`[[unlikely]]`)
- All standard C++ exception types

## The `ah_*` Macro System

### Basic Pattern

```cpp
#include <ah-errors.H>

// Conditional throw - if condition is true
ah_out_of_range_error_if(index >= size) 
    << "Index " << index << " exceeds size " << size;

// Conditional throw - if condition is false
ah_invalid_argument_unless(ptr != nullptr) 
    << "Pointer cannot be null";

// Unconditional throw
ah_domain_error() << "Invalid state detected";
```

### Available Exception Types

| Macro Prefix | Exception Type | Use Case |
|--------------|----------------|----------|
| `ah_out_of_range_error` | `std::out_of_range` | Container index out of bounds |
| `ah_invalid_argument` | `std::invalid_argument` | Invalid function parameter |
| `ah_domain_error` | `std::domain_error` | Value outside function domain |
| `ah_logic_error` | `std::logic_error` | Program logic violation (bug) |
| `ah_runtime_error` | `std::runtime_error` | Runtime error condition |
| `ah_range_error` | `std::range_error` | Value outside valid range |
| `ah_overflow_error` | `std::overflow_error` | Arithmetic overflow |
| `ah_underflow_error` | `std::underflow_error` | Arithmetic underflow |
| `ah_length_error` | `std::length_error` | Container size limit exceeded |
| `ah_bad_alloc` | `std::bad_alloc` | Memory allocation failure |
| `ah_fatal_error` | `std::runtime_error` | Unrecoverable error |

### Macro Variants

Each exception type has three variants:

```cpp
// Throws if condition is TRUE
ah_TYPE_error_if(condition) << "message";

// Throws if condition is FALSE  
ah_TYPE_error_unless(condition) << "message";

// Always throws
ah_TYPE_error() << "message";

// For compile-time conditions (constexpr)
ah_TYPE_error_if_constexpr(condition) << "message";
ah_TYPE_error_unless_constexpr(condition) << "message";
```

### Warnings (Non-Throwing)

```cpp
// Emit warning to stderr if condition is true
ah_warning_if(std::cerr, size > 1000) << "Large size may impact performance";

// Emit warning to stderr if condition is false
ah_warning_unless(std::cerr, valid) << "Invalid state detected";
```

## Guidelines

### DO ✓

```cpp
// Use macros for all exception throwing
ah_out_of_range_error_if(i >= size()) 
    << "select: position " << i << " >= size " << size();

// Include context in error messages
ah_domain_error_if(graph.is_empty()) 
    << "Prim's algorithm requires non-empty graph";

// Use _if for positive conditions (more common)
ah_invalid_argument_if(n == 0) << "n must be positive";

// Use _unless for preconditions
ah_logic_error_unless(is_initialized()) << "Object not initialized";
```

### DON'T ✗

```cpp
// Don't use throw directly
throw std::out_of_range("bad index");  // ✗ No location info

// Don't use generic messages
ah_domain_error() << "error";  // ✗ Not helpful

// Don't forget the << operator
ah_invalid_argument_if(n < 0);  // ✗ Compiles but no message
```

## Return Values vs Exceptions

### Use Exceptions For:
- Precondition violations (invalid arguments)
- Out-of-range access in containers
- Resource allocation failures
- Unrecoverable error conditions

### Use `nullptr` / Sentinel Values For:
- Search operations that may not find an element
- Insert operations that may fail due to duplicates
- Low-level tree operations

```cpp
// Search returns nullptr if not found (not an error)
Node* found = tree.search(key);
if (found == nullptr)
    // Handle not-found case - this is expected behavior

// But invalid index IS an error
ah_out_of_range_error_if(i >= size()) << "Index out of bounds";
```

## Assertions vs Exceptions

| Mechanism | Purpose | Enabled |
|-----------|---------|---------|
| `assert()` | Debug-only invariant checks | Only with DEBUG |
| `ah_*_error` | Runtime error conditions | Always |

```cpp
// Use assert for internal invariants (debug only)
assert(left->count + right->count + 1 == parent->count);

// Use exceptions for errors that can happen at runtime
ah_out_of_range_error_if(index >= size()) << "Invalid index";
```

## Migration Guide

To convert legacy code to the unified system:

```cpp
// BEFORE
if (n < 0)
    throw std::invalid_argument("n must be non-negative");

// AFTER
ah_invalid_argument_if(n < 0) << "n must be non-negative (got " << n << ")";
```

Benefits:
- Location tracking (file:line in error message)
- Consistent formatting
- Compiler optimization hints
- Type safety

## File Organization

The error handling system is defined in:
- `ah-errors.H` - All macros and the `ExceptionBuilder` class

Include it at the top of any file that needs error handling:

```cpp
#include <ah-errors.H>
```

