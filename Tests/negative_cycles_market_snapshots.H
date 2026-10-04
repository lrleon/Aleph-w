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
 * @file negative_cycles_market_snapshots.H
 * @brief Top-of-book quotes of real Binance spot markets, as a fixture for the
 *        cycle searches of `Negative_Cycles.H`, `Min_Mean_Cycle.H` and
 *        `Howard_Min_Mean_Cycle.H`.
 *
 * The quotes come from the best bid and ask stream (`bookTicker`) of six
 * assets (BTC, ETH, SOL, BNB, USDT, USDC) and their fifteen pairs, captured
 * for 24 hours from 2026-10-02 00:31 UTC. About 4.4 million updates were
 * replayed through the searches; these snapshots were chosen by criteria, not
 * at random:
 *
 * - `first` and `typical-1` .. `typical-8`: the first complete state and eight
 *   spread over the day, where nothing is profitable once fees are charged;
 * - `len3` .. `len6`: the first state whose most profitable fee-free cycle
 *   has that many legs;
 * - `raw-peak`: the largest raw profit with a fee of 0.075 % per leg, made of
 *   stale quotes;
 * - `near-zero`: the best cycle closest to break-even with a fee of 0.1 %;
 * - `bounded-miss`: a state where the bounded search with `L = 6`, a relaxed
 *   problem, does not return the best cycle;
 * - `burst-1` .. `burst-3`: the peak of each of the three episodes of the day
 *   in which every leg was at most one second old and its coverage observed,
 *   with a fee of 0.075 % per leg. They fall within seven seconds of each
 *   other.
 *
 * Only prices are kept: quantities, timestamps and connection identifiers are
 * left out. A price is `bid / 10^exponent` (or `ask / 10^exponent`). For each
 * quote, `age_ms` is the time since its last update when the snapshot was
 * taken, and `observed` tells whether its coverage was confirmed; together
 * they let a test hide the stale arcs with the arc filter of the searches, as
 * an application would.
 */

# ifndef NEGATIVE_CYCLES_MARKET_SNAPSHOTS_H
# define NEGATIVE_CYCLES_MARKET_SNAPSHOTS_H

# include <cstddef>

namespace Negative_Cycles_Market_Data
{
  /// Number of assets of the market: BTC, ETH, SOL, BNB, USDT and USDC.
  inline constexpr int asset_count = 6;

  /// Number of pairs, one for each pair of assets.
  inline constexpr int pair_count = 15;

  /// Names of the assets, indexed by `Market_Quote::base` and `Market_Quote::quote`.
  inline constexpr const char * const asset_names[asset_count] = {"BTC", "ETH", "SOL", "BNB", "USDT", "USDC"};

  /// Best bid and ask of one pair, with the price scale of both.
  struct Market_Quote
  {
    int base;           ///< Index of the base asset: what the bid buys and the ask sells.
    int quote;          ///< Index of the quote asset: what prices are expressed in.
    long long bid;      ///< Best bid, in units of `10^-exponent`.
    long long ask;      ///< Best ask, in units of `10^-exponent`.
    int exponent;       ///< Decimal places of the prices.
    long long age_ms;   ///< Milliseconds since the last update of this pair.
    int observed;       ///< Whether the coverage of the stream was confirmed (1) or not (0).
  };

  /// State of the whole market at one instant.
  struct Market_Snapshot
  {
    const char * label;               ///< Why it was chosen (see the file documentation).
    long long offset_ms;              ///< Milliseconds since the first complete state.
    Market_Quote quotes[pair_count];  ///< One quote for each pair, ordered by base and quote.
  };

  /// The snapshots, in chronological order.
  inline const Market_Snapshot snapshots[] = {
  {"first", 0,
   {
    {0, 4, 8476400, 8476401, 2, 10, 0},  // BTCUSDT
    {0, 5, 8474775, 8474776, 2, 528, 0},  // BTCUSDC
    {1, 0, 3190, 3191, 5, 3262, 0},  // ETHBTC
    {1, 4, 270418, 270419, 2, 1420, 0},  // ETHUSDT
    {1, 5, 270322, 270323, 2, 1764, 0},  // ETHUSDC
    {2, 0, 13996, 13997, 7, 752, 0},  // SOLBTC
    {2, 1, 4387, 4388, 5, 10026, 0},  // SOLETH
    {2, 3, 1537, 1538, 4, 0, 0},  // SOLBNB
    {2, 4, 11865, 11866, 2, 762, 0},  // SOLUSDT
    {2, 5, 11861, 11862, 2, 802, 0},  // SOLUSDC
    {3, 0, 9102, 9103, 6, 2426, 0},  // BNBBTC
    {3, 1, 2853, 2854, 4, 7902, 0},  // BNBETH
    {3, 4, 77165, 77166, 2, 1908, 0},  // BNBUSDT
    {3, 5, 77138, 77139, 2, 1918, 0},  // BNBUSDC
    {5, 4, 100030, 100031, 5, 712, 0},  // USDCUSDT
   }},
  {"len5", 13485,
   {
    {0, 4, 8476963, 8476964, 2, 453, 1},  // BTCUSDT
    {0, 5, 8475232, 8475233, 2, 109, 1},  // BTCUSDC
    {1, 0, 3190, 3191, 5, 1932, 1},  // ETHBTC
    {1, 4, 270416, 270417, 2, 0, 1},  // ETHUSDT
    {1, 5, 270318, 270319, 2, 70, 1},  // ETHUSDC
    {2, 0, 13996, 13997, 7, 2698, 1},  // SOLBTC
    {2, 1, 4387, 4388, 5, 3627, 1},  // SOLETH
    {2, 3, 1537, 1538, 4, 10820, 1},  // SOLBNB
    {2, 4, 11866, 11867, 2, 1849, 1},  // SOLUSDT
    {2, 5, 11862, 11863, 2, 1859, 1},  // SOLUSDC
    {3, 0, 9102, 9103, 6, 907, 1},  // BNBBTC
    {3, 1, 2853, 2854, 4, 3174, 1},  // BNBETH
    {3, 4, 77166, 77167, 2, 19, 1},  // BNBUSDT
    {3, 5, 77139, 77140, 2, 463, 1},  // BNBUSDC
    {5, 4, 100030, 100031, 5, 3979, 1},  // USDCUSDT
   }},
  {"len3", 25312,
   {
    {0, 4, 8476790, 8476792, 2, 20, 1},  // BTCUSDT
    {0, 5, 8474134, 8474308, 2, 0, 1},  // BTCUSDC
    {1, 0, 3189, 3190, 5, 2296, 1},  // ETHBTC
    {1, 4, 270377, 270378, 2, 424, 1},  // ETHUSDT
    {1, 5, 270276, 270277, 2, 72, 1},  // ETHUSDC
    {2, 0, 13994, 13995, 7, 1884, 1},  // SOLBTC
    {2, 1, 4387, 4388, 5, 1048, 1},  // SOLETH
    {2, 3, 1537, 1538, 4, 22646, 1},  // SOLBNB
    {2, 4, 11863, 11864, 2, 152, 1},  // SOLUSDT
    {2, 5, 11859, 11860, 2, 142, 1},  // SOLUSDC
    {3, 0, 9102, 9103, 6, 536, 1},  // BNBBTC
    {3, 1, 2853, 2854, 4, 9760, 1},  // BNBETH
    {3, 4, 77162, 77163, 2, 597, 1},  // BNBUSDT
    {3, 5, 77128, 77135, 2, 214, 1},  // BNBUSDC
    {5, 4, 100030, 100031, 5, 224, 1},  // USDCUSDT
   }},
  {"len6", 75304,
   {
    {0, 4, 8474872, 8474873, 2, 40, 1},  // BTCUSDT
    {0, 5, 8472701, 8472702, 2, 392, 1},  // BTCUSDC
    {1, 0, 3189, 3190, 5, 362, 1},  // ETHBTC
    {1, 4, 270318, 270319, 2, 230, 1},  // ETHUSDT
    {1, 5, 270232, 270233, 2, 210, 1},  // ETHUSDC
    {2, 0, 13991, 13992, 7, 371, 1},  // SOLBTC
    {2, 1, 4387, 4388, 5, 0, 1},  // SOLETH
    {2, 3, 1537, 1538, 4, 10270, 1},  // SOLBNB
    {2, 4, 11859, 11860, 2, 30, 1},  // SOLUSDT
    {2, 5, 11855, 11856, 2, 330, 1},  // SOLUSDC
    {3, 0, 9102, 9103, 6, 1222, 1},  // BNBBTC
    {3, 1, 2853, 2854, 4, 59752, 1},  // BNBETH
    {3, 4, 77144, 77145, 2, 70, 1},  // BNBUSDT
    {3, 5, 77117, 77118, 2, 20, 1},  // BNBUSDC
    {5, 4, 100030, 100031, 5, 126, 1},  // USDCUSDT
   }},
  {"typical-1", 8288112,
   {
    {0, 4, 8506400, 8506401, 2, 642, 1},  // BTCUSDT
    {0, 5, 8505036, 8505037, 2, 831, 1},  // BTCUSDC
    {1, 0, 3194, 3195, 5, 461, 1},  // ETHBTC
    {1, 4, 271775, 271776, 2, 141, 1},  // ETHUSDT
    {1, 5, 271669, 271670, 2, 0, 1},  // ETHUSDC
    {2, 0, 14114, 14115, 7, 8877, 0},  // SOLBTC
    {2, 1, 4418, 4419, 5, 8142, 0},  // SOLETH
    {2, 3, 1552, 1553, 4, 5607, 0},  // SOLBNB
    {2, 4, 12007, 12008, 2, 1211, 1},  // SOLUSDT
    {2, 5, 12004, 12005, 2, 701, 1},  // SOLUSDC
    {3, 0, 9089, 9091, 6, 2199, 1},  // BNBBTC
    {3, 1, 2846, 2847, 4, 4063, 0},  // BNBETH
    {3, 4, 77336, 77337, 2, 287, 1},  // BNBUSDT
    {3, 5, 77306, 77307, 2, 9, 1},  // BNBUSDC
    {5, 4, 100036, 100037, 5, 811, 1},  // USDCUSDT
   }},
  {"raw-peak", 13818442,
   {
    {0, 4, 8638842, 8638989, 2, 157, 0},  // BTCUSDT
    {0, 5, 8636178, 8636920, 2, 116, 0},  // BTCUSDC
    {1, 0, 3170, 3171, 5, 487, 0},  // ETHBTC
    {1, 4, 273912, 273913, 2, 137, 0},  // ETHUSDT
    {1, 5, 273832, 273860, 2, 20, 0},  // ETHUSDC
    {2, 0, 14131, 14132, 7, 6732, 1},  // SOLBTC
    {2, 1, 4451, 4452, 5, 79904, 1},  // SOLETH
    {2, 3, 1560, 1561, 4, 5692, 1},  // SOLBNB
    {2, 4, 12207, 12208, 2, 0, 0},  // SOLUSDT
    {2, 5, 12099, 12100, 2, 6483, 1},  // SOLUSDC
    {3, 0, 9056, 9057, 6, 14173, 1},  // BNBBTC
    {3, 1, 2853, 2854, 4, 15028, 1},  // BNBETH
    {3, 4, 77549, 77550, 2, 4109, 1},  // BNBUSDT
    {3, 5, 77801, 77802, 2, 612, 0},  // BNBUSDC
    {5, 4, 100029, 100030, 5, 840, 0},  // USDCUSDT
   }},
  {"bounded-miss", 14112197,
   {
    {0, 4, 8653634, 8653635, 2, 22, 0},  // BTCUSDT
    {0, 5, 8650344, 8650345, 2, 41, 0},  // BTCUSDC
    {1, 0, 3168, 3169, 5, 3802, 1},  // ETHBTC
    {1, 4, 273715, 273716, 2, 60, 0},  // ETHUSDT
    {1, 5, 273409, 273410, 2, 6148, 1},  // ETHUSDC
    {2, 0, 14101, 14102, 7, 9801, 1},  // SOLBTC
    {2, 1, 4450, 4451, 5, 9836, 1},  // SOLETH
    {2, 3, 1563, 1564, 4, 11822, 1},  // SOLBNB
    {2, 4, 12208, 12209, 2, 0, 0},  // SOLUSDT
    {2, 5, 12204, 12205, 2, 135, 0},  // SOLUSDC
    {3, 0, 9016, 9018, 6, 20744, 1},  // BNBBTC
    {3, 1, 2846, 2847, 4, 141896, 1},  // BNBETH
    {3, 4, 77840, 77841, 2, 5143, 1},  // BNBUSDT
    {3, 5, 77814, 77815, 2, 4428, 1},  // BNBUSDC
    {5, 4, 100028, 100029, 5, 3911, 1},  // USDCUSDT
   }},
  {"typical-2", 17373782,
   {
    {0, 4, 8646461, 8646462, 2, 524, 1},  // BTCUSDT
    {0, 5, 8643342, 8643343, 2, 291, 1},  // BTCUSDC
    {1, 0, 3158, 3159, 5, 234, 1},  // ETHBTC
    {1, 4, 273060, 273061, 2, 97, 1},  // ETHUSDT
    {1, 5, 272972, 272973, 2, 0, 1},  // ETHUSDC
    {2, 0, 14210, 14211, 7, 5877, 1},  // SOLBTC
    {2, 1, 4498, 4500, 5, 3447, 1},  // SOLETH
    {2, 3, 1574, 1575, 4, 28569, 1},  // SOLBNB
    {2, 4, 12285, 12286, 2, 406, 1},  // SOLUSDT
    {2, 5, 12282, 12283, 2, 1208, 1},  // SOLUSDC
    {3, 0, 9026, 9027, 6, 30421, 1},  // BNBBTC
    {3, 1, 2857, 2858, 4, 7457, 1},  // BNBETH
    {3, 4, 78043, 78044, 2, 658, 1},  // BNBUSDT
    {3, 5, 78016, 78017, 2, 2676, 1},  // BNBUSDC
    {5, 4, 100031, 100032, 5, 1507, 1},  // USDCUSDT
   }},
  {"typical-3", 26582897,
   {
    {0, 4, 8591139, 8591140, 2, 0, 0},  // BTCUSDT
    {0, 5, 8589981, 8589982, 2, 1733, 0},  // BTCUSDC
    {1, 0, 3178, 3179, 5, 25754, 0},  // ETHBTC
    {1, 4, 273012, 273013, 2, 577, 0},  // ETHUSDT
    {1, 5, 272964, 272965, 2, 539, 0},  // ETHUSDC
    {2, 0, 14117, 14118, 7, 7242, 0},  // SOLBTC
    {2, 1, 4441, 4443, 5, 74602, 1},  // SOLETH
    {2, 3, 1564, 1565, 4, 74718, 1},  // SOLBNB
    {2, 4, 12129, 12130, 2, 4317, 0},  // SOLUSDT
    {2, 5, 12126, 12127, 2, 11494, 0},  // SOLUSDC
    {3, 0, 9027, 9028, 6, 34942, 0},  // BNBBTC
    {3, 1, 2839, 2841, 4, 33979, 0},  // BNBETH
    {3, 4, 77540, 77541, 2, 499, 0},  // BNBUSDT
    {3, 5, 77530, 77531, 2, 39, 0},  // BNBUSDC
    {5, 4, 100019, 100021, 5, 1219, 0},  // USDCUSDT
   }},
  {"typical-4", 35735846,
   {
    {0, 4, 8624998, 8624999, 2, 73, 1},  // BTCUSDT
    {0, 5, 8625502, 8625503, 2, 229, 1},  // BTCUSDC
    {1, 0, 3176, 3177, 5, 3313, 1},  // ETHBTC
    {1, 4, 273919, 273920, 2, 1050, 1},  // ETHUSDT
    {1, 5, 273893, 273894, 2, 991, 1},  // ETHUSDC
    {2, 0, 14090, 14091, 7, 39, 1},  // SOLBTC
    {2, 1, 4436, 4437, 5, 438, 1},  // SOLETH
    {2, 3, 1566, 1567, 4, 1499, 1},  // SOLBNB
    {2, 4, 12154, 12155, 2, 0, 1},  // SOLUSDT
    {2, 5, 12152, 12153, 2, 19, 1},  // SOLUSDC
    {3, 0, 8995, 8996, 6, 11160, 1},  // BNBBTC
    {3, 1, 2832, 2833, 4, 20835, 1},  // BNBETH
    {3, 4, 77585, 77586, 2, 4127, 1},  // BNBUSDT
    {3, 5, 77574, 77575, 2, 2375, 1},  // BNBUSDC
    {5, 4, 100015, 100016, 5, 1541, 1},  // USDCUSDT
   }},
  {"typical-5", 45448170,
   {
    {0, 4, 8675000, 8675001, 2, 0, 1},  // BTCUSDT
    {0, 5, 8673161, 8673162, 2, 1377, 1},  // BTCUSDC
    {1, 0, 3173, 3174, 5, 397, 1},  // ETHBTC
    {1, 4, 275247, 275248, 2, 10, 1},  // ETHUSDT
    {1, 5, 275240, 275241, 2, 225, 1},  // ETHUSDC
    {2, 0, 14121, 14122, 7, 1027, 1},  // SOLBTC
    {2, 1, 4451, 4452, 5, 1249, 1},  // SOLETH
    {2, 3, 1567, 1568, 4, 2093, 1},  // SOLBNB
    {2, 4, 12251, 12252, 2, 257, 1},  // SOLUSDT
    {2, 5, 12248, 12249, 2, 773, 1},  // SOLUSDC
    {3, 0, 9007, 9008, 6, 7319, 0},  // BNBBTC
    {3, 1, 2838, 2839, 4, 871, 1},  // BNBETH
    {3, 4, 78166, 78167, 2, 43, 1},  // BNBUSDT
    {3, 5, 78143, 78144, 2, 3561, 1},  // BNBUSDC
    {5, 4, 100010, 100011, 5, 55, 1},  // USDCUSDT
   }},
  {"near-zero", 49618748,
   {
    {0, 4, 8626155, 8626156, 2, 3267, 1},  // BTCUSDT
    {0, 5, 8626327, 8626328, 2, 3771, 1},  // BTCUSDC
    {1, 0, 3167, 3168, 5, 0, 0},  // ETHBTC
    {1, 4, 274098, 274099, 2, 19, 0},  // ETHUSDT
    {1, 5, 273321, 273322, 2, 3167, 1},  // ETHUSDC
    {2, 0, 14104, 14105, 7, 4345, 1},  // SOLBTC
    {2, 1, 4451, 4452, 5, 5268, 1},  // SOLETH
    {2, 3, 1560, 1562, 4, 38685, 0},  // SOLBNB
    {2, 4, 12167, 12168, 2, 3420, 1},  // SOLUSDT
    {2, 5, 12165, 12166, 2, 3543, 1},  // SOLUSDC
    {3, 0, 9034, 9035, 6, 7741, 0},  // BNBBTC
    {3, 1, 2851, 2852, 4, 16803, 0},  // BNBETH
    {3, 4, 77944, 77945, 2, 3690, 1},  // BNBUSDT
    {3, 5, 77938, 77940, 2, 4670, 1},  // BNBUSDC
    {5, 4, 100005, 100006, 5, 3207, 1},  // USDCUSDT
   }},
  {"typical-6", 55264020,
   {
    {0, 4, 8536200, 8536201, 2, 332, 0},  // BTCUSDT
    {0, 5, 8534027, 8534028, 2, 132, 0},  // BTCUSDC
    {1, 0, 3159, 3160, 5, 4408, 1},  // ETHBTC
    {1, 4, 269712, 269713, 2, 273, 0},  // ETHUSDT
    {1, 5, 269676, 269677, 2, 77, 0},  // ETHUSDC
    {2, 0, 14060, 14061, 7, 14271, 1},  // SOLBTC
    {2, 1, 4449, 4451, 5, 13159, 1},  // SOLETH
    {2, 3, 1551, 1553, 4, 50367, 1},  // SOLBNB
    {2, 4, 12008, 12009, 2, 3943, 1},  // SOLUSDT
    {2, 5, 12006, 12007, 2, 4510, 1},  // SOLUSDC
    {3, 0, 9058, 9060, 6, 505, 0},  // BNBBTC
    {3, 1, 2867, 2868, 4, 3844, 1},  // BNBETH
    {3, 4, 77374, 77375, 2, 7904, 1},  // BNBUSDT
    {3, 5, 77314, 77318, 2, 0, 0},  // BNBUSDC
    {5, 4, 100016, 100017, 5, 5847, 1},  // USDCUSDT
   }},
  {"typical-7", 65488419,
   {
    {0, 4, 8442000, 8442001, 2, 40, 1},  // BTCUSDT
    {0, 5, 8440666, 8440667, 2, 0, 1},  // BTCUSDC
    {1, 0, 3169, 3170, 5, 260, 1},  // ETHBTC
    {1, 4, 267518, 267519, 2, 20, 1},  // ETHUSDT
    {1, 5, 267479, 267480, 2, 1010, 1},  // ETHUSDC
    {2, 0, 14028, 14029, 7, 1715, 1},  // SOLBTC
    {2, 1, 4427, 4428, 5, 12190, 1},  // SOLETH
    {2, 3, 1544, 1545, 4, 3443, 1},  // SOLBNB
    {2, 4, 11840, 11841, 2, 1779, 1},  // SOLUSDT
    {2, 5, 11840, 11841, 2, 10278, 1},  // SOLUSDC
    {3, 0, 9082, 9084, 6, 3726, 1},  // BNBBTC
    {3, 1, 2866, 2867, 4, 7010, 1},  // BNBETH
    {3, 4, 76675, 76676, 2, 6802, 1},  // BNBUSDT
    {3, 5, 76667, 76668, 2, 1382, 1},  // BNBUSDC
    {5, 4, 100012, 100013, 5, 242, 1},  // USDCUSDT
   }},
  {"burst-1", 65503410,
   {
    {0, 4, 8441600, 8441601, 2, 147, 1},  // BTCUSDT
    {0, 5, 8439917, 8440358, 2, 109, 1},  // BTCUSDC
    {1, 0, 3169, 3170, 5, 896, 1},  // ETHBTC
    {1, 4, 267514, 267515, 2, 12, 1},  // ETHUSDT
    {1, 5, 267450, 267451, 2, 123, 1},  // ETHUSDC
    {2, 0, 14026, 14027, 7, 2818, 1},  // SOLBTC
    {2, 1, 4426, 4427, 5, 1817, 1},  // SOLETH
    {2, 3, 1544, 1545, 4, 18434, 1},  // SOLBNB
    {2, 4, 11838, 11839, 2, 2598, 1},  // SOLUSDT
    {2, 5, 11836, 11837, 2, 5309, 1},  // SOLUSDC
    {3, 0, 9082, 9084, 6, 425, 1},  // BNBBTC
    {3, 1, 2866, 2867, 4, 14893, 1},  // BNBETH
    {3, 4, 76320, 76321, 2, 0, 1},  // BNBUSDT
    {3, 5, 76667, 76668, 2, 2838, 1},  // BNBUSDC
    {5, 4, 100012, 100013, 5, 2682, 1},  // USDCUSDT
   }},
  {"burst-2", 65504098,
   {
    {0, 4, 8441600, 8441601, 2, 16, 1},  // BTCUSDT
    {0, 5, 8439662, 8439918, 2, 148, 1},  // BTCUSDC
    {1, 0, 3169, 3170, 5, 1584, 1},  // ETHBTC
    {1, 4, 267513, 267514, 2, 28, 1},  // ETHUSDT
    {1, 5, 267450, 267451, 2, 76, 1},  // ETHUSDC
    {2, 0, 14026, 14027, 7, 3506, 1},  // SOLBTC
    {2, 1, 4426, 4427, 5, 2506, 1},  // SOLETH
    {2, 3, 1544, 1545, 4, 19123, 1},  // SOLBNB
    {2, 4, 11838, 11839, 2, 592, 1},  // SOLUSDT
    {2, 5, 11836, 11837, 2, 5997, 1},  // SOLUSDC
    {3, 0, 9083, 9084, 6, 0, 1},  // BNBBTC
    {3, 1, 2866, 2867, 4, 15581, 1},  // BNBETH
    {3, 4, 76320, 76321, 2, 688, 1},  // BNBUSDT
    {3, 5, 76667, 76668, 2, 3526, 1},  // BNBUSDC
    {5, 4, 100012, 100013, 5, 3370, 1},  // USDCUSDT
   }},
  {"burst-3", 65506272,
   {
    {0, 4, 8441479, 8441480, 2, 24, 1},  // BTCUSDT
    {0, 5, 8416145, 8416146, 2, 11, 1},  // BTCUSDC
    {1, 0, 3162, 3163, 5, 1415, 1},  // ETHBTC
    {1, 4, 267512, 267513, 2, 123, 1},  // ETHUSDT
    {1, 5, 267450, 267451, 2, 184, 1},  // ETHUSDC
    {2, 0, 14026, 14027, 7, 47, 1},  // SOLBTC
    {2, 1, 4426, 4427, 5, 4680, 1},  // SOLETH
    {2, 3, 1544, 1545, 4, 21297, 1},  // SOLBNB
    {2, 4, 11838, 11839, 2, 898, 1},  // SOLUSDT
    {2, 5, 11836, 11837, 2, 8171, 1},  // SOLUSDC
    {3, 0, 9083, 9084, 6, 112, 1},  // BNBBTC
    {3, 1, 2866, 2867, 4, 17755, 1},  // BNBETH
    {3, 4, 76320, 76321, 2, 0, 1},  // BNBUSDT
    {3, 5, 76666, 76667, 2, 36, 1},  // BNBUSDC
    {5, 4, 100012, 100013, 5, 172, 1},  // USDCUSDT
   }},
  {"typical-8", 75778255,
   {
    {0, 4, 8442600, 8442601, 2, 38, 1},  // BTCUSDT
    {0, 5, 8441360, 8441361, 2, 123, 1},  // BTCUSDC
    {1, 0, 3147, 3148, 5, 1465, 1},  // ETHBTC
    {1, 4, 265723, 265724, 2, 0, 1},  // ETHUSDT
    {1, 5, 265702, 265703, 2, 884, 1},  // ETHUSDC
    {2, 0, 13963, 13964, 7, 13754, 1},  // SOLBTC
    {2, 1, 4435, 4436, 5, 3723, 1},  // SOLETH
    {2, 3, 1541, 1542, 4, 35448, 1},  // SOLBNB
    {2, 4, 11787, 11788, 2, 146, 1},  // SOLUSDT
    {2, 5, 11786, 11787, 2, 42675, 1},  // SOLUSDC
    {3, 0, 9060, 9061, 6, 1484, 1},  // BNBBTC
    {3, 1, 2877, 2878, 4, 23351, 1},  // BNBETH
    {3, 4, 76482, 76483, 2, 261, 1},  // BNBUSDT
    {3, 5, 76468, 76469, 2, 5893, 1},  // BNBUSDC
    {5, 4, 100012, 100013, 5, 1589, 1},  // USDCUSDT
   }},
  };

  /// Number of snapshots.
  inline constexpr std::size_t snapshot_count = sizeof(snapshots) / sizeof(snapshots[0]);
} // namespace Negative_Cycles_Market_Data

# endif // NEGATIVE_CYCLES_MARKET_SNAPSHOTS_H
