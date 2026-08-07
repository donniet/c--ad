#include "testing.hpp"

#include "expressions/expressions.hpp"
#include "expressions/predicate.hpp"
#include "expressions/arithmetic.hpp"

using test::ensure;

using namespace expressions;

struct SubTests
{
    static constexpr Var< 16, int > n;
    static constexpr Var< 0, float > x;
    static constexpr Var< 1, float > y;
    static constexpr Var< 2, float > z;
    static constexpr Constant< 0.f > zero;
    static constexpr Constant< 1.f > one;
    static constexpr Constant< (int)0 > zeroi;

    static_assert( is_compatible_substitution_v< Var< 0, float >, float >);
    static_assert( is_compatible_substitution_v< tuple< float, Var< 0, float >>, float >);

    static_assert( is_same_v< remove_cv_t< decltype( n + zeroi )>,
        Sum< Var< 16, int >, Constant< (int)0 >>> );
    static_assert( is_same_v< make_expression_t< Sum< Var< 16, int >, Constant< (int)0 >>>,
        Sum< Var< 16, int >, Constant< (int)0 >>> );
    static_assert( is_same_v< make_expression_t< Constant< (int)0 >>, Constant< (int)0 >> );
    static_assert(( sub_for< n.id >( n + zeroi, zeroi ) | eval()) == 0 ); 
    static_assert(( sub_for< x.id >( x + one, one ) | eval()) == 2 );
    static_assert(( sub_for< x.id >( x + one, zero ) | eval()) == 1 );
    static_assert(( substitute( 2*x, one ) == 2 ));
    static_assert(( substitute(( one + one ) * x, one ) | eval()) == 2 );

    static_assert( is_same_v< std::remove_cv_t< decltype( 2.f * x )>,
        Product< StaticValue< float >, Var< 0, float >>> );

    static_assert( compound_expression< Sum< Var< 0, int >, Var< 1, int >>> );
    static_assert( is_same_v< free_variables_t< Sum< Var< 0, int >, Var< 1, int >>>,
        unique_variables< Var< 0, int >, Var< 1, int >>> ); 
    static_assert( free_variables_t< Sum< Var< 0, int >, Var< 1, int >>>::size == 2 );
    //static_assert( requires { typename ForExpression< Sum< Var< 0, int >, Var< 1, int >>>; } );
    static_assert( is_compatible_substitution_v< Product< StaticValue< int >, Var< 0, float >>, float >,
       "FAILURE: substitution into product" );
    
    static_assert( not ForExpression< Sum< Var< 0, int >, Var< 1, int >>>::template Is<
        Difference< Constant< 5 >, Constant< 6 >>>::value,
            "FAILED: <int> + <int> =not-matches=> 5 - 6" );
    
    static_assert( ForExpression< Sum< Var< 0, int >, Var< 0, int >>>::template Is<
        Sum< Constant< 5 >, Constant< 5 >>>::value, 
            "FAILED: <int[0]> + <int[0]> =matches=> 5 + 5" );
    
    static_assert( not ForExpression< Sum< Var< 0, int >, Var< 0, int >>>::template Is<
        Sum< Constant< 5 >, Constant< 7 >>>::value, 
            "FAILED: <int[0]> + <int[0]> =not-matches=> 5 + 7" );
    
    static_assert( is_same_v< tuple_element_t< 0, typename ForExpression< 
        Sum< Var< 0, int >, Var< 0, int >>>::template Is<
            Sum< Constant< 5 >, Constant< 5 >>>::matches_type >, 
                match< Var< 0, int >, Constant< 5 >>> );
    
};

bool test_eval() 
{
    Var< 0, float > x;
    Constant< 0.f > zero;
    Constant< 1.f > one;

    auto evaluator = eval();

    Product< StaticValue< float >, Var< 0, float >> prod{ 2.f, {} };

    std::println( "prod[0]: {}", get_argument< 0 >( prod ).get_value() );

    using sub_t = Sub< Product< StaticValue< float >, Var< 0, float >>, Constant< 1.f >>;

    sub_t sub{ prod, Constant< 1.f >{} };

    std::println( "sub[0][0]: {}", get_argument< 0 >( get_argument< 0 >( sub )).get_value() );

    assert( (Applier< StaticValue< float >, Evaluator< void >>::value( StaticValue< float >{ 2.f }, evaluator )) == 2.f );
    //assert( (sub_t::value( Product< StaticValue< float >, Var< 0, float >>{{ 3.f }, {}}, 
    //    Constant< 1.f >{}) | eval()) == 3.f );

//    std::println( "sub result: {}", ( Applier< Sub< 
//        Product< StaticValue< float >, Var< 0, float >>, Constant< 1.f >>, Evaluator< void >>::
//            value({ prod, Constant< 1.f >{} }, evaluator )));

//    assert(( Applier< Sub< 
//        Product< StaticValue< float >, Var< 0, float >>, Constant< 1.f >>, Evaluator< void >>::
//            value({ prod, Constant< 1.f >{} }, evaluator )) == 2.f );

    static_assert( std::is_same_v< std::remove_cvref_t< decltype(
        one * x )>, Product< Constant< 1.f >, Var< 0, float >>> );
    
    static_assert( free_variables_t<Sub< 
        Product< Constant< 1.f >, Var< 0, float >>, Constant< 1.f >>>::size == 0 );
    static_assert( not non_expression<Sub< 
        Product< Constant< 1.f >, Var< 0, float >>, Constant< 1.f >>> );
    static_assert( closed_expression<Sub< 
        Product< Constant< 1.f >, Var< 0, float >>, Constant< 1.f >>> );

    static_assert( not open_expression<Sub< 
        Product< Constant< 1.f >, Var< 0, float >>, Constant< 1.f >>> );
    //static_assert( std::is_same_v< typename Applier< Sub< 
    //    Product< Constant< 1.f >, Var< 0, float >>, Constant< 1.f >>, 
    //        Evaluator< void >>::type, float > );


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
    
    Var< 0, float > x;
    Var< 1, float > y;

    auto f = x + y;
    auto g = f( 2.f, 2.f );
    // auto h = f( 2.f );

    assert(( g | eval() ) == 4.f );
    // TODO: come back to this once second order work
    //assert(( f | eval( in_scope )) == 7.f );
    // assert(( h | eval( in_scope )) == 9.f );

    return true;
}

bool test_second_order()
{
    Var< 0, float > x;
    Var< 1, float > y;
    Var< 2, float > f;
    
    using std::println;
    using std::remove_cv_t;

    println( "SECOND ORDER SUBSTITUTION" );
    println( "-------------------------" );

    println( "substitute_for( x*x, x, 3.f ) == {}", sub_for< x.id >( x*x, 3.f ) | eval() );
    
    auto h = x * x;
    println( "substitute_for( g, x, 3.f ) == {}", sub_for< x.id >( h, 3.f ) | eval() );

    println( "h(3.f) == {}", h(3.f) );

    auto l = f(x);
    //println( "f(x): {}", f(x) );
    //
    // var<0, float> f;
    // var<2, float> x;
    // var<3, float> g
    // var<4, float> y;
    //
    // f(x)(3.f)(x * x) 
    //
    // f(x):            func_sub( var<12, var<0, float>>, var<11, var<2, float>> )
    // f(x)(3.f)        sub_for<2>( var<12, var<0, float>>, 3.f )
    // f(x)(3.f)(x*x)   sub_for<0, 2>( var<0, float>, x*x, 3.f );
    //
    // g(y,x):          func_sub( var<13, var<3, float>>, var<11, var<4, float>>, var<12, var<2, float>> )
    // g(y,x)(8,4)      sub_for<4, 2>( var<13, var<3, float >>, 8.f, 4.f );
    // g(y,x)(8,4)(y/x) sub_for<3, 4, 2>( var<3, float>, y/x, 8.f, 4.f );
    //
    //
    //
    // sub(sub(sub( var<12, var<2, float>>, var<11, var<0, float>> ), 3.f ), x*x )
    // sub(sub( var<12, var<2, float>>, 3.f

    
    // OH! It's GetFreeVars on substitution expressions 
    //     I was subtracting a larger unsigned from a smaller one resulting in a very large sequence
    //static_assert( free_variables_t< 
    //    Sub< Sub< Var< 12, Var< 2, float >>, Var< 11, Var< 0, float >>>, 
    //        StaticValue< float >>>::size == 1, "TEST" );
    //static_assert( bound_variables_t< 
    //    Sub< Sub< Var< 12, Var< 2, float >>, Var< 11, Var< 0, float >>>, 
    //        StaticValue< float >>>::variable_set::size == 1 );

    // DT: not sure if Func should be a compound expression or not...
    static_assert( not compound_expression< Func< Var< 2, Var< 2, float >>, Var< 0, float >>> );
    //static_assert( is_same_v< remove_cv_t< decltype( l )>, 
    //    Func< Var< 2, Var< 2, float >>, Var< 0, float >>> );
    static_assert( is_same_v< remove_cv_t< decltype( l )>,
        Func< Var< 2, Var< 2, float >>, Var< 0, float >> >);

    using lt = remove_cv_t< decltype( l )>;

    static_assert( is_same_v< lt, Func< Var< 2, Var< 2, float >>, Var< 0, float >>> );
    static_assert( is_same_v< free_variables_t< lt >, 
        unique_variables< Var< 0, float >, Var< 2, Func< Var< 2, float >, Var< 0, float >>>>> ); 

    static_assert( open_expression< Func< Var< 2, Var< 2, float >>, Var< 0, float >>> );
//    static_assert( is_same_v< free_variables_t< 
//        Sub< Var< 2, Func< Var< 2, float >, Var< 0, float >>>, StaticValue< float >>>,
//            unique_variables< Var< 2, Func< Var< 2, float >, Var< 0, float >>>> > );
//    static_assert( open_expression< Sub< Var< 2, Func< Var< 2, float >, Var< 0, float >>>, StaticValue< float >>> );
    //static_assert( std::is_same_v< substitute_t< Var< 2, Func< Var< 2, float >, Var< 0, float >>>, StaticValue< float >>, void > );
    //static_assert( requires{ typename substitute_t< Var< 2, Func< Var< 2, float >, Var< 0, float >>>, StaticValue< float >>; } );
    //static_assert( is_same_v< free_variables_t< Func< Var< 2, Var< 2, float >>, Var< 0, float >>>, void > );

    auto m = l(3.f);
    using m_type = remove_cv_t< decltype( m )>;

    static_assert( is_same_v< m_type, Sub< Func< Var< 2, Var< 2, float >>, Var< 0, float >>, StaticValue< float >>> );
    static_assert( is_same_v< free_variables_t< m_type >, unique_variables<
        Var< 2, Func< Var< 2, float >, Var< 0, float >>>>> ); 

    using func_type = Func< Var< 2, Var< 2, float >>, Var< 0, float >>;
    using sub_type = Sub< func_type,  
            StaticValue< float >, Product< Var< 0, float >, Var< 0, float >>>;

    static_assert( is_same_v< free_variables_t< func_type >, 
        unique_variables< Var< 0, float >, Var< 2, Func< Var< 2, float >, Var< 0, float >>>>> );

    //static_assert( bound_variables_t< sub_type >::template IsDependent< 1, 0 >::value );

    // failing here because now that I've included the raw variables in GetFreeVars the dependency logic
    // is off
    //static_assert( is_same_v< typename bound_variables_t< sub_type >::binding_order_seq,
    //        seq< 1, 0 >> );

    using subber_type = Substituter< func_type, StaticValue< float >, Product< Var< 0, float >, Var< 0, float >>>;

    //static_assert( subber_type::variable_id_of< 1 > == 2 );
    //static_assert( is_same_v< typename PredicateSub< ForVar< 2 >::template Is, func_type, 
    //    Product< Var< 0, float >, Var< 0, float >>>::type, 
    //        Func< Product< Var< 0, float >, Var< 0, float >>, Var< 0, float >> > );
   
    // this almost works. we should check the specialization of substitute_t for functions
    //static_assert( is_same_v< substitute_t<
    //    Func< Var< 2, Var< 2, float >>, Var< 0, float >>, 
    //        StaticValue< float >, Product< Var< 0, float >, Var< 0, float >>>, 
    //            float >);
    
    //static_assert( is_same_v< void, SubFor< 2, m_type, Product< Var< 0, float >, Var< 0, float >>>::type > );

    //auto n2 = m.debug_sub( x*x ); 
    auto n = m( x*x );
    using n_type = remove_cv_t< decltype( n )>;
    static_assert( is_same_v< n_type, float > );

    static_assert( free_variables_t< n_type >::size == 0 );

    println( "f(x)(3.f)(x*x) == {}", n );


    return true;
}

constexpr bool test_func()
{
    Var< 0, int > x;
    Var< 1, int > y;
    Var< 2, int > z;

    Var< 3, int > f;
    Var< 4, int > g;

    auto h = f(x,y);
    auto l = g(y,x);

    //static_assert( is_same_v< void, bound_variables_t< 
    //        Sub< decltype( l(4,2) ), Difference< Var< 1, int >, Var< 0, int >>>
    //    >::variable_tuple > );
    //static_assert( is_same_v< void, substitute_t< 
    //    decltype( l( 4, 2 ) ), Difference< Var< 1, int >, Var< 0, int >> 
    //>>);
    //static_assert( is_same_v< void, bound_variables_t<
    //    Sub< Func< Difference

    assert( h(2,4)(y-x) == 2 );
    assert( l(4,2)(y-x) == 2 );

    return true;
}

int main( int ac, char* av[] )
{
    ensure( test_basic, "basic substitutions" );
    ensure( test_eval, "evaluation" );
    ensure( test_func, "functions" );
    ensure( test_second_order, "second_order" );

    return EXIT_SUCCESS;
}
