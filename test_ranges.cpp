#include <iostream>
#include <vector>
#include <ranges> // For std::views::filter

#include "iso_surface.hpp"

int main() 
{
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6};

    // Filter out even numbers
    auto even_numbers_view = numbers | std::views::filter([](int n) {
        return n % 2 == 0;
    });

    for (int n : even_numbers_view) {
        std::cout << n << " ";
    }
    std::cout << std::endl; // Output: 2 4 6

    return 0;
}