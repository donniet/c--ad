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

    auto two = 2_scalar;
    auto one = 1_scalar;
    auto zero = 0_scalar;

    auto x = one + zero + one + zero;

    println( "1 + 0 + 1 + 0 == {}", x );
    assert( one + zero + one + zero == two );

    // testing lengths
    auto five_km = 5_km;

    println( "5km == {}", five_km );
    println( runtime_format("5km == {:km}"), five_km );
    println( runtime_format("5km == {:km:10f}"), five_km );
    println( runtime_format("5km == {:mi:10f}"), five_km );
    println( runtime_format("5km == {:ft:10f}"), five_km );
    println( runtime_format("5km == {:millimeters:10f}"), five_km );

    auto ten_sec = 10_s;
    auto two_years = 2_y;

    println( "10s == {}", 10_s );
    println( "2years == {}", 2_y );
    println( runtime_format("2years == {:y}"), 2_y );
    println( runtime_format("2years == {:ms:10f}"), 2_y );

    auto five_pounds = 5_lb;
    auto five_kg = 5_kg;

    println( "5pounds == {}", 5_lb );
    println( runtime_format("5pounds == {:kg:10f}"), 5_lb );
    println( runtime_format("5pounds == {:lb}"), 5_lb );

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

    auto hectar = 0.01_scalar * 1_km * 1_km;
    assert( hectar == 0.1_km * 0.1_km and hectar == pow<2>( 0.1_km ));
    // assert( hectar == pow<2>( 0.1_km ) );
    assert( one == pow<0>( 0.1_km ));


    

    return 0;
}