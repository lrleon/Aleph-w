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
 * @file simplex_example.C
 * @brief Simplex Algorithm: Linear Programming Optimization
 * 
 * This example demonstrates the Simplex algorithm for solving linear
 * programming (LP) problems - one of the most important algorithms
 * in optimization and operations research.
 * 
 * ## What is Linear Programming?
 * 
 * Linear programming finds the optimal value (maximum or minimum)
 * of a linear objective function subject to linear constraints.
 * 
 * Standard form:
 *   Maximize:   Z = c₁x₁ + c₂x₂ + ... + cₙxₙ
 *   Subject to: a₁₁x₁ + a₁₂x₂ + ... ≤ b₁
 *               a₂₁x₁ + a₂₂x₂ + ... ≤ b₂
 *               xᵢ ≥ 0 (non-negativity)
 * 
 * ## Applications
 * 
 * - **Production planning**: Maximize profit given resource limits
 * - **Supply chain**: Minimize transportation costs
 * - **Finance**: Portfolio optimization
 * - **Scheduling**: Resource allocation
 * - **Network flow**: Routing and capacity planning
 * 
 * ## Complexity
 * 
 * - Worst case: O(2^n)
 * - Average case: Polynomial (typically fast in practice)
 * 
 * @see Simplex.H
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <Simplex.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

/**
 * @brief Helper to print solution state
 */
string state_to_string(Simplex<double>::State state)
{
  switch (state)
  {
    case Simplex<double>::Not_Solved: return "Not Solved";
    case Simplex<double>::Solving: return "Solving";
    case Simplex<double>::Unbounded: return "Unbounded";
    case Simplex<double>::Solved: return "Solved";
    case Simplex<double>::Unfeasible: return "Unfeasible";
    default: return "Unknown";
  }
}

/**
 * @brief Classic example: Production Planning
 * 
 * A factory produces two products (A and B):
 * - Product A: $40 profit, needs 1 hr labor, 2 hrs machine
 * - Product B: $30 profit, needs 1 hr labor, 1 hr machine
 * - Available: 40 labor hours, 60 machine hours
 * 
 * Maximize: Z = 40*xA + 30*xB
 * Subject to: xA + xB ≤ 40 (labor)
 *             2*xA + xB ≤ 60 (machine)
 */
void demo_production_planning()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Example 1: Production Planning Problem" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nProblem:" << endl;
  cout << "  Factory produces two products (A and B)" << endl;
  cout << "  Product A: $40 profit, 1 hr labor, 2 hrs machine" << endl;
  cout << "  Product B: $30 profit, 1 hr labor, 1 hr machine" << endl;
  cout << "  Available: 40 labor hours, 60 machine hours" << endl;
  
  cout << "\nMathematical formulation:" << endl;
  cout << "  Maximize:    Z = 40*xA + 30*xB" << endl;
  cout << "  Subject to:  xA + xB <= 40 (labor)" << endl;
  cout << "               2*xA + xB <= 60 (machine)" << endl;
  cout << "               xA, xB >= 0" << endl;
  
  // Create solver with 2 decision variables
  Simplex<double> solver(2);
  
  // Set objective function: maximize 40*xA + 30*xB
  solver.put_objetive_function_coef(0, 40.0);  // xA coefficient
  solver.put_objetive_function_coef(1, 30.0);  // xB coefficient
  
  // Add constraints: {coef_xA, coef_xB, RHS}
  double labor[] = {1.0, 1.0, 40.0};    // xA + xB <= 40
  double machine[] = {2.0, 1.0, 60.0};  // 2*xA + xB <= 60
  
  solver.put_restriction(labor);
  solver.put_restriction(machine);
  
  // Solve
  solver.prepare_linear_program();
  auto state = solver.solve();
  
  cout << "\n--- Solution ---" << endl;
  cout << "Status: " << state_to_string(state) << endl;
  
  if (state == Simplex<double>::Solved)
  {
    solver.load_solution();
    
    double xA = solver.get_solution(0);
    double xB = solver.get_solution(1);
    double profit = solver.objetive_value();
    
    cout << fixed << setprecision(2);
    cout << "  Product A (xA): " << xA << " units" << endl;
    cout << "  Product B (xB): " << xB << " units" << endl;
    cout << "  Maximum profit: $" << profit << endl;
    
    // Verify constraints
    cout << "\n--- Constraint Verification ---" << endl;
    cout << "  Labor used: " << (xA + xB) << " / 40 hours" << endl;
    cout << "  Machine used: " << (2*xA + xB) << " / 60 hours" << endl;
    
    if (solver.verify_solution())
      cout << "  All constraints satisfied: YES" << endl;
  }
}

/**
 * @brief Diet Problem (classic LP problem)
 * 
 * Find minimum cost diet meeting nutritional requirements:
 * - Food 1: $2/unit, 3g protein, 1g fat
 * - Food 2: $4/unit, 4g protein, 3g fat
 * - Need: at least 12g protein, at least 6g fat
 * 
 * Note: This is a minimization problem.
 * Minimize: C = 2*x1 + 4*x2
 * Subject to: 3*x1 + 4*x2 >= 12 (protein)
 *             1*x1 + 3*x2 >= 6 (fat)
 * 
 * Convert to standard form (maximization):
 * Maximize: -C = -2*x1 - 4*x2
 * Convert >= to <= by multiplying by -1:
 * -3*x1 - 4*x2 <= -12
 * -1*x1 - 3*x2 <= -6
 */
void demo_diet_problem()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Example 2: Diet Problem (Minimization)" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nProblem:" << endl;
  cout << "  Food 1: $2/unit, 3g protein, 1g fat" << endl;
  cout << "  Food 2: $4/unit, 4g protein, 3g fat" << endl;
  cout << "  Requirements: >= 12g protein, >= 6g fat" << endl;
  
  cout << "\nOriginal formulation:" << endl;
  cout << "  Minimize: C = 2*x1 + 4*x2" << endl;
  cout << "  Subject to: 3*x1 + 4*x2 >= 12" << endl;
  cout << "              x1 + 3*x2 >= 6" << endl;
  
  cout << "\nConverted to standard form:" << endl;
  cout << "  Maximize: -C = -2*x1 - 4*x2" << endl;
  cout << "  Constraints multiplied by -1 for <= form" << endl;
  
  Simplex<double> solver(2);
  
  // Objective: maximize -2*x1 - 4*x2 (to minimize 2*x1 + 4*x2)
  solver.put_objetive_function_coef(0, -2.0);
  solver.put_objetive_function_coef(1, -4.0);
  
  // Constraints (converted from >= to <=)
  double protein[] = {-3.0, -4.0, -12.0};  // -3*x1 - 4*x2 <= -12
  double fat[] = {-1.0, -3.0, -6.0};       // -x1 - 3*x2 <= -6
  
  solver.put_restriction(protein);
  solver.put_restriction(fat);
  
  solver.prepare_linear_program();
  auto state = solver.solve();
  
  cout << "\n--- Solution ---" << endl;
  cout << "Status: " << state_to_string(state) << endl;
  
  if (state == Simplex<double>::Solved)
  {
    solver.load_solution();
    
    double x1 = solver.get_solution(0);
    double x2 = solver.get_solution(1);
    double cost = -solver.objetive_value();  // Negate to get original min
    
    cout << fixed << setprecision(2);
    cout << "  Food 1 (x1): " << x1 << " units" << endl;
    cout << "  Food 2 (x2): " << x2 << " units" << endl;
    cout << "  Minimum cost: $" << cost << endl;
    
    cout << "\n--- Nutritional Check ---" << endl;
    cout << "  Protein: " << (3*x1 + 4*x2) << "g (need >= 12g)" << endl;
    cout << "  Fat: " << (x1 + 3*x2) << "g (need >= 6g)" << endl;
  }
}

/**
 * @brief Transportation Problem (simplified)
 * 
 * Allocate resources to maximize output:
 * - Resource X: 2 units capacity, yields 5 per unit
 * - Resource Y: 3 units capacity, yields 4 per unit
 * - Resource Z: 4 units capacity, yields 3 per unit
 * - Total capacity limit: 6 units
 */
void demo_resource_allocation()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Example 3: Resource Allocation" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nProblem:" << endl;
  cout << "  Resource X: max 2 units, yields 5/unit" << endl;
  cout << "  Resource Y: max 3 units, yields 4/unit" << endl;
  cout << "  Resource Z: max 4 units, yields 3/unit" << endl;
  cout << "  Total capacity: max 6 units" << endl;
  
  cout << "\nFormulation:" << endl;
  cout << "  Maximize: Z = 5*x + 4*y + 3*z" << endl;
  cout << "  Subject to: x <= 2" << endl;
  cout << "              y <= 3" << endl;
  cout << "              z <= 4" << endl;
  cout << "              x + y + z <= 6" << endl;
  
  Simplex<double> solver(3);
  
  // Objective: maximize 5x + 4y + 3z
  solver.put_objetive_function_coef(0, 5.0);  // x
  solver.put_objetive_function_coef(1, 4.0);  // y
  solver.put_objetive_function_coef(2, 3.0);  // z
  
  // Constraints
  double limit_x[] = {1.0, 0.0, 0.0, 2.0};     // x <= 2
  double limit_y[] = {0.0, 1.0, 0.0, 3.0};     // y <= 3
  double limit_z[] = {0.0, 0.0, 1.0, 4.0};     // z <= 4
  double total[] = {1.0, 1.0, 1.0, 6.0};       // x + y + z <= 6
  
  solver.put_restriction(limit_x);
  solver.put_restriction(limit_y);
  solver.put_restriction(limit_z);
  solver.put_restriction(total);
  
  solver.prepare_linear_program();
  auto state = solver.solve();
  
  cout << "\n--- Solution ---" << endl;
  cout << "Status: " << state_to_string(state) << endl;
  
  if (state == Simplex<double>::Solved)
  {
    solver.load_solution();
    
    double x = solver.get_solution(0);
    double y = solver.get_solution(1);
    double z = solver.get_solution(2);
    double yield = solver.objetive_value();
    
    cout << fixed << setprecision(2);
    cout << "  Resource X: " << x << " units" << endl;
    cout << "  Resource Y: " << y << " units" << endl;
    cout << "  Resource Z: " << z << " units" << endl;
    cout << "  Total allocated: " << (x + y + z) << " / 6 units" << endl;
    cout << "  Maximum yield: " << yield << endl;
    
    cout << "\n--- Analysis ---" << endl;
    cout << "Notice how the algorithm prioritizes higher-yield resources" << endl;
    cout << "while respecting all constraints." << endl;
  }
}

/**
 * @brief Demonstrate unbounded and infeasible cases
 */
void demo_special_cases()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Special Cases: Unbounded and Infeasible" << endl;
  cout << string(60, '=') << endl;
  
  // Example of potentially unbounded (if no upper bound)
  cout << "\n--- Case 1: Well-defined problem ---" << endl;
  cout << "Maximize: Z = x + y" << endl;
  cout << "Subject to: x + y <= 10" << endl;
  
  {
    Simplex<double> solver(2);
    solver.put_objetive_function_coef(0, 1.0);
    solver.put_objetive_function_coef(1, 1.0);
    
    double limit[] = {1.0, 1.0, 10.0};
    solver.put_restriction(limit);
    
    solver.prepare_linear_program();
    auto state = solver.solve();
    
    cout << "Status: " << state_to_string(state) << endl;
    if (state == Simplex<double>::Solved)
    {
      solver.load_solution();
      cout << "Optimal value: " << solver.objetive_value() << endl;
    }
  }
  
  // Example with conflicting constraints
  cout << "\n--- Case 2: Conflicting constraints ---" << endl;
  cout << "Maximize: Z = x + y" << endl;
  cout << "Subject to: x + y <= 5" << endl;
  cout << "            x + y >= 10 (converted to -x - y <= -10)" << endl;
  
  {
    Simplex<double> solver(2);
    solver.put_objetive_function_coef(0, 1.0);
    solver.put_objetive_function_coef(1, 1.0);
    
    double limit1[] = {1.0, 1.0, 5.0};     // x + y <= 5
    double limit2[] = {-1.0, -1.0, -10.0}; // x + y >= 10
    
    solver.put_restriction(limit1);
    solver.put_restriction(limit2);
    
    solver.prepare_linear_program();
    auto state = solver.solve();
    
    cout << "Status: " << state_to_string(state) << endl;
    cout << "(x + y can't be both <= 5 and >= 10 simultaneously)" << endl;
  }
}

/**
 * @brief Demonstrate the algorithm steps
 */
void demo_algorithm_steps()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Understanding the Simplex Algorithm" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\n--- The Simplex Method Steps ---" << endl;
  cout << "1. Convert to standard form (slack variables)" << endl;
  cout << "2. Build initial simplex tableau" << endl;
  cout << "3. Find pivot column (most negative in objective row)" << endl;
  cout << "4. Find pivot row (minimum ratio test)" << endl;
  cout << "5. Pivot to improve solution" << endl;
  cout << "6. Repeat until optimal (no negative in objective row)" << endl;
  
  cout << "\n--- Geometric Interpretation ---" << endl;
  cout << "The constraints define a convex polytope (feasible region)." << endl;
  cout << "Simplex moves along edges of this polytope," << endl;
  cout << "visiting vertices until it finds the optimum." << endl;
  cout << "Each pivot operation moves to an adjacent vertex" << endl;
  cout << "with a better (or equal) objective value." << endl;
  
  cout << "\n--- Simple Example Visualization ---" << endl;
  cout << "Maximize: Z = x + y" << endl;
  cout << "Subject to: x <= 3, y <= 2, x + y <= 4" << endl;
  
  Simplex<double> solver(2);
  solver.put_objetive_function_coef(0, 1.0);
  solver.put_objetive_function_coef(1, 1.0);
  
  double c1[] = {1.0, 0.0, 3.0};  // x <= 3
  double c2[] = {0.0, 1.0, 2.0};  // y <= 2
  double c3[] = {1.0, 1.0, 4.0};  // x + y <= 4
  
  solver.put_restriction(c1);
  solver.put_restriction(c2);
  solver.put_restriction(c3);
  
  solver.prepare_linear_program();
  auto state = solver.solve();
  
  if (state == Simplex<double>::Solved)
  {
    solver.load_solution();
    
    cout << "\nFeasible region vertices:" << endl;
    cout << "  (0, 0): Z = 0" << endl;
    cout << "  (3, 0): Z = 3" << endl;
    cout << "  (3, 1): Z = 4  <-- binding x<=3 and x+y<=4" << endl;
    cout << "  (2, 2): Z = 4  <-- binding y<=2 and x+y<=4" << endl;
    cout << "  (0, 2): Z = 2" << endl;
    
    cout << fixed << setprecision(2);
    cout << "\nSimplex found: (" << solver.get_solution(0) 
         << ", " << solver.get_solution(1) << ")" << endl;
    cout << "Optimal value: Z = " << solver.objetive_value() << endl;
  }
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("Simplex Linear Programming Example", ' ', "1.0");
    
    TCLAP::SwitchArg prodArg("p", "production",
      "Show production planning example", false);
    TCLAP::SwitchArg dietArg("d", "diet",
      "Show diet problem example", false);
    TCLAP::SwitchArg resArg("r", "resources",
      "Show resource allocation example", false);
    TCLAP::SwitchArg specArg("s", "special",
      "Show special cases (unbounded, infeasible)", false);
    TCLAP::SwitchArg algoArg("g", "algorithm",
      "Show algorithm explanation", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);
    
    cmd.add(prodArg);
    cmd.add(dietArg);
    cmd.add(resArg);
    cmd.add(specArg);
    cmd.add(algoArg);
    cmd.add(allArg);
    
    cmd.parse(argc, argv);
    
    bool runProd = prodArg.getValue();
    bool runDiet = dietArg.getValue();
    bool runRes = resArg.getValue();
    bool runSpec = specArg.getValue();
    bool runAlgo = algoArg.getValue();
    bool runAll = allArg.getValue();
    
    if (not runProd and not runDiet and not runRes and 
        not runSpec and not runAlgo)
      runAll = true;
    
    cout << "=== Simplex Algorithm: Linear Programming ===" << endl;
    cout << "Find optimal solutions subject to linear constraints" << endl;
    
    if (runAll or runProd)
      demo_production_planning();
    
    if (runAll or runDiet)
      demo_diet_problem();
    
    if (runAll or runRes)
      demo_resource_allocation();
    
    if (runAll or runSpec)
      demo_special_cases();
    
    if (runAll or runAlgo)
      demo_algorithm_steps();
    
    cout << "\n=== Summary ===" << endl;
    cout << "Simplex is the workhorse of linear programming." << endl;
    cout << "Standard form: Maximize c'x subject to Ax <= b, x >= 0" << endl;
    cout << "Convert: Min -> Max (negate), >= -> <= (negate)" << endl;
    cout << "Applications: Production, logistics, finance, scheduling" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}

