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

    auto one = 1_scalar;
    auto zero = 0_scalar;

    auto x = one + zero + one + zero;

    println( "1 + 0 + 1 + 0 == {}", x );

    // testing lengths
    auto five_km = 5_km;

    println( "5km == {}", five_km );
    println( runtime_format("5km == {:km}"), five_km );
    println( runtime_format("5km == {:km:10f}"), five_km );
    println( runtime_format("5km == {:mi:10f}"), five_km );
    println( runtime_format("5km == {:ft:10f}"), five_km );
    println( runtime_format("5km == {:millimeters:10f}"), five_km );

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
    assert( hectar == 0.1_km * 0.1_km );
    assert( hectar == pow<2>( 0.1_km ) );
    assert( one == pow<0>( 0.1_km ));
    

    return 0;
}