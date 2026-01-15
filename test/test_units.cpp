#include "units.hpp"

#include <iostream>

int main(int ac, char * av[])
{
    using namespace units::literals;
    using namespace units::placeholders;
    using namespace units::variables;
    using namespace units::operators;
    using namespace units;
    using std::cout, std::endl;
    using std::to_string;

    Expression< Length > width = { 3_in };
    auto varx = expression_of( _x );
    auto vary = expression_of( _y );
    auto dx = d( varx );
    auto expx = exp( varx );
    auto dy = d( expx );
    auto logx = log( varx );

    auto a = exp( 0.5 * pow( varx, 2 ));

    auto expx_on_x = expx.quotient( varx );
    auto dexpx_on_x = d( expx_on_x );

    cout << to_string( varx ) << endl;
    cout << to_string( dx ) << endl;
    cout << to_string( width.value ) << endl;
    cout << to_string( dy ) << endl;
    cout << to_string( expx_on_x ) << endl;
    cout << to_string( dexpx_on_x ) << endl;
    cout << to_string( logx ) << endl;
    cout << to_string( d( logx ).quotient( d( varx ))) << endl;
    cout << to_string( a ) << endl;
    cout << to_string( d( a )) << endl;

    

    return 0;
}