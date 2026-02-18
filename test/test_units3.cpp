#include "units.hpp"

#include <print>
#include <string>

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

    return 0;
}