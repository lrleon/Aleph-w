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
 * @file streaming_demo.cc
 * @brief Demonstration of probabilistic streaming algorithms in Aleph-w.
 */

# include <iostream>
# include <iomanip>
# include <string>

# include <reservoir-sampling.H>
# include <count-min-sketch.H>
# include <hyperloglog.H>
# include <minhash.H>
# include <print_rule.H>
# include <tpl_dynList.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "\n=== Probabilistic Streaming Algorithms Demo ===\n\n";

  // Simulation data: a stream of words with many repetitions
  DynList<string> stream = {
    "apple", "banana", "apple", "cherry", "banana", "apple", "date", 
    "banana", "elderberry", "fig", "apple", "banana", "cherry", "apple"
  };
  
  // Extend stream to make it "large"
  for (int i = 0; i < 1000; ++i)
    stream.append("word_" + to_string(i % 100)); // 100 unique generated words

  cout << "Stream size: " << stream.size() << " elements.\n\n";

  // 1. Reservoir Sampling
  {
    cout << "[1] Reservoir Sampling (picking 5 random elements from stream)\n";
    print_rule();
    auto sample = reservoir_sample(stream.begin(), stream.end(), 5, 42);
    cout << "Sample: ";
    for (size_t i = 0; i < sample.size(); ++i)
      cout << sample[i] << (i == sample.size() - 1 ? "" : ", ");
    cout << "\n\n";
  }

  // 2. Count-Min Sketch
  {
    cout << "[2] Count-Min Sketch (frequency estimation)\n";
    print_rule();
    auto cms = Count_Min_Sketch<string>::from_error_bounds(0.01, 0.01);
    for (const auto & s : stream) cms.update(s);

    cout << "Estimated frequency of 'apple' : " << cms.estimate("apple") << " (Actual: 5)\n";
    cout << "Estimated frequency of 'banana': " << cms.estimate("banana") << " (Actual: 4)\n";
    cout << "Estimated frequency of 'word_0': " << cms.estimate("word_0") << " (Actual: 10)\n";
    cout << "\n";
  }

  // 3. HyperLogLog
  {
    cout << "[3] HyperLogLog (cardinality estimation)\n";
    print_rule();
    HyperLogLog<string> hll(10); // 1024 registers
    for (const auto & s : stream) hll.update(s);

    // Total unique: 6 (initial) + 100 (generated) = 106
    cout << "Estimated unique elements: " << fixed << setprecision(1) << hll.estimate() << " (Actual: 106)\n";
    cout << "\n";
  }

  // 4. MinHash
  {
    cout << "[4] MinHash (set similarity estimation)\n";
    print_rule();
    MinHash<string> mh1(128), mh2(128);
    
    DynList<string> set1 = {"cat", "dog", "bird", "fish", "hamster"};
    DynList<string> set2 = {"dog", "cat", "lion", "tiger", "bird"};
    // Intersection: {cat, dog, bird} (3)
    // Union: {cat, dog, bird, fish, hamster, lion, tiger} (7)
    // Jaccard: 3/7 = 0.428
    
    mh1.update(set1.begin(), set1.end());
    mh2.update(set2.begin(), set2.end());

    cout << "Estimated Jaccard similarity: " << mh1.similarity(mh2) << "\n";
    cout << "Actual Jaccard similarity   : " << 3.0/7.0 << "\n";
  }

  cout << "\nDone.\n";
  return 0;
}
