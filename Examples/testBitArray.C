
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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

# include <fstream>
# include <iostream>
# include <bitArray.H>

using namespace std;
using namespace Aleph;

void print(BitArray & array)
{
  cout << "size = " << array.size() << endl;

  for (size_t i = 0; i < array.size(); ++i)
    cout << array[i] << " ";
  cout << endl;
}

int main()
{
  BitArray array(40);

  for (int i = 0; i < 20; i++)
    {
      array[i] = 0;
    }

  array[20] = 1;
  array[27] = 1;
  array[22] = 1;

  for (int i = 30; i < 40; i++)
    {
      array[i] = array[i];
    }

  print(array);

  cout << endl;  

  {
    ofstream out("test.bits");
    array.save(out);
  }

  ifstream in("test.bits");
  array.load(in);

  print(array);
}
