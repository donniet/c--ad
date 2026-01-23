/**
 * test of matrix.hpp methods
 */

#include "matrix.hpp"

#include <iostream>
#include <cmath>

using std::sin, std::cos;
using std::numbers::pi;

auto rot( double angle )
{ return matrix::from( cos( angle ), sin( angle ),
                      -sin( angle ), cos( angle )); }

int main( int ac, char* av[] )
{
    using std::cout, std::endl;

    auto m = matrix::from( 1., 1.,
                           -1., 1. );

    auto r = rot( pi / 7. );

    cout << m << endl;
    cout << "det(m) = " << m.det() << endl;
    cout << "det(r) = " << r.det() << endl;
    cout << "r = \n" << r << endl;
    cout << "inv(r) = \n" << r.inverse() << endl;
    cout << "m + r = \n" << ( m + r ) << endl;
    cout << "m - r = \n" << ( m - r ) << endl;
    cout << "m * r = \n" << ( m * r ) << endl;
    cout << "m / r = \n" << ( m / r ) << endl;
    cout << "r / r = \n" << ( r / r ) << endl;

    return EXIT_SUCCESS;
}