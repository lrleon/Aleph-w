
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
 * @file net_apps_example.cc
 * @brief Network Flow Applications: Real-World Problem Solving
 * 
 * This example demonstrates how network flow algorithms solve diverse
 * real-world optimization problems. Many seemingly unrelated problems
 * can be reduced to max-flow or min-cut, enabling efficient polynomial-time
 * solutions. This showcases the power of algorithmic reductions.
 *
 * ## The Power of Reductions
 *
 * ### What is a Reduction?
 *
 * A **reduction** transforms problem A into problem B such that:
 * - Solution to B gives solution to A
 * - If B is polynomial-time, then A is also polynomial-time
 *
 * ### Why Network Flow?
 *
 * Network flow is powerful because:
 * - **Many problems reduce to it**: Diverse applications
 * - **Polynomial-time algorithms**: Efficient solutions exist
 * - **Min-cut duality**: Max-flow = Min-cut (powerful tool)
 * - **Well-studied**: Many efficient algorithms available
 *
 * ### Reduction Strategy
 *
 * To reduce problem P to max-flow:
 * 1. Model problem as network
 * 2. Encode constraints as capacities
 * 3. Encode objective as flow
 * 4. Solve max-flow
 * 5. Extract solution from flow
 *
 * ## Applications Demonstrated
 *
 * ### 1. Project Selection (Max Closure Problem)
 *
 * #### Problem Statement
 *
 * Choose projects to maximize profit while respecting dependencies:
 * - If project A requires project B, both must be chosen
 * - Some projects have profit, others have cost
 * - Goal: Maximize net profit
 *
 * #### Reduction to Min-Cut
 *
 * **Network construction**:
 * ```
 * Source → Projects with profit (capacity = profit)
 * Projects with cost → Sink (capacity = cost)
 * Dependencies: A requires B → B → A (infinite capacity)
 * ```
 *
 * **Key insight**: Min-cut separates profitable projects from costly ones.
 * Projects on source side are selected.
 *
 * #### Applications
 * - **Portfolio optimization**: Choose investments with dependencies
 * - **Feature selection**: Choose features respecting constraints
 * - **Resource planning**: Select projects optimally
 *
 * ### 2. Image Segmentation (Graph Cuts)
 *
 * #### Problem Statement
 *
 * Label each pixel as foreground or background optimally:
 * - Pixels similar to foreground should be foreground
 * - Pixels similar to background should be background
 * - Neighboring pixels should have same label (smoothness)
 *
 * #### Reduction to Min-Cut
 *
 * **Network construction**:
 * ```
 * Source → Pixels (capacity = foreground affinity)
 * Pixels → Sink (capacity = background affinity)
 * Neighboring pixels → edges (capacity = similarity penalty)
 * ```
 *
 * **Key insight**: Min-cut minimizes total penalty:
 * - Cut source-pixel edge: pixel is background
 * - Cut pixel-sink edge: pixel is foreground
 * - Cut pixel-pixel edge: neighbors have different labels
 *
 * #### Applications
 * - **Computer vision**: Object segmentation
 * - **Medical imaging**: Organ segmentation
 * - **Photo editing**: Background removal
 *
 * ### 3. Baseball Elimination
 *
 * #### Problem Statement
 *
 * Determine which teams can still win the league given:
 * - Current standings (wins per team)
 * - Remaining games (who plays whom)
 * - Can a specific team still win?
 *
 * #### Reduction to Max-Flow
 *
 * **Network construction**:
 * ```
 * Source → Games (capacity = 1 game)
 * Games → Teams (capacity = 1, represents game outcome)
 * Teams → Sink (capacity = max_wins - current_wins)
 * ```
 *
 * **Key insight**: If max-flow saturates all game edges, team can win
 * by distributing wins appropriately.
 *
 * #### Applications
 * - **Sports analytics**: Tournament analysis
 * - **Tournament scheduling**: Determine possible outcomes
 * - **Game theory**: Analyze competition scenarios
 *
 * ### 4. Survey Design
 *
 * #### Problem Statement
 *
 * Assign respondents to questions ensuring:
 * - Coverage: Each question answered by required number of people
 * - Constraints: Each person answers exactly k questions
 * - Feasibility: Is such assignment possible?
 *
 * #### Reduction to Max-Flow
 *
 * **Network construction**:
 * ```
 * Source → Respondents (capacity = k questions per person)
 * Respondents → Questions (capacity = 1, person can answer question)
 * Questions → Sink (capacity = required respondents per question)
 * ```
 *
 * **Key insight**: Max-flow = total assignments. If it equals
 * required coverage, assignment is possible.
 *
 * #### Applications
 * - **Market research**: Design surveys efficiently
 * - **Experimental design**: Assign treatments to subjects
 * - **Resource allocation**: Match resources to tasks
 *
 * ## General Reduction Pattern
 *
 * ### Max-Flow Reductions
 *
 * Problems reducible to max-flow often involve:
 * - **Matching**: Assigning items to slots
 * - **Coverage**: Ensuring requirements met
 * - **Flow constraints**: Capacity-like limitations
 *
 * ### Min-Cut Reductions
 *
 * Problems reducible to min-cut often involve:
 * - **Partitioning**: Dividing into two groups
 * - **Selection**: Choosing subset optimally
 * - **Labeling**: Assigning binary labels
 *
 * ## Complexity
 *
 * ### Polynomial-Time Solutions
 *
 * Since max-flow can be solved in polynomial time:
 * - **Edmonds-Karp**: O(VE²)
 * - **Dinic**: O(V²E)
 * - **HLPP**: O(V²√E)
 *
 * Any problem reducible to max-flow is also polynomial-time!
 *
 * ### Comparison with Alternatives
 *
 * | Problem | Direct Approach | Max-Flow Reduction |
 * |---------|----------------|-------------------|
 * | Project Selection | Exponential (try all) | O(VE²) |
 * | Image Segmentation | Heuristic | O(VE²) optimal |
 * | Baseball Elimination | Complex logic | O(VE²) |
 * | Survey Design | Constraint solving | O(VE²) |
 *
 * ## Key Insight
 *
 * Many optimization problems can be reduced to max-flow or min-cut,
 * allowing efficient polynomial-time solutions. The art is recognizing
 * when a problem has this structure!
 *
 * **Look for**:
 * - Binary choices (select/don't select)
 * - Capacity-like constraints
 * - Matching/assignment problems
 * - Partitioning problems
 *
 * ## Usage
 *
 * ```bash
 * # Run all application demos
 * ./net_apps_example
 *
 * # Run specific application
 * ./net_apps_example --project-selection
 * ./net_apps_example --image-segmentation
 * ./net_apps_example --baseball-elimination
 * ./net_apps_example --survey-design
 *
 * # Show help
 * ./net_apps_example --help
 * ```
 *
 * @see net_apps.H Application-specific reductions and utilities
 * @see tpl_maxflow.H Maximum flow algorithm implementations
 * @see network_flow_example.C Basic max-flow introduction
 * @see maxflow_advanced_example.cc Advanced max-flow algorithms
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <net_apps.H>

using namespace std;
using namespace Aleph;

static bool has_flag(int argc, char* argv[], const char* flag)
{
  for (int i = 1; i < argc; ++i)
    if (std::strcmp(argv[i], flag) == 0)
      return true;
  return false;
}

/**
 * @brief Demo 1: Project Selection Problem
 * 
 * A company must choose which projects to undertake.
 * Some projects have positive profit, others have costs.
 * Some projects depend on others (prerequisites).
 * 
 * Goal: Maximize total profit while respecting dependencies.
 */
void demo_project_selection()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Demo 1: Project Selection (Max Closure Problem)" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nScenario: A tech company is planning next year's projects." << endl;
  cout << "\nAvailable projects:" << endl;
  cout << "  ID  Name              Profit   Prerequisites" << endl;
  cout << "  --  ----              ------   -------------" << endl;
  cout << "  0   Mobile App        +$100K   -" << endl;
  cout << "  1   Backend API       +$80K    -" << endl;
  cout << "  2   Cloud Migration   -$50K    (infrastructure)" << endl;
  cout << "  3   ML Feature        +$120K   2 (needs cloud)" << endl;
  cout << "  4   Security Audit    -$30K    (compliance)" << endl;
  cout << "  5   GDPR Compliance   +$60K    4 (needs audit)" << endl;
  
  vector<Project<double>> projects = {
    {0, 100000, {}, "Mobile App"},
    {1, 80000, {}, "Backend API"},
    {2, -50000, {}, "Cloud Migration"},
    {3, 120000, {2}, "ML Feature"},      // Requires Cloud Migration
    {4, -30000, {}, "Security Audit"},
    {5, 60000, {4}, "GDPR Compliance"}   // Requires Security Audit
  };
  
  cout << "\n--- Solving with Max-Flow Min-Cut ---" << endl;
  
  auto result = solve_project_selection(projects);
  
  cout << "\n*** Optimal Selection ***" << endl;
  cout << "Maximum profit: $" << fixed << setprecision(0) 
       << result.max_profit << endl;
  
  cout << "\nSelected projects:" << endl;
  for (size_t id : result.selected)
  {
    const auto& p = projects[id];
    cout << "  [" << id << "] " << p.name 
         << " (" << (p.profit >= 0 ? "+" : "") << "$" << p.profit << ")" << endl;
  }
  
  cout << "\nAnalysis:" << endl;
  cout << "  Total revenue: $" << result.total_revenue << endl;
  cout << "  Total costs:   $" << result.total_cost << endl;
  cout << "  Net profit:    $" << result.max_profit << endl;
  
  cout << "\nNote: The ML Feature (+$120K) is selected despite requiring" << endl;
  cout << "Cloud Migration (-$50K) because net profit is +$70K." << endl;
}

/**
 * @brief Demo 2: Image Segmentation
 * 
 * Segment an image into foreground and background using graph cuts.
 * Each pixel has costs for being foreground or background.
 * Adjacent pixels prefer having the same label.
 */
void demo_image_segmentation()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Demo 2: Image Segmentation (Graph Cuts)" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nScenario: Segment a 4x4 grayscale image." << endl;
  cout << "Dark pixels (low intensity) -> Background (label 0)" << endl;
  cout << "Bright pixels (high intensity) -> Foreground (label 1)" << endl;
  
  // Simulated 4x4 image (grayscale intensities 0-255)
  // Dark regions are background, bright regions are foreground
  vector<vector<int>> image = {
    {30,  40,  180, 200},
    {35,  45,  190, 210},
    {40,  50,  185, 195},
    {50,  60,  170, 180}
  };
  
  cout << "\nOriginal image (intensities):" << endl;
  for (const auto& row : image)
  {
    cout << "  ";
    for (int val : row)
      cout << setw(4) << val;
    cout << endl;
  }
  
  // Convert to data costs
  // Cost of labeling pixel as background = intensity (high = expensive)
  // Cost of labeling pixel as foreground = 255 - intensity
  vector<vector<array<double, 2>>> data_cost(4, 
    vector<array<double, 2>>(4));
  
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
    {
      double intensity = image[i][j];
      data_cost[i][j][0] = intensity;        // Cost of background
      data_cost[i][j][1] = 255 - intensity;  // Cost of foreground
    }
  
  double smoothness = 50.0;  // Penalty for adjacent pixels with different labels
  
  cout << "\nSmooth penalty for label changes: " << smoothness << endl;
  
  cout << "\n--- Computing Optimal Segmentation ---" << endl;
  
  auto result = segment_image(4, 4, data_cost, smoothness);
  
  cout << "\nSegmentation result (0=background, 1=foreground):" << endl;
  for (const auto& row : result.labels)
  {
    cout << "  ";
    for (int label : row)
      cout << setw(4) << (label ? "FG" : "BG");
    cout << endl;
  }
  
  cout << "\nTotal energy: " << result.energy << endl;
  cout << "(Lower energy = better segmentation)" << endl;
  
  cout << "\nVisualization (# = foreground, . = background):" << endl;
  for (const auto& row : result.labels)
  {
    cout << "  ";
    for (int label : row)
      cout << (label ? " # " : " . ");
    cout << endl;
  }
  
  cout << "\nNote: The algorithm correctly separates the bright right half" << endl;
  cout << "(foreground) from the dark left half (background)." << endl;
}

/**
 * @brief Demo 3: Baseball Elimination
 * 
 * Determine if a team is mathematically eliminated from winning.
 * A team is eliminated if no outcome of remaining games allows them to finish first.
 */
void demo_baseball_elimination()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Demo 3: Baseball Elimination" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nScenario: Late-season standings in a 4-team division." << endl;
  cout << "\nCurrent standings:" << endl;
  cout << "  Team      Wins  Losses  Remaining" << endl;
  cout << "  ----      ----  ------  ---------" << endl;
  cout << "  Yankees    83     71        8" << endl;
  cout << "  Red Sox    80     79        3" << endl;
  cout << "  Blue Jays  78     78        6" << endl;
  cout << "  Orioles    75     84        3" << endl;
  
  vector<Team> teams;
  
  // Create teams and set their against games
  Team yankees("Yankees", 83, 71, 8);
  yankees.against = {0, 1, 2, 1};  // Games vs each team
  teams.push_back(yankees);
  
  Team redsox("Red Sox", 80, 79, 3);
  redsox.against = {1, 0, 0, 2};
  teams.push_back(redsox);
  
  Team bluejays("Blue Jays", 78, 78, 6);
  bluejays.against = {2, 0, 0, 4};
  teams.push_back(bluejays);
  
  Team orioles("Orioles", 75, 84, 3);
  orioles.against = {1, 2, 4, 0};
  teams.push_back(orioles);
  
  cout << "\nRemaining games matrix:" << endl;
  cout << "         NYY  BOS  TOR  BAL" << endl;
  for (size_t i = 0; i < teams.size(); ++i)
  {
    cout << "  " << setw(4) << left << teams[i].name.substr(0, 3) << ":  ";
    for (int g : teams[i].against)
      cout << setw(4) << g;
    cout << endl;
  }
  
  cout << "\n--- Checking Each Team's Elimination Status ---" << endl;
  
  for (size_t i = 0; i < teams.size(); ++i)
  {
    auto result = check_baseball_elimination(teams, i);
    
    cout << "\n" << teams[i].name << ": ";
    if (result.eliminated)
    {
      cout << "ELIMINATED!" << endl;
      cout << "  Max possible wins: " << result.max_possible_wins << endl;
      if (!result.certificate.empty())
      {
        cout << "  Certificate (teams blocking): ";
        for (size_t t : result.certificate)
          cout << teams[t].name << " ";
        cout << endl;
      }
    }
    else
    {
      cout << "Can still win!" << endl;
      cout << "  Max possible wins: " << result.max_possible_wins << endl;
    }
  }
  
  cout << "\nNote: A team is eliminated if the sum of wins among a subset" << endl;
  cout << "of other teams, plus their remaining games among themselves," << endl;
  cout << "makes it impossible to finish in first place." << endl;
}

/**
 * @brief Demo 4: Survey Design
 * 
 * Assign respondents to survey questions respecting constraints:
 * - Each question needs min-max responses
 * - Each respondent answers min-max questions
 * - Respondents can only answer eligible questions
 */
void demo_survey_design()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Demo 4: Survey Design" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nScenario: Design a customer feedback survey." << endl;
  cout << "\nQuestions:" << endl;
  cout << "  Q0: Product quality    (needs 2-4 responses)" << endl;
  cout << "  Q1: Customer service   (needs 2-3 responses)" << endl;
  cout << "  Q2: Pricing feedback   (needs 1-3 responses)" << endl;
  
  cout << "\nRespondents:" << endl;
  cout << "  R0: Can answer Q0, Q1    (answers 1-2 questions)" << endl;
  cout << "  R1: Can answer Q0, Q2    (answers 1-2 questions)" << endl;
  cout << "  R2: Can answer Q1, Q2    (answers 1-2 questions)" << endl;
  cout << "  R3: Can answer Q0, Q1, Q2 (answers 2-3 questions)" << endl;
  
  vector<SurveyQuestion> questions = {
    {0, 2, 4},  // Q0: min 2, max 4 responses
    {1, 2, 3},  // Q1: min 2, max 3 responses
    {2, 1, 3}   // Q2: min 1, max 3 responses
  };
  
  vector<SurveyRespondent> respondents = {
    {0, 1, 2, {0, 1}},     // R0: 1-2 questions, eligible for Q0, Q1
    {1, 1, 2, {0, 2}},     // R1: 1-2 questions, eligible for Q0, Q2
    {2, 1, 2, {1, 2}},     // R2: 1-2 questions, eligible for Q1, Q2
    {3, 2, 3, {0, 1, 2}}   // R3: 2-3 questions, eligible for all
  };
  
  cout << "\n--- Finding Feasible Assignment ---" << endl;
  
  auto result = design_survey(questions, respondents);
  
  if (result.feasible)
  {
    cout << "\n*** Feasible Assignment Found! ***" << endl;
    
    // Group by respondent
    map<size_t, vector<size_t>> resp_questions;
    for (const auto& [r, q] : result.assignments)
      resp_questions[r].push_back(q);
    
    cout << "\nAssignments:" << endl;
    for (const auto& [r, qs] : resp_questions)
    {
      cout << "  Respondent " << r << " answers: ";
      for (size_t q : qs)
        cout << "Q" << q << " ";
      cout << endl;
    }
    
    // Verify question coverage
    cout << "\nQuestion coverage:" << endl;
    for (size_t q = 0; q < questions.size(); ++q)
    {
      int count = 0;
      for (const auto& [r, qid] : result.assignments)
        if (qid == q) count++;
      
      cout << "  Q" << q << ": " << count << " responses "
           << "(need " << questions[q].min_responses << "-" 
           << questions[q].max_responses << ")" << endl;
    }
  }
  else
  {
    cout << "\nNo feasible assignment exists!" << endl;
    cout << "The constraints are too restrictive." << endl;
  }
}

int main(int argc, char* argv[])
{
  cout << "=== Network Flow Applications ===" << endl;
  cout << "Real-world problems solved with max-flow/min-cut\n" << endl;

  if (has_flag(argc, argv, "--help"))
    {
      cout << "Usage: " << argv[0] << " [--project-selection] [--image-segmentation] "
              "[--baseball-elimination] [--survey-design] [--help]\n";
      cout << "\nIf no flags are given, all demos are executed.\n";
      return 0;
    }

  const bool any_specific =
    has_flag(argc, argv, "--project-selection") ||
    has_flag(argc, argv, "--image-segmentation") ||
    has_flag(argc, argv, "--baseball-elimination") ||
    has_flag(argc, argv, "--survey-design");

  if (!any_specific || has_flag(argc, argv, "--project-selection"))
    demo_project_selection();

  if (!any_specific || has_flag(argc, argv, "--image-segmentation"))
    demo_image_segmentation();

  if (!any_specific || has_flag(argc, argv, "--baseball-elimination"))
    demo_baseball_elimination();

  if (!any_specific || has_flag(argc, argv, "--survey-design"))
    demo_survey_design();
  
  // Summary
  cout << "\n" << string(60, '=') << endl;
  cout << "Summary" << endl;
  cout << string(60, '=') << endl;
  
  cout << R"(
These problems share a common structure:

1. **Project Selection** (Max Closure)
   - Model: Projects as nodes, dependencies as infinite edges
   - Solution: Min s-t cut separates selected from rejected

2. **Image Segmentation** (Graph Cuts)
   - Model: Pixels as nodes, neighbor edges with smoothness cost
   - Solution: Min cut optimally labels foreground/background

3. **Baseball Elimination**
   - Model: Game outcomes as flow through team vertices
   - Solution: If max-flow < total games, team is eliminated

4. **Survey Design** (Feasibility)
   - Model: Bipartite matching with lower bounds
   - Solution: Flow satisfies constraints if feasible

Key Insight:
  Many combinatorial problems reduce to max-flow/min-cut,
  yielding polynomial-time algorithms for NP-hard looking problems!
)" << endl;
  
  return 0;
}
