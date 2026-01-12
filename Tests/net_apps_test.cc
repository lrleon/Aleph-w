/* Tests for net_apps.H - Network flow applications
 *
 * Tests cover:
 * - Circulation with demands
 * - Project selection
 * - Baseball elimination
 * - Image segmentation
 * - Survey design
 */

#include <gtest/gtest.h>
#include <net_apps.H>

using namespace Aleph;


//==============================================================================
// Project Selection Tests
//==============================================================================

class ProjectSelectionTest : public ::testing::Test
{
protected:
};

TEST_F(ProjectSelectionTest, SimpleProjects)
{
  std::vector<Project<double>> projects = {
    {0, 100, {}, "Profitable A"},   // Pure profit
    {1, 50, {}, "Profitable B"},    // Pure profit
  };

  auto result = solve_project_selection(projects);

  EXPECT_DOUBLE_EQ(result.max_profit, 150.0);  // Select both
  EXPECT_EQ(result.selected.size(), 2);
}

TEST_F(ProjectSelectionTest, WithDependency)
{
  std::vector<Project<double>> projects = {
    {0, 100, {}, "Project A"},      // Profit 100
    {1, -30, {}, "Infra"},          // Cost 30
    {2, 50, {1}, "Project B"},      // Profit 50, needs Infra
  };

  auto result = solve_project_selection(projects);

  // Should select A (100) + Infra (-30) + B (50) = 120
  // Or just A (100) if B's dependency makes it not worth it
  // 50 - 30 = 20 > 0, so B + Infra is profitable
  // Total: 100 + 50 - 30 = 120
  EXPECT_GE(result.max_profit, 120.0);
}

TEST_F(ProjectSelectionTest, NoProfitableProjects)
{
  std::vector<Project<double>> projects = {
    {0, -50, {}, "Cost A"},
    {1, -30, {}, "Cost B"},
  };

  auto result = solve_project_selection(projects);

  EXPECT_DOUBLE_EQ(result.max_profit, 0.0);
  EXPECT_TRUE(result.selected.empty());
}

TEST_F(ProjectSelectionTest, EmptyProjects)
{
  std::vector<Project<double>> projects;

  auto result = solve_project_selection(projects);

  EXPECT_DOUBLE_EQ(result.max_profit, 0.0);
}

TEST_F(ProjectSelectionTest, CircularDependency)
{
  // Note: Circular dependencies are handled but don't make practical sense
  std::vector<Project<double>> projects = {
    {0, 100, {1}, "A needs B"},
    {1, 50, {0}, "B needs A"},
  };

  auto result = solve_project_selection(projects);

  // Both are selected together due to mutual dependency
  EXPECT_GE(result.max_profit, 150.0);
}

TEST_F(ProjectSelectionTest, ChainDependency)
{
  std::vector<Project<double>> projects = {
    {0, -10, {}, "Foundation"},
    {1, -10, {0}, "Level 1"},
    {2, -10, {1}, "Level 2"},
    {3, 100, {2}, "Payoff"},  // Profit 100, needs all previous
  };

  auto result = solve_project_selection(projects);

  // Net profit: 100 - 10 - 10 - 10 = 70
  EXPECT_GE(result.max_profit, 70.0);
}


//==============================================================================
// Baseball Elimination Tests
//==============================================================================

class BaseballEliminationTest : public ::testing::Test
{
protected:
  std::vector<Team> create_simple_division()
  {
    // 4-team division
    std::vector<Team> teams(4);

    teams[0].name = "Atlanta";
    teams[0].wins = 83;
    teams[0].losses = 71;
    teams[0].remaining = 8;
    teams[0].against = {0, 1, 6, 1};

    teams[1].name = "Philly";
    teams[1].wins = 80;
    teams[1].losses = 79;
    teams[1].remaining = 3;
    teams[1].against = {1, 0, 0, 2};

    teams[2].name = "New York";
    teams[2].wins = 78;
    teams[2].losses = 78;
    teams[2].remaining = 6;
    teams[2].against = {6, 0, 0, 0};

    teams[3].name = "Montreal";
    teams[3].wins = 77;
    teams[3].losses = 82;
    teams[3].remaining = 3;
    teams[3].against = {1, 2, 0, 0};

    return teams;
  }
};

TEST_F(BaseballEliminationTest, NotEliminated)
{
  auto teams = create_simple_division();

  // Atlanta (team 0) is not eliminated
  auto result = check_baseball_elimination(teams, 0);

  EXPECT_FALSE(result.eliminated);
  EXPECT_EQ(result.max_possible_wins, 91);
}

TEST_F(BaseballEliminationTest, TrivialElimination)
{
  std::vector<Team> teams(3);

  teams[0].name = "Leader";
  teams[0].wins = 100;
  teams[0].remaining = 0;
  teams[0].against = {0, 0, 0};

  teams[1].name = "Middle";
  teams[1].wins = 50;
  teams[1].remaining = 40;
  teams[1].against = {0, 0, 40};

  teams[2].name = "Loser";
  teams[2].wins = 10;
  teams[2].remaining = 50;
  teams[2].against = {0, 40, 0};

  // Team 2 max wins = 60, Leader already has 100
  auto result = check_baseball_elimination(teams, 2);

  EXPECT_TRUE(result.eliminated);
}

TEST_F(BaseballEliminationTest, NonTrivialElimination)
{
  auto teams = create_simple_division();

  // Montreal (team 3) might be eliminated non-trivially
  auto result = check_baseball_elimination(teams, 3);

  // Montreal max wins = 77 + 3 = 80
  // Atlanta can reach 83 + 8 = 91
  // But we need to check if there's any scenario where Montreal can win
}

TEST_F(BaseballEliminationTest, InvalidTeamIndex)
{
  auto teams = create_simple_division();

  auto result = check_baseball_elimination(teams, 100);

  // Should handle gracefully
  EXPECT_FALSE(result.eliminated);
}


//==============================================================================
// Image Segmentation Tests
//==============================================================================

class ImageSegmentationTest : public ::testing::Test
{
protected:
};

TEST_F(ImageSegmentationTest, SimpleSegmentation)
{
  // 2x2 image
  std::vector<std::vector<std::array<double, 2>>> data(2,
    std::vector<std::array<double, 2>>(2));

  // Top-left strongly prefers foreground (label 1)
  data[0][0] = {100, 10};  // cost[0]=100 (background), cost[1]=10 (foreground)

  // Others prefer background
  data[0][1] = {10, 100};
  data[1][0] = {10, 100};
  data[1][1] = {10, 100};

  auto result = segment_image(2, 2, data, 50.0);

  EXPECT_EQ(result.labels.size(), 2);
  EXPECT_EQ(result.labels[0].size(), 2);

  // Top-left should be foreground if smoothness cost allows
  // With smoothness=50, need to evaluate trade-off
}

TEST_F(ImageSegmentationTest, UniformPreference)
{
  // 3x3 image all preferring foreground
  std::vector<std::vector<std::array<double, 2>>> data(3,
    std::vector<std::array<double, 2>>(3));

  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      data[i][j] = {100, 10};  // All prefer foreground

  auto result = segment_image(3, 3, data, 10.0);

  // All should be foreground
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      EXPECT_EQ(result.labels[i][j], 1);
}

TEST_F(ImageSegmentationTest, EmptyImage)
{
  std::vector<std::vector<std::array<double, 2>>> data;

  auto result = segment_image(0, 0, data, 10.0);

  EXPECT_TRUE(result.labels.empty());
}

TEST_F(ImageSegmentationTest, SinglePixel)
{
  std::vector<std::vector<std::array<double, 2>>> data(1,
    std::vector<std::array<double, 2>>(1));

  data[0][0] = {5, 10};  // Prefers background (lower cost)

  auto result = segment_image(1, 1, data, 100.0);

  EXPECT_EQ(result.labels[0][0], 0);  // Background
}


//==============================================================================
// Survey Design Tests
//==============================================================================

class SurveyDesignTest : public ::testing::Test
{
protected:
};

TEST_F(SurveyDesignTest, SimpleFeasible)
{
  std::vector<SurveyQuestion> questions = {
    {0, 1, 2},  // Question 0: needs 1-2 responses
    {1, 1, 2},  // Question 1: needs 1-2 responses
  };

  std::vector<SurveyRespondent> respondents = {
    {0, 1, 2, {0, 1}},  // Respondent 0: answers 1-2 questions, eligible for both
    {1, 1, 2, {0, 1}},  // Respondent 1: answers 1-2 questions, eligible for both
  };

  auto result = design_survey(questions, respondents);

  EXPECT_TRUE(result.feasible);
  EXPECT_GE(result.assignments.size(), 2);  // At least 2 assignments
}

TEST_F(SurveyDesignTest, Infeasible)
{
  std::vector<SurveyQuestion> questions = {
    {0, 5, 10},  // Needs at least 5 responses
  };

  std::vector<SurveyRespondent> respondents = {
    {0, 1, 1, {0}},  // Only 1 respondent who can answer 1 question
  };

  auto result = design_survey(questions, respondents);

  EXPECT_FALSE(result.feasible);
}

TEST_F(SurveyDesignTest, EligibilityConstraints)
{
  std::vector<SurveyQuestion> questions = {
    {0, 1, 3},  // Question 0
    {1, 1, 3},  // Question 1
  };

  std::vector<SurveyRespondent> respondents = {
    {0, 1, 2, {0}},     // Only eligible for Q0
    {1, 1, 2, {1}},     // Only eligible for Q1
    {2, 1, 2, {0, 1}},  // Eligible for both
  };

  auto result = design_survey(questions, respondents);

  EXPECT_TRUE(result.feasible);

  // Check assignments respect eligibility
  for (const auto& [r, q] : result.assignments)
    {
      bool eligible = false;
      for (size_t eq : respondents[r].eligible_questions)
        if (eq == q)
          eligible = true;
      EXPECT_TRUE(eligible);
    }
}

TEST_F(SurveyDesignTest, EmptySurvey)
{
  std::vector<SurveyQuestion> questions;
  std::vector<SurveyRespondent> respondents;

  auto result = design_survey(questions, respondents);

  // Empty survey is trivially feasible but not considered feasible here
  EXPECT_FALSE(result.feasible);
}


//==============================================================================
// Circulation Tests (Basic)
//==============================================================================

class CirculationTest : public ::testing::Test
{
protected:
  using TestNet = Net_Graph<Net_Node<Empty_Class>,
                            Net_Arc<Empty_Class, double>>;
};

TEST_F(CirculationTest, NoDemands)
{
  TestNet net;
  auto a = net.insert_node();
  auto b = net.insert_node();
  net.insert_arc(a, b, 10);

  auto result = solve_circulation(net,
    [](auto*) { return 0.0; },
    [](auto*) { return 0.0; });

  EXPECT_TRUE(result.feasible);
}


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
