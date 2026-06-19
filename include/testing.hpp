#ifndef __TESTING_HPP__
#define __TESTING_HPP__

#include <print>
#include <format>
#include <cassert>
#include <string>
#include <stdexcept>
#include <stdlib.h>


namespace test {

void report( bool success, std::string const& name, std::string const& msg )
{
    if( not success )
    {
        if( msg == "" )
            std::println( "FAILURE: {}", name );
        else
            std::println( "FAILURE: {}: {}", name, msg );

        std::exit( EXIT_FAILURE );
        return;
    }
    
    std::println( "SUCCESS: {}", name );
}

void ensure( bool (*func)(), std::string const& name )
{
    bool success = false;
    std::string msg = "";

    try 
    { if( success = func(); not success )
        msg = "returned false"; }
    catch( std::runtime_error& e )
    { msg = e.what(); }
    catch(...)
    { msg = "exception thrown"; }

    report( success, name, msg );
}

void ensure( void (*func)(), std::string const& name )
{
    bool success = false;
    std::string msg = "";

    try
    { func(); success = true; }
    catch( std::runtime_error& e )
    { msg = e.what(); }
    catch(...)
    { msg = "exception thrown"; }

    report( success, name, msg );
}

void ensure( std::pair< bool, std::string > (*func)(), std::string const& name )
{
    bool success = false;
    std::string msg = "";

    try
    { std::tie( success, msg ) = func(); }
    catch( std::runtime_error& e )
    { msg = e.what(); }
    catch(...)
    { msg = "exception thrown"; }

    report( success, name, msg );
}

} // namespace test

#endif // __TESTING_HPP__
