#include <iostream>

#include "universe.hpp"

struct Test : virtual Object
{
    Test()
    { std::cerr << "Test()" << std::endl; }

    virtual ~Test()
    { std::cerr << "~Test()" << std::endl;}

    int x;
};

int main( int ac, char * av[] )
{
    using std::cout, std::endl;

    auto world = Universe{ "hello_world" };
    auto other = Universe{ "other_world" };
    // bool b = true;

    auto obj = new (world) Test;
    delete obj;

    obj = new (other) Test;
    // delete obj;

    cout << "test_memory" << endl;
}