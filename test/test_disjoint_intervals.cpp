#include "disjoint_intervals.hpp"

#include <iostream>

int main(int ac, char* av[])
{
    disjoint_intervals< > intervals;

    intervals.insert(3, 6);
    intervals.insert(4, 8);

    for(auto x : intervals)
        std::cout << x << " ";

    std::cout << std::endl;

    return 0;
}