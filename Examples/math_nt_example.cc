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
 * @file math_nt_example.cc
 * @brief Demonstration of Number Theory and Modular Arithmetic algorithms.
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <print_rule.H>
#include <modular_arithmetic.H>
#include <primality.H>
#include <pollard_rho.H>
#include <ntt.H>
#include <modular_combinatorics.H>
#include <modular_linalg.H>

using namespace std;
using namespace Aleph;

void demo_modular_arithmetic()
{
    cout << "[1] Modular Arithmetic (Safe 64-bit)\n";
    print_rule();
    
    uint64_t a = 3000000000000000000ULL;
    uint64_t b = 2000000000000000000ULL;
    uint64_t m = 4000000000000000000ULL;
    
    cout << "Safe multiplication avoiding 64-bit overflow:\n";
    cout << "  " << a << " * " << b << " mod " << m << "\n";
    cout << "  = " << mod_mul(a, b, m) << "\n\n";

    cout << "Extended Euclidean Algorithm:\n";
    int64_t x, y;
    int64_t gcd = ext_gcd<int64_t>(30, 20, x, y);
    cout << "  gcd(30, 20) = " << gcd << "\n";
    cout << "  Coefficients: x=" << x << ", y=" << y << " (30*x + 20*y = " << (30*x + 20*y) << ")\n\n";
    
    cout << "Chinese Remainder Theorem:\n";
    Array<uint64_t> rem = {2, 3, 2};
    Array<uint64_t> mod = {3, 5, 7};
    cout << "  x = 2 mod 3\n  x = 3 mod 5\n  x = 2 mod 7\n";
    cout << "  Solution: " << crt(rem, mod) << " (mod " << (3*5*7) << ")\n";
    print_rule();
    cout << "\n";
}

void demo_primality_factorization()
{
    cout << "[2] Primality Testing & Factorization\n";
    print_rule();
    
    uint64_t prime_candidate = 2147483647; // 2^31 - 1
    cout << "Miller-Rabin (deterministic for 64-bit):\n";
    cout << "  Is " << prime_candidate << " prime? " 
         << (miller_rabin(prime_candidate) ? "Yes" : "No") << "\n\n";

    uint64_t composite = 1000000007ULL * 97ULL;
    cout << "Pollard's Rho Factorization:\n";
    cout << "  Factoring " << composite << "...\n";
    auto factors = pollard_rho(composite);
    cout << "  Prime factors: ";
    for (size_t i = 0; i < factors.size(); ++i)
        cout << factors[i] << (i + 1 == factors.size() ? "" : ", ");
    cout << "\n";
    print_rule();
    cout << "\n";
}

void demo_ntt()
{
    cout << "[3] Number Theoretic Transform (NTT)\n";
    print_rule();
    
    cout << "Polynomial Multiplication modulo 998244353:\n";
    Array<uint64_t> A = {1, 2}; // 1 + 2x
    Array<uint64_t> B = {3, 4}; // 3 + 4x
    
    cout << "  A(x) = 1 + 2x\n";
    cout << "  B(x) = 3 + 4x\n";
    
    auto C = NTT<>::multiply(A, B);
    cout << "  C(x) = A(x) * B(x) = ";
    for (size_t i = 0; i < C.size(); ++i)
    {
        if (i > 0) cout << " + ";
        cout << C[i];
        if (i > 0) cout << "x^" << i;
    }
    cout << "\n";
    print_rule();
    cout << "\n";
}

void demo_modular_combinatorics()
{
    cout << "[4] Modular Combinatorics & Lucas Theorem\n";
    print_rule();
    
    uint64_t mod = 1000000007;
    ModularCombinatorics mc(10, mod);
    cout << "O(1) Combinations (precomputed factorials):\n";
    cout << "  C(5, 2) mod " << mod << " = " << mc.nCk(5, 2) << "\n\n";

    uint64_t small_mod = 3;
    ModularCombinatorics mc_lucas(10, small_mod);
    cout << "Lucas Theorem (large n, small prime):\n";
    cout << "  C(5, 2) mod " << small_mod << " = " << mc_lucas.lucas_nCk(5, 2) << "\n";
    print_rule();
    cout << "\n";
}

void demo_modular_linalg()
{
    cout << "[5] Modular Linear Algebra (Gauss)\n";
    print_rule();
    
    Array<Array<uint64_t>> m_data;
    m_data.append(Array<uint64_t>({1, 2}));
    m_data.append(Array<uint64_t>({3, 4}));

    uint64_t p = 5;
    ModularMatrix mat(m_data, p);
    
    cout << "Matrix A (mod " << p << "):\n";
    for (size_t i = 0; i < mat.get().size(); ++i)
        cout << "  [" << mat.get()[i][0] << ", " << mat.get()[i][1] << "]\n";
        
    cout << "Determinant |A| = " << mat.determinant() << "\n";
    
    auto inv_opt = mat.inverse();
    if (inv_opt.has_value())
    {
        cout << "Inverse A^-1 (mod " << p << "):\n";
        for (size_t i = 0; i < inv_opt->get().size(); ++i)
            cout << "  [" << inv_opt->get()[i][0] << ", " << inv_opt->get()[i][1] << "]\n";
    }
    else
    {
        cout << "Matrix is singular.\n";
    }
    
    cout << "\nSparse Matrix with DynMatrix (mod 7):\n";
    DynMatrix<uint64_t> sparse_data(3, 3, 0);
    sparse_data.write(0, 0, 2);
    sparse_data.write(1, 1, 3);
    sparse_data.write(2, 2, 4);
    ModularSparseMatrix smat(sparse_data, 7);
    
    cout << "Diagonal Matrix D:\n";
    for (size_t i = 0; i < 3; ++i)
      cout << "  [" << smat.get().read(i, 0) << ", " << smat.get().read(i, 1) << ", " << smat.get().read(i, 2) << "]\n";
    cout << "Determinant |D| = " << smat.determinant() << "\n";
    
    print_rule();
    cout << "\n";
}

int main()
{
    cout << "\n=== Mathematical & Number Theory Toolkit ===\n\n";
    
    demo_modular_arithmetic();
    demo_primality_factorization();
    demo_ntt();
    demo_modular_combinatorics();
    demo_modular_linalg();

    cout << "Done.\n";
    return 0;
}
