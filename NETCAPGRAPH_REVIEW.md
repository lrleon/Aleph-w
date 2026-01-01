# Code Review: tpl_netcapgraph.H

## Executive Summary

A comprehensive review of `tpl_netcapgraph.H` revealed several critical issues that have been fixed, along with a complete rewrite of the header to modernize the code and translate all documentation to English.

## Issues Found and Fixed

### 1. **CRITICAL: Incorrect Header Include**

**Original:**
```cpp
#include <tpl_netgraph.H>  // This file does NOT exist
```

**Fixed:**
```cpp
#include <tpl_net.H>  // Correct header containing Net_Graph, Net_Node, Net_Arc
```

**Impact:** Code would not compile at all.

---

### 2. **CRITICAL: Obsolete API Usage for Net_Node**

**Original:**
```cpp
class Net_Cap_Node : public Net_Node<Node_Info, F_Type>  // 2 template params
```

**Issue:** The current `Net_Node` is defined as:
```cpp
template <typename Node_Info = Empty_Class>
using Net_Node = Graph_Anode<Node_Info>;  // Only 1 template parameter
```

**Fixed:** `Net_Cap_Node` now inherits from `Graph_Anode<Node_Info>` directly:
```cpp
class Net_Cap_Node : public Graph_Anode<Node_Info>
```

---

### 3. **CRITICAL: Undefined Filter `No_Res_Arc`**

**Original Code (line 223):**
```cpp
for (Arc_Iterator<Aux_Net, No_Res_Arc<Aux_Net>> it(*aux_net); ...)
```

**Issue:** `No_Res_Arc` is not defined anywhere in the codebase.

**Fixed:** Removed this filter usage since it's unnecessary for the update operation. The standard `Arc_Iterator` is sufficient.

---

### 4. **Missing in_flow and out_flow Fields in Nodes**

**Original Issue:** The `update()` method assigned to `p->in_flow` and `p->out_flow`, but `Net_Node` (now `Graph_Anode`) doesn't have these fields.

**Fixed:** Added `in_flow` and `out_flow` fields directly to `Net_Cap_Node`:
```cpp
Flow_Type in_flow = Flow_Type{0};
Flow_Type out_flow = Flow_Type{0};
```

---

### 5. **Documentation in Spanish**

All documentation was in Spanish. Translated to English for international consistency.

**Examples of translations:**
- "Nodo con capacidad" → "Node with capacity constraint for flow networks"
- "Red capacitada con límites de capacidad en nodos" → "Capacitated network with node capacity constraints"

---

### 6. **Missing Error Handling**

Added validation for:
- Negative capacity values (throws `std::domain_error`)
- Flow exceeding capacity when setting node capacity (throws `std::overflow_error`)
- Double computation of auxiliary network
- Operations on non-existent auxiliary network

---

### 7. **Missing const-correctness and noexcept**

Added `[[nodiscard]]`, `const`, and `noexcept` attributes where appropriate:

```cpp
[[nodiscard]] Aux_Net* get_aux_net() noexcept { return aux_net; }
[[nodiscard]] const Aux_Net* get_aux_net() const noexcept { return aux_net; }
[[nodiscard]] bool has_aux_net() const noexcept { return aux_net != nullptr; }
```

---

### 8. **Missing Move Semantics**

Added move constructor and move assignment operator:

```cpp
Net_Cap_Graph(Net_Cap_Graph&& other) noexcept;
Net_Cap_Graph& operator=(Net_Cap_Graph&& other) noexcept;
```

---

### 9. **Missing Utility Methods**

Added useful methods:
- `reset_flows()` - Reset all flow values to zero
- `check_node_capacities()` - Validate all node constraints
- `get_total_flow()` - Get total network flow
- `get_node_cap()` / `set_node_cap()` - Static accessors for node capacity

---

## Related File: tpl_net_sup_dem.H

This file has the **same incorrect include** and **more severe API incompatibilities**:

### Issues in tpl_net_sup_dem.H:

1. **Incorrect include:** `#include <tpl_netgraph.H>` → Fixed to `#include <tpl_net.H>`

2. **Obsolete API usage:**
   - Uses `Net_Node<Node_Info, F_Type>` (2 params, doesn't exist)
   - Uses `p->out_cap`, `p->in_cap` (don't exist in current `Net_Graph`)
   - Uses `p->out_flow`, `p->in_flow` (calculated via methods, not fields)
   - Uses `this->residual_net` (doesn't exist)
   - Uses `this->unmake_residual_net()` (doesn't exist)
   - Uses `insert_arc(src, tgt, cap)` without 4th parameter (should include flow)

3. **Documentation in Spanish**

**Recommendation:** `tpl_net_sup_dem.H` requires a complete rewrite similar to what was done for `tpl_netcapgraph.H`, or it should be deprecated in favor of a modern implementation.

---

## Test Suite Created

Created comprehensive test suite in `/Tests/netcapgraph_test.cc` with **44 tests** covering:

### Edge Cases (18 tests)
- Empty network operations
- Single node with various capacities
- Capacity boundary values (0, epsilon, max)
- Arc with zero capacity
- Arc with flow = capacity
- Arc with flow > capacity (throws)
- Double compute of aux net (throws)
- Free/update without compute (throws)
- Copy and move construction

### Structural Tests (6 tests)
- Linear network (pipeline)
- Star network (hub and spokes)
- Diamond network (parallel paths)
- Complete network (all-to-all)
- Bipartite network

### Realistic Scenarios (3 tests)
- Data center network model
- Water distribution network
- Traffic network with intersections (4x4 grid)

### Stress Tests (3 tests)
- Large random network (500 nodes)
- Deep chain network (1000 nodes)
- Wide network (200 parallel paths)

### Validation Tests (6 tests)
- Flow reset
- Node capacity checking
- Setting valid/invalid capacities

### Type Variant Tests (4 tests)
- Integer flow types (`long`)
- String info types

### Cookie Mapping Tests (2 tests)
- Node to aux-arc mapping
- Arc to aux-arc mapping

### Fuzz Tests (2 tests)
- Random operations
- Extreme capacity values

---

## Compilation Status

✅ All code compiles successfully with only minor warnings (unrelated to this header)
✅ All 44 tests pass

---

## Recommendations

1. **Deprecate `tpl_net_sup_dem.H`** or rewrite it completely to use the modern API.

2. **Consider adding C++20 concepts** to constrain template parameters:
   ```cpp
   template <typename T>
   concept FlowType = std::is_arithmetic_v<T> && std::totally_ordered<T>;
   ```

3. **Add integration tests** that run actual max-flow algorithms on `Net_Cap_Graph` networks.

4. **Consider thread safety** if networks will be used in concurrent contexts.

---

## Files Modified

1. `/home/lrleon/Aleph-w/tpl_netcapgraph.H` - Complete rewrite
2. `/home/lrleon/Aleph-w/tpl_net_sup_dem.H` - Include fix only
3. `/home/lrleon/Aleph-w/Tests/netcapgraph_test.cc` - New test file
4. `/home/lrleon/Aleph-w/Tests/CMakeLists.txt` - Added new test target

---

## Appendix: API Changes Summary

| Old API | New API | Notes |
|---------|---------|-------|
| `Net_Node<Info, F_Type>` | `Net_Cap_Node<Info, F_Type>` | Custom node class |
| `#include <tpl_netgraph.H>` | `#include <tpl_net.H>` | Correct header |
| No move semantics | Move constructor/assignment | Modern C++ |
| Spanish docs | English docs | Internationalization |
| No validation | Throws on invalid input | Safer code |
| `No_Res_Arc` filter | Removed | Undefined symbol |

