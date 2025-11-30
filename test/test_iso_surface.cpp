
#include <iostream>

import iso_surface;


template<typename FloatType>
std::ostream& operator<<(std::ostream& os, Point<FloatType> const& point)
{ return os << point.x << " " << point.y << " " << point.z; } 

int main(int ac, char* av[])
{
    Point<float> p;


    std::cout << p << std::endl;

    return 0;
}