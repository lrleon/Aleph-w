#include <iostream>
#include <string>
#include "timeoutQueue.H"

int main() {
    std::cerr << "=== Before creating queue ===" << std::endl;
    TimeoutQueue* queue = new TimeoutQueue();
    std::cerr << "=== Queue created ===" << std::endl;
    delete queue;
    std::cerr << "=== Queue deleted ===" << std::endl;
    return 0;
}
