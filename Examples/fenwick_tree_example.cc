/**
 * @file fenwick_tree_example.cc
 * @brief Fenwick trees applied to trading and sports betting
 *
 * This example demonstrates the three Fenwick tree variants through
 * realistic scenarios from quantitative finance and betting exchanges.
 *
 * SCENARIO 1 — Order Book Depth (Fenwick_Tree + find_kth)
 * ========================================================
 * A stock exchange order book tracks how many shares are available to
 * buy at each price tick.  A trader placing a market order of size K
 * needs to know: "what is the worst price I will pay?"  That is
 * exactly find_kth(K) — the lowest price tick whose cumulative volume
 * reaches K.
 *
 * SCENARIO 2 — Intraday P&L Dashboard (Gen_Fenwick_Tree)
 * ========================================================
 * A trading desk records the profit/loss of each executed trade by
 * the minute-bucket in which it occurred.  The risk dashboard needs
 * real-time answers to "what was the P&L between 10:15 and 11:30?"
 * — a range query that the classic Fenwick tree answers in O(log n).
 *
 * SCENARIO 3 — Betting Exchange Dividends (Range_Fenwick_Tree)
 * =============================================================
 * A betting exchange distributes daily dividends to participants.
 * Each promotion covers a contiguous span of days and adds a fixed
 * bonus per day.  Promotions overlap freely.  The accounting team
 * needs both "total paid on day d" and "total cost over days [l, r]".
 * This is the textbook use-case for range-update / range-query.
 *
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o fenwick_tree_example fenwick_tree_example.cc
 * ./fenwick_tree_example
 */

# include <iostream>
# include <iomanip>
# include <string>

# include <tpl_fenwick_tree.H>

using namespace std;
using namespace Aleph;

// =====================================================================
// Helper: print a horizontal bar of width proportional to value
// =====================================================================
static void bar(int val, int scale = 1)
{
  int w = val / scale;
  for (int i = 0; i < w; ++i)
    cout << '#';
}

// =====================================================================
// SCENARIO 1 — Order Book Depth
// =====================================================================
//
// Imagine a simplified stock listed with price ticks 0..19 (each tick
// represents $100.00 + tick * $0.01, so tick 0 = $100.00, tick 19 =
// $100.19).
//
// The "bid side" of the order book tells us how many shares people are
// willing to BUY at each tick.  When someone sells at market, the
// exchange fills the order starting at the highest bid, consuming
// volume downward.
//
// But here we model the "ask side" (offers to sell) from the lowest
// price upward, because that is the natural direction for a buyer
// placing a market order:
//
//   tick 0  ($100.00): 120 shares
//   tick 1  ($100.01):  80 shares
//   tick 3  ($100.03): 200 shares
//   ...
//
// A market BUY order of 250 shares will sweep:
//   120 from tick 0 + 80 from tick 1 + 50 from tick 3 = 250
// The worst price paid is tick 3 ($100.03).
//
// find_kth(250) gives us that answer in O(log n).
//
static void scenario_order_book()
{
  cout << "============================================================\n"
       << " SCENARIO 1: Order Book Depth (Fenwick_Tree + find_kth)\n"
       << "============================================================\n\n";

  const size_t TICKS = 20; // price ticks 0..19
  Fenwick_Tree<int> ask_book(TICKS);

  // -- Limit orders arrive on the ask side --
  cout << "Limit SELL orders arrive:\n";

  struct Order { size_t tick; int shares; const char *label; };
  Order orders[] = {
    { 0, 120, "$100.00"},
    { 1,  80, "$100.01"},
    { 3, 200, "$100.03"},
    { 4,  50, "$100.04"},
    { 7, 300, "$100.07"},
    {10, 150, "$100.10"},
    {15, 400, "$100.15"},
  };

  for (auto & o : orders)
    {
      ask_book.update(o.tick, o.shares);
      cout << "  " << setw(7) << o.label
           << "  +" << setw(4) << o.shares << " shares\n";
    }

  cout << "\nOrder book (ask side):\n\n";
  cout << "  Tick  Price     Volume  Cumulative\n"
       << "  ----  --------  ------  ----------\n";

  for (size_t t = 0; t < TICKS; ++t)
    {
      int vol = ask_book.get(t);
      if (vol == 0)
        continue;
      int cum = ask_book.prefix(t);
      cout << "  " << setw(4) << t
           << "  $" << fixed << setprecision(2) << (100.0 + t * 0.01)
           << "  " << setw(6) << vol
           << "  " << setw(10) << cum << "  ";
      bar(vol, 10);
      cout << "\n";
    }

  // -- A trader places a market BUY order --
  int buy_sizes[] = {100, 250, 500, 1000, 1500};

  cout << "\nMarket BUY order fill simulation:\n\n";
  cout << "  Order Size  Worst Tick  Worst Price\n"
       << "  ----------  ----------  -----------\n";

  for (int sz : buy_sizes)
    {
      size_t worst_tick = ask_book.find_kth(sz);
      if (worst_tick >= TICKS)
        cout << "  " << setw(10) << sz << "  INSUFFICIENT LIQUIDITY\n";
      else
        cout << "  " << setw(10) << sz
             << "  " << setw(10) << worst_tick
             << "  $" << fixed << setprecision(2)
             << (100.0 + worst_tick * 0.01) << "\n";
    }

  // -- An order gets cancelled, book updates --
  cout << "\n>> Cancel 150 shares at tick 3 ($100.03)\n";
  ask_book.update(3, -150);

  cout << ">> New fill for 250 shares: ";
  size_t w = ask_book.find_kth(250);
  cout << "worst price = $" << fixed << setprecision(2)
       << (100.0 + w * 0.01)
       << " (tick " << w << ")\n";

  int total_liquidity = ask_book.prefix(TICKS - 1);
  cout << "\nTotal ask liquidity: " << total_liquidity << " shares\n";
}

// =====================================================================
// SCENARIO 2 — Intraday P&L Dashboard
// =====================================================================
//
// A quantitative trading desk executes hundreds of trades per day.
// Each trade's realized P&L is booked into a minute-bucket (0 = 09:30,
// 1 = 09:31, ..., 389 = 16:00 — the 390 minutes of a US trading day).
//
// The risk dashboard needs instant answers to:
//   "What was the cumulative P&L from market open to now?"
//   "What was the P&L between the Fed announcement (14:00 = min 270)
//    and the close (16:00 = min 389)?"
//
static void scenario_pnl_dashboard()
{
  cout << "\n\n============================================================\n"
       << " SCENARIO 2: Intraday P&L Dashboard (Gen_Fenwick_Tree)\n"
       << "============================================================\n\n";

  const size_t MINUTES = 390; // 09:30 to 16:00
  Gen_Fenwick_Tree<double> pnl(MINUTES);

  // -- Simulated trades throughout the day --
  struct Trade { size_t minute; double pnl; const char *event; };
  Trade trades[] = {
    {   0,  1200.0, "Open: initial scalp profit"          },
    {   5,  -300.0, "Stop-loss hit on AAPL"                },
    {  30,  4500.0, "NVDA earnings beat — long pays off"   },
    {  31,  2200.0, "Follow-through momentum"              },
    {  60,  -800.0, "Mean reversion loss"                  },
    { 120, -1500.0, "Lunch hour chop"                      },
    { 180,  3000.0, "Afternoon trend resumes"              },
    { 270,  8000.0, "Fed holds rates — massive rally"      },
    { 271,  5000.0, "Fed follow-through"                   },
    { 330, -2000.0, "Profit taking"                        },
    { 389,  1500.0, "MOC imbalance capture"                },
  };

  auto min_to_time = [](size_t m) -> string
  {
    int h = 9 + (int)(m + 30) / 60;
    int mm = ((int)m + 30) % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", h, mm);
    return buf;
  };

  cout << "Trades booked:\n\n";
  cout << "  Time   Minute  P&L         Event\n"
       << "  -----  ------  ----------  ----------------------------\n";

  for (auto & t : trades)
    {
      pnl.update(t.minute, t.pnl);
      cout << "  " << min_to_time(t.minute)
           << "  " << setw(6) << t.minute
           << "  " << setw(10) << fixed << setprecision(2) << t.pnl
           << "  " << t.event << "\n";
    }

  // -- Dashboard queries --
  cout << "\nDashboard queries:\n\n";

  double open_to_lunch = pnl.prefix(179);
  double fed_to_close  = pnl.query(270, 389);
  double total_day     = pnl.prefix(389);
  double morning       = pnl.query(0, 119);   // 09:30 — 11:30
  double afternoon     = pnl.query(120, 389);  // 11:30 — 16:00

  cout << "  Open to lunch  (09:30-12:30):  $" << setw(10) << open_to_lunch << "\n"
       << "  Fed to close   (14:00-16:00):  $" << setw(10) << fed_to_close  << "\n"
       << "  Morning session (09:30-11:30): $" << setw(10) << morning       << "\n"
       << "  Afternoon       (11:30-16:00): $" << setw(10) << afternoon     << "\n"
       << "  ----------------------------------------\n"
       << "  Total day P&L:                 $" << setw(10) << total_day     << "\n";

  // -- Late correction: a trade was re-priced --
  cout << "\n>> Correction: NVDA trade at 10:00 re-priced from "
       << "$4500 to $4000\n";
  pnl.update(30, -500.0);
  cout << "   Adjusted total day P&L: $"
       << fixed << setprecision(2) << pnl.prefix(389) << "\n";
}

// =====================================================================
// SCENARIO 3 — Betting Exchange Promotions
// =====================================================================
//
// A sports betting exchange runs daily promotions to attract users.
// Each promotion adds a fixed bonus (in cents) to every participant
// on each day the promotion is active.  Promotions overlap freely:
//
//   Promo A: days 0-6   (first week),   +50c/day
//   Promo B: days 3-9   (overlap),      +30c/day
//   Promo C: days 5-5   (single day),  +100c/day
//   Promo D: days 0-13  (full period),  +10c/day
//
// The accounting team needs:
//   - How much was paid to each user on day d?
//   - Total payout over any date range?
//
// This is a textbook range-update / range-query problem.
//
static void scenario_betting_promos()
{
  cout << "\n\n============================================================\n"
       << " SCENARIO 3: Betting Exchange Promotions (Range_Fenwick_Tree)\n"
       << "============================================================\n\n";

  const size_t DAYS = 14; // two weeks, days 0..13
  Range_Fenwick_Tree<int> payouts(DAYS); // cents per user per day

  struct Promo { size_t from; size_t to; int bonus; const char *name; };
  Promo promos[] = {
    {  0,  6,  50, "Welcome Week"      },
    {  3,  9,  30, "Midweek Boost"     },
    {  5,  5, 100, "Super Saturday"    },
    {  0, 13,  10, "Loyalty Baseline"  },
  };

  cout << "Promotions applied:\n\n";
  cout << "  Promotion         Days       Bonus/day\n"
       << "  ----------------  ---------  ---------\n";

  for (auto & p : promos)
    {
      payouts.update(p.from, p.to, p.bonus);
      cout << "  " << setw(16) << left << p.name << right
           << "  " << setw(2) << p.from << " - " << setw(2) << p.to
           << "    " << setw(5) << p.bonus << "c\n";
    }

  // -- Daily breakdown --
  const char *day_names[] = {
    "Mon W1", "Tue W1", "Wed W1", "Thu W1", "Fri W1", "Sat W1", "Sun W1",
    "Mon W2", "Tue W2", "Wed W2", "Thu W2", "Fri W2", "Sat W2", "Sun W2"
  };

  cout << "\nDaily payout per user:\n\n"
       << "  Day  Name     Cents  Breakdown\n"
       << "  ---  -------  -----  ---------\n";

  for (size_t d = 0; d < DAYS; ++d)
    {
      int cents = payouts.get(d);
      cout << "  " << setw(3) << d
           << "  " << day_names[d]
           << "  " << setw(5) << cents << "  ";
      bar(cents, 5);
      cout << "\n";
    }

  // -- Accounting queries --
  cout << "\nAccounting queries:\n\n";

  int week1 = payouts.query(0, 6);
  int week2 = payouts.query(7, 13);
  int total = payouts.prefix(13);
  int peak_weekend = payouts.query(4, 6); // Fri-Sun of week 1

  cout << "  Week 1 total (days 0-6):   " << setw(5) << week1  << "c\n"
       << "  Week 2 total (days 7-13):  " << setw(5) << week2  << "c\n"
       << "  Peak weekend (Fri-Sun W1): " << setw(5) << peak_weekend << "c\n"
       << "  ----------------------------------\n"
       << "  Grand total (14 days):     " << setw(5) << total  << "c\n";

  // -- New promotion added retroactively --
  cout << "\n>> Retroactive adjustment: 'Apology Bonus' +20c on days 10-13\n";
  payouts.update(10, 13, 20);

  cout << "   Adjusted week 2 total: " << payouts.query(7, 13) << "c\n"
       << "   Adjusted grand total:  " << payouts.prefix(13)   << "c\n";

  // -- Per-user cost if exchange has 10,000 users --
  int users = 10000;
  cout << "\n>> With " << users << " users:\n"
       << "   Total 14-day cost: $"
       << fixed << setprecision(2)
       << (payouts.prefix(13) * users / 100.0) << "\n";
}

// =====================================================================

int main()
{
  scenario_order_book();
  scenario_pnl_dashboard();
  scenario_betting_promos();

  cout << "\n";
  return 0;
}