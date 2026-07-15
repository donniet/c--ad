#include "testing.hpp"

#include "expressions/expressions.hpp"

using test::ensure;
using namespace expressions;

struct SubstitutionTests
{
    static constexpr Variable< 16, int > n;
    static constexpr Variable< 0, float > x;
    static constexpr Variable< 1, float > y;
    static constexpr Variable< 2, float > z;
    static constexpr Constant< 0.f > zero;
    static constexpr Constant< 1.f > one;
    static constexpr Constant< (int)0 > zeroi;

    static_assert( is_compatible_substitution_v<Substitution<expressions::Product<
        expressions::StaticValue<int>, expressions::Variable<0, float>>, 
            expressions::Product<expressions::StaticValue<int>, 
                expressions::Variable<1, float>>>, float> );
//    static_assert( is_compatible_substitution_v<
//        expressions::Product<expressions::StaticValue<int>, float>, 
//            expressions::Product<expressions::StaticValue<int>, expressions::Variable<1, float>>> );
//    static_assert( is_compatible_substitution_v<
//        expressions::Product<expressions::StaticValue<float>, float>, 
//            expressions::Product<expressions::StaticValue<float>, 
//                expressions::Variable<1, float>>> );

    static_assert(( substitute_for( n + zeroi, n, zeroi ) | eval()) == 0 );
    static_assert(( substitute_for( x + one, x, one ) | eval()) == 2 );
    static_assert(( substitute_for( x + one, x, zero ) | eval()) == 1 );
    static_assert(( substitute( 2*x, one ) | eval()) == 2 );

    static_assert( is_same_v< std::remove_cv_t< decltype( 2.f * x )>,
        Product< StaticValue< float >, Variable< 0, float >>> );
    static_assert( is_same_v< std::remove_cv_t< decltype( ( 2.f * x )( 1.f ))>,
        Substitution< Product< StaticValue< float >, Variable< 0, float >>, float >> );

    // DT: then when the above type is |eval(), Substitution::value is called which calls
    //         substitute( Product< StaticValue< float >, Variable< 0, float >>{{ 3.f }, {}}, 2.f )
    //      which results in the expression
    //          Product< StaticValue< float >, float >{{ 3.f }, 2.f }
    //

    static_assert(( ( 2.f*x )( 1.f ) | eval()) == 2 );
    static_assert(( ( 3.f*x )( 2.f ) | eval()) == 6 );
//    static_assert(( ( 3.f*x )( 2.f*y )( 2.f ) | eval()) == 12 );
//    static_assert(( ( 3.f*x( 2.f ))( 2.f*y ) | eval()) == 12 );
};

bool test_basic()
{
    auto in_scope = simple_scope( 3.f, 4.f );
    
    Variable< 0, float > x;
    Variable< 1, float > y;

    auto f = x + y;
    auto g = f( 2.f, 2.f );
    // auto h = f( 2.f );

    assert(( g | eval() ) == 4.f );
    assert(( f | eval( in_scope )) == 7.f );
    // assert(( h | eval( in_scope )) == 9.f );

    return true;
}

bool test_second_order()
{
    Variable< 1, float > x;
    Variable< 0, float > f;

    auto g = f(x);
   
    static_assert( compound_expression<Product< Variable< 1, float >, Variable< 1, float >>> );
    static_assert( std::is_same_v< free_variables_t< Product< Variable< 1, float >, Variable< 1, float >>>,
        unique_variables< Variable< 1, float >>> ); 
    static_assert( expressions::detail::CompatibleSubstitutionHelper< unique_variables< Variable< 1, float >>, tuple< Variable< 1, float >>>::value );

    static_assert( expressions::detail::CompatibleSubstitution< Product< Variable< 1, float >, Variable< 1, float >>, 
        Variable< 1, float >>::value ); 
    static_assert( expressions::detail::IsCompatibleVariableSubstitution< 
        Variable< 0, Substitution< Variable<0, float>, Variable<1, float>>>, 
            Product< Variable< 1, float >, Variable< 1, float >>>::value ); 
    static_assert( is_compatible_substitution_v< 
        Substitution< Variable< 0, Variable< 0, float >>, Variable< 1, float >>, 
            Product< Variable< 1, float >, Variable< 1, float >>, float >);

    static_assert( is_same_v< std::remove_cv_t< decltype( g( x * x ))>,
        Substitution< Substitution< Variable< 0, Variable< 0, float >>, Variable< 1, float >>,
            Product< Variable< 1, float >, Variable< 1, float >>>> );

    assert(( g( x * x, 3.f ) | eval() ) == 9.f );
    assert(( g( x * x )( 3.f ) | eval() ) == 9.f );

    return true;
}

int main( int ac, char* av[] )
{
    ensure( test_basic, "basic substitutions" );

    return EXIT_SUCCESS;
}
