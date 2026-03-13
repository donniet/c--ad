#include "units.hpp"

#include <print>
#include <string>
#include <cassert>

int main( int ac, char* av[] )
{
    using std::string, std::println, std::runtime_format;

    using namespace units;

    string program = av[0];
    println( "running {}", program );

    // testing formatting
    println( "false == {}", false_bool );
    println( "false == {:i}", false_bool );
    println( "true == {}", true_bool );
    println( "true == {:i}", true_bool );

    assert( not false_bool );
    assert( true_bool );

    auto two = 2_scalar;
    auto one = 1_scalar;
    auto zero = 0_scalar;

    auto x = one + zero + one + zero;

    println( "1 + 0 + 1 + 0 == {}", x );
    assert( one + zero + one + zero == two );

    auto pi_rad = radians( pi );
    println( "pi_rad == {}", pi_rad );
    println( "pi_rad.get_value() == {}", pi_rad.get_value() );

    println( "180deg == {}", 180_deg );
    println( "180deg.get_value() == {}", (180_deg).get_value() );
    println( "180deg / pi == {}", 180_deg / pi );
    // not quite but close...
    // assert( 1 == 180_deg / pi );
    println( "180deg == {:deg}", 180_deg );
    println( "180deg == {:rad:10f}", 180_deg );
    println( "cos( 180deg ) == {}", cos( 180_deg ));
    println( "sin( 30deg ) == {}", sin( 30_deg ));
    println( "sin( pi/4 ) == {} ", sin( pi_rad / 4. ));

    println( "480_deg.get_value() == {}", (480_deg).get_value() );
    println( "120 * pi / 180 == {}", 120 * pi / 180 );
    println( "480_deg == {:deg}", 480_deg );

    println( "120_deg.value == {}", (120_deg).get_value() );
    println( "480_deg.value == {}", (480_deg).get_value() );
    println( "degrees(120).value == {}", degrees(120).get_value() );
    println( "degrees(480).value == {}", degrees(480).get_value() );

    
    assert( degrees( 480 ) == 480_deg );
    assert( degrees( 480 ) == degrees( 120 ) );
    assert( 480_deg == 120_deg );
    assert( degrees( -480 ) == degrees( 240 ) );

    // testing lengths
    auto five_km = 5_km;

    println( "5km == {}", five_km );
    println( "5km == {:km}", five_km );
    println( "5km == {:km:10f}", five_km );
    println( "5km == {:mi:10f}", five_km );
    println( "5km == {:ft:10f}", five_km );
    println( "5km == {:millimeters:10f}", five_km );

    auto ten_sec = 10_s;
    auto two_years = 2_y;

    println( "10s == {}", 10_s );
    println( "2years == {}", 2_y );
    println( "2years == {:y}", 2_y );
    println( "2years == {:ms:10f}", 2_y );

    auto five_pounds = 5_lb;
    auto five_kg = 5_kg;

    println( "5pounds == {}", 5_lb );
    println( "5pounds == {:kg:10f}", 5_lb );
    println( "5pounds == {:lb}", 5_lb );

    assert( 1_mi == 5280_ft );

    // unit products and quotients
    // TODO: test unit products and quotient formatting

    auto still_five_km = 5_km * one;
    assert( still_five_km == 5_km );
    still_five_km = 5_km / one;
    assert( still_five_km == 5_km );
    auto zero_km = 5_km * zero;
    assert( zero_km == 0_km );
    assert( zero_km != 5_km );
    assert( 5_km / 5_km == one );

    auto hectar = 1_percent * 1_km * 1_km;
    assert( hectar == 0.1_km * 0.1_km and 
            hectar == pow<2>( 0.1_km ));
    // assert( hectar == pow<2>( 0.1_km ) );
    assert( 1 == pow<0>( 0.1_km ));

    assert( 5_m / 1_s * 5_s / 1_m == 25 and
            5_m / 1_s * 5_s / 1_m == 25_scalar );

    assert( 5_m % 3_m == 2 and 
            5_m % 3_m == 2_scalar );

    assert( 5_m * 3_m % 2_m == 1_m );

    
    

    return 0;
}