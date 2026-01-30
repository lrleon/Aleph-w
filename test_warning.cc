#include <iostream>
#include "ah-errors.H"

int main() {
    ah_warning(std::cerr) << "Test warning message" << std::endl;
    return 0;
}
