// Include the Chlorine Header
#include "chlorine.hpp"
int main(int argc, char * argv[])
{
    // Create Some Data
    std::vector<float> spam(10, 3.1415f);
    std::vector<float> eggs(10, 2.7182f);

    // Initialize a Chlorine Worker
    ch::Worker worker("swap.cl");

    // Call the Swap Function in the Given Kernel
    worker.call("swap", spam, eggs);

    // Host Containers Are Automatically Updated
    std::cout << "Spam: " << spam[0] << "\n"; // 2.7182
    std::cout << "Eggs: " << eggs[0] << "\n"; // 3.1415
}
