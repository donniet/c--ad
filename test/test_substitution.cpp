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

    static_assert( is_compatible_substitution_v< Variable< 0, float >, float >);
    static_assert( is_compatible_substitution_v< tuple< float, Variable< 0, float >>, float >);

// TODO: these asserts began to fail-- maybe they are malformed given the work on second-order vars?
//    static_assert( is_compatible_substitution_v<Substitution<expressions::Product<
//        expressions::StaticValue<int>, expressions::Variable<0, float>>, 
//            expressions::Product<expressions::StaticValue<int>, 
//                expressions::Variable<1, float>>>, float> );
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
    static_assert(( substitute(( one + one ) * x, one ) | eval()) == 2 );

    static_assert( is_same_v< std::remove_cv_t< decltype( 2.f * x )>,
        Product< StaticValue< float >, Variable< 0, float >>> );

    // DT: then when the above type is |eval(), Substitution::value is called which calls
    //         substitute( Product< StaticValue< float >, Variable< 0, float >>{{ 3.f }, {}}, 2.f )
    //      which results in the expression
    //          Product< StaticValue< float >, float >{{ 3.f }, 2.f }
    //
   
    static_assert(( ( 2.f * one ) | eval()) == 2.f );
    static_assert(( ( 2.f * one + zero ) | eval()) == 2.f );
    static_assert(( ( 2.f * one + 3.f ) | eval()) == 5.f );
    static_assert(( ( 3.f * one + one ) | eval()) == 4.f );
};

bool test_eval() 
{
    Variable< 0, float > x;
    Constant< 0.f > zero;
    Constant< 1.f > one;

    auto evaluator = eval();

    Product< StaticValue< float >, Variable< 0, float >> prod{ 2.f, {} };

    std::println( "prod[0]: {}", get_argument< 0 >( prod ).get_value() );

    using sub_t = Substitution< Product< StaticValue< float >, Variable< 0, float >>, Constant< 1.f >>;

    sub_t sub{ prod, Constant< 1.f >{} };

    std::println( "sub[0][0]: {}", get_argument< 0 >( get_argument< 0 >( sub )).get_value() );

    assert( (Applier< StaticValue< float >, Evaluator< void >>::value( StaticValue< float >{ 2.f }, evaluator )) == 2.f );
    assert( (sub_t::value( Product< StaticValue< float >, Variable< 0, float >>{{ 3.f }, {}}, 
        Constant< 1.f >{}) | eval()) == 3.f );

//    std::println( "sub result: {}", ( Applier< Substitution< 
//        Product< StaticValue< float >, Variable< 0, float >>, Constant< 1.f >>, Evaluator< void >>::
//            value({ prod, Constant< 1.f >{} }, evaluator )));

//    assert(( Applier< Substitution< 
//        Product< StaticValue< float >, Variable< 0, float >>, Constant< 1.f >>, Evaluator< void >>::
//            value({ prod, Constant< 1.f >{} }, evaluator )) == 2.f );

    static_assert( std::is_same_v< std::remove_cvref_t< decltype(
        one * x )>, Product< Constant< 1.f >, Variable< 0, float >>> );

    static_assert( std::is_same_v< typename Applier< Substitution< 
        Product< Constant< 1.f >, Variable< 0, float >>, Constant< 1.f >>, 
            Evaluator< void >>::type, float > );


    static_assert( std::is_same_v< std::remove_cvref_t< decltype(
        ( one * x )( one ) | eval() )>, float >);

    static_assert( std::is_same_v< std::remove_cvref_t< decltype( 
        (( one + one ) * x )( one ) | eval() )>, float >);

    assert(( ((one + one) * x )( one ) | eval()) == 2.f );
    assert(( ( 2.f * x )( one ) | eval()) == 2.f );
    assert(( ( 2.f*x )( 1.f ) | eval()) == 2 );
    assert(( ( 3.f*x )( 2.f ) | eval()) == 6 );
    return true;
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
    Variable< 0, float > x;
    Variable< 1, float > y;
    Variable< 2, float > f;

    //static_assert( compound_expression<Product< Variable< 1, float >, Variable< 1, float >>> );
    //static_assert( std::is_same_v< free_variables_t< Product< Variable< 1, float >, Variable< 1, float >>>,
    //    unique_variables< Variable< 1, float >>> ); 
    //static_assert( expressions::detail::IsCompatibleSubstitutionHelper< unique_variables< Variable< 1, float >>, tuple< Variable< 1, float >>>::value );

    //static_assert( expressions::detail::IsCompatibleSubstitution< Product< Variable< 1, float >, Variable< 1, float >>, 
    //    Variable< 1, float >>::value ); 
    //static_assert( expressions::detail::IsCompatibleVariableSubstitution< 
    //    Variable< 0, Substitution< Variable<0, float>, Variable<1, float>>>, 
    //        Product< Variable< 1, float >, Variable< 1, float >>>::value ); 
    //static_assert( is_compatible_substitution_v< 
    //    Substitution< Variable< 0, Variable< 0, float >>, Variable< 1, float >>, 
    //        Product< Variable< 1, float >, Variable< 1, float >>, float >);

//    static_assert( is_same_v< std::remove_cv_t< decltype( g( x * x ))>,
//        Substitution< Substitution< Variable< 0, Variable< 0, float >>, Variable< 1, float >>,
//            Product< Variable< 1, float >, Variable< 1, float >>>> );
//
//    assert(( g( x * x, 3.f ) | eval() ) == 9.f );
//    assert(( g( 3.f, x * x ) | eval() ) == 9.f );

//    static_assert( is_same_v< void, decltype( g )>, "g typename" );
//    static_assert( is_same_v< void, decltype( g( 3.f ) )>, "typename" );
//    std::println( "g( 3.f ) == {}", g( 3.f ) | eval() );
//


    // Can we distinguish between a variable sub with a single variable, and a swap of one var
    // for another?
    // f(x);
    // ( x + 0 )( y );
    //
    static_assert( std::is_same_v< free_variables_t<
        Substitution< 
            Variable<12, Variable<2, float>>, 
                Variable<11, Variable<0, float>>>>,
        unique_variables<
            Variable<  0, float >,
            Variable< 12, Substitution< Variable< 2, float >, Variable< 0, float >>>>
    > );

    // DT: We are close to this.  The current problem is that the substitution order matters.
    //     We likely need to build a topographical sort.  Or we could remove any of the variables
    //     that were bound during this instance of Substitution...
    static_assert( std::is_same_v< unique_variables< >, free_variables_t<
        Substitution<
            Substitution< Variable<12, Variable<2, float>>, Variable<11, Variable<0, float>>>, 
        float, 
        Product<Variable<0, float>, Variable<0, float>>> 
    >> );

    //static_assert( std::is_same_v< void, free_variables_t<
    //    Substitution< Variable< 12, Variable< 2, float >>, Variable< 11, Variable< 0, float >>>>> );

    static_assert( std::is_same_v< typename Evaluator< void >::Helper<
        Substitution< Variable< 12, Variable< 2, float >>, Variable< 11, Variable< 0, float >>>>::type,
        void > );

    auto g = f(x);
    
    // DT: this and the below goes into an infinite compiler loop.
    //     It seems the evaluator's recursion is broken.
    auto h = g(3.f)( x*x );
  
//    std::println( "g( 3.f )( x * x ) == {}", g( 3.f )( x*x ) | eval() );
    //static_assert( std::is_same_v< decltype( g( 3.f,  x * x )), void >, "TEST" );
//    std::println( "g( 3.f, x * x ) == {}", g( 3.f, x * x ) | eval() );
//    assert(( g( 3.f,  x * x ) | eval() ) == 9.f );
//    assert(( ( 3.f * x )( 2.f ) | eval() ) == 6.f );
//    assert(( ( 2.f * y )( ( 3.f * x )( 2.f ) ) | eval() ) == 12.f );
//    assert(( ( 2.f * y )( 3.f * x )( 2.f ) | eval() ) == 12.f );
//
//    auto h = ( 3.f * x )( 2.f * y );
//    
//    static_assert( std::is_same_v< std::remove_cv_t< decltype( h )>,
//        Substitution< Product< StaticValue< float >, Variable< 0, float >>,
//            Product< StaticValue< float >, Variable< 1, float >>>> );
//
//    std::println( "h( 2.f ) == {}", h( 2.f ) | eval() );
//    assert(( h( 2.f ) | eval() ) == 12.f );
//
//    std::println( "( 3.f*x )( 2.f*y )( 2.f ) == {}", ( 3.f*x )( 2.f*y )( 2.f ) | eval() );
//    assert(( ( 3.f*x )( 2.f*y )( 2.f ) | eval()) == 12 );
//    assert(( ( 3.f*x( 2.f ))( 2.f*y ) | eval()) == 12 );

    return true;
}

int main( int ac, char* av[] )
{
    ensure( test_basic, "basic substitutions" );
    ensure( test_eval, "evaluation" );
    ensure( test_second_order, "second_order" );

    return EXIT_SUCCESS;
}
