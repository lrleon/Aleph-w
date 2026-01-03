/**
 * @file test_coderabbit.H
 * @brief Test file for CodeRabbit PR review - DELETE AFTER TESTING
 * 
 * This file is intentionally created to test the CodeRabbit integration.
 * It contains some code patterns that CodeRabbit should detect and comment on.
 */

#ifndef TEST_CODERABBIT_H
#define TEST_CODERABBIT_H

#include <vector>
#include <string>

using namespace std;  // CodeRabbit should flag this in a header!

namespace Aleph
{

/**
 * @brief A test class with some intentional issues for CodeRabbit to find
 */
class TestClass
{
    int* ptr;  // Raw pointer without RAII - potential memory leak
    
public:
    TestClass() : ptr(new int(42)) {}  // Missing virtual destructor if inherited
    
    // Missing copy constructor and assignment operator (Rule of Three violation)
    
    ~TestClass() { delete ptr; }
    
    int getValue() { return *ptr; }  // Should be const
    
    void processData(vector<int> data)  // Should pass by const reference
    {
        for (int i = 0; i < data.size(); i++)  // Signed/unsigned comparison
        {
            cout << data[i] << endl;  // Using endl instead of '\n'
        }
    }
};

} // namespace Aleph

#endif // TEST_CODERABBIT_H

