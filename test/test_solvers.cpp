/// solvers unit test

#include "expressions/calculus.hpp"
#include "expressions/solvers.hpp"
#include "units.hpp"

#include "testing.hpp"

#include <print>

using namespace expressions;
using namespace test;

bool test_minimization()
{
    //auto solver = gradient_descent( x, y );
    //solver[ maximum_iterations ] = 1000;
    //solver[ learning_rate ] = 1e-3;
    //solver( para );
    //println( "solved para({}, {}) == {}", eval( x ), eval( y ), eval( para ));

    auto vars = declare_variables(
        var< Cardinal    >( "n" ),
        var< double      >( "x" ),
        var< Length      >( "w" ),
        var< Length      >( "z" ));

    auto [ n, x, w, z ] = vars.variables();

    // parabola
    auto para = ( ( x - 2 ) * ( x - 2 ) + 3 );

    //auto x0 = argmin( para, x );


    // paraboloid
    auto para2 = ( ( w - 2_ft ) * ( w - 2_ft ) + ( z - 3_ft ) * ( z - 3_ft ) + 3_ft * 1_ft );

    //assert( para2( 2_ft, 3_ft ) == 3_ft * 1_ft );

    // verify the free_variables_t trait works
//    static_assert( 
//        ( z.index < w.index and is_same_v< 
//            tuple< decltype( z ), decltype( w )>,
//            free_variables_t< decltype( para2 )>> ) or 
//        ( w.index < z.index and is_same_v< 
//            tuple< decltype( w ), decltype( z )>,
//            free_variables_t< decltype( para2 )>> ));
//
    return true;
}

bool test_canonicalization()
{
    auto scope = declare_variables(
        var< double >( "x" ),
        var< double >( "y" ));

    auto [ x, y ] = scope.variables();

    auto f = (  2*x + 5*y == 22 );
    auto g = ( -4*x +   y == 11 );
    // y == 5, x == -1.5

    println( "f: {}", f );
    println( "g: {}", g );
    println( "f and g: {}", f and g );
    println( "canonical( f and g ): {}", canonicalize( f and g ));

    return true;
}

std::pair< bool, std::string > test_boolean_satisfaction()
{
    auto scope = declare_variables(
        var< bool >("b0"), var< bool >("b1"), var< bool >("b2"), var< bool >("b3"),
        var< bool >("b4"), var< bool >("b5"), var< bool >("b6"), var< bool >("b7")
    );

    auto [ b0, b1, b2, b3, b4, b5, b6, b7 ] = scope.variables();

    if( b0 | solve_for( b0 )) { /* success */ }
    else return { false, "solve for a single boolean variable" };

    ( b1 or b2 ) | solve( scope );

    if(( b1 | scope ) or ( b2 | scope )) { /* success */ }
    else return { false, "solve for (b1 or b2) failed" };

    auto [ t3, t4 ] = ( b3 and b4 ) | solve_for( b3, b4 );

    if( t3 ) { /* success */ }
    else return { false, "solve for (b3 and b4) and checking for b3 failed" };

    if( t4 ) { /* success */ }
    else return { false, "solve for (b3 and b4) and checking b4 failed" };

    return { true, "" };
}

constexpr bool test_is_linear()
{
    using x_type = Var< 0, float >;
    using y_type = Var< 1, float >;
    static constexpr Var< 0, float > x;
    static constexpr Var< 1, float > y;
    static constexpr Var< 2, float > z;
    //auto two = static_expr( 2.f );

    static_assert( is_linear_equation( 0_c == 1_c ));
    static_assert( is_linear_equation( x + y == 0_c ));
    static_assert( is_linear_equation( x / 1_c - y == x ));
    static_assert( not is_linear_equation( x / y == 1_c ));
    // NOTE: pow<(0|1)> is not considered linear until canonicalizer has been written
    // static_assert( is_linear_equation( pow< 1 >( x ) + pow< 0 >( y ) == x + 1_c ));
    static_assert( is_linear_equation( x == y + 1_c ));
    static_assert( not is_linear_equation( x < y + 1_c ));
    static_assert( not is_linear_equation( x * y == 1_c ));
    static_assert( not is_linear_equation( sin( x ) == 0_c ));
    static_assert( is_linear_equation( 2*x == y ));

    static_assert( is_linear_of( x + y, x ));
    static_assert( is_linear_of( 2*x, x ));
    static_assert( is_linear_of( 2*x + y*y + 0_c, x ));
    static_assert( not is_linear_of( 2*x + y*y + 0_c, y ));

    static_assert(( substitute( 2*x, 1_c ) | eval()) == 2 );
    
    static constexpr auto fx = ( 5 * x );
    using deps_fx  = free_variables_t< std::remove_cv_t< decltype( fx )>>;
    using deps_fx2 = free_variables_t< tuple< StaticValue< int >, Var< 0, float >>>;
    
    static_assert( std::is_same_v< deps_fx, deps_fx2 >);
    static_assert( std::is_same_v< Product< StaticValue<int>, Var< 0, float >>,
        std::remove_cv_t< decltype( fx )>> );
    static_assert( std::tuple_size_v< deps_fx > == 1 );

    static constexpr auto fxy = ( 5*x + 4*y );
    using deps_fxy  = free_variables_t< std::remove_cv_t< decltype( fxy )>>;
    using deps_fxy2 = free_variables_t< tuple< tuple< StaticValue<int>, Var<0, float>>,
        tuple< StaticValue<int>, Var<1, float>>>>;
    using deps_fxy3 = unique_variables< Var< 0, float >, Var< 1, float >>;

    static_assert( std::is_same_v< deps_fxy, deps_fxy2 >);
    static_assert( std::is_same_v< deps_fxy, deps_fxy3 >);
    
    static_assert( scalar_of( 2*x, x ) == 2 );

    auto hxy = 2*x + 3*y + 4;
    using hxy_type = std::remove_cv_t< decltype( hxy )>;

    static_assert( is_same_v< hxy_type,
        Sum< Sum<  
                Product< StaticValue< int >, Var< 0, float >>,
                Product< StaticValue< int >, Var< 1, float >>>,
            StaticValue< int >>> );
    static_assert( expressions::detail::IsLinearOf< Var< 1, float >, hxy_type >::value );
    static_assert( not is_boolean_expression_v< hxy_type > );
    static_assert( depends_on_variable_v< Var< 1, float >, hxy_type > );
    
    static_assert( scalar_of( 2*x + 3*y + 4, x ) == 2 );
    static_assert( scalar_of( 2*x + 3*y + 4, y ) == 3 );
    static_assert( non_homogeneous_term_of( 2*x + 3*y + 4 ) == 4 ); 

    // solution is ( -4, 1 )
    static constexpr auto sys = 
        (   x - 7*y == -11 ) and
        ( 5*x + 2*y == -18 );

    using sys_type = Conjunction< 
        Equals< 
            Difference< 
                Var<0,float>, 
                Product< StaticValue<int>,Var<1,float>>>, 
            StaticValue<int>>,
        Equals< 
            Sum< 
                Product< StaticValue<int>, Var<0,float>>, 
                Product< StaticValue<int>,Var<1,float>>>, 
            StaticValue<int>>>;

    static_assert( std::is_same_v< sys_type, std::remove_cv_t< decltype( sys )>> );

    using deps = free_variables_t< decltype( sys )>;
    using deps2 = free_variables_t< sys_type >;

    static_assert( std::is_same_v< deps, deps2 > );
    

    // verifying dependent variables
    static_assert( std::tuple_size_v< deps > == 2 );
    static_assert( deps::template element_t< 0 >::id == 0 );
    static_assert( deps::template element_t< 1 >::id == 1 );
    static_assert( deps::size == 2 );

    // verifying conjunction
    static_assert( is_conjunction_v< std::remove_cv_t< decltype( sys )>> );

    // verify each formula is a linear equation
    static_assert( is_linear_equation( get_argument< 0 >( sys )) );
    static_assert( is_linear_equation( get_argument< 1 >( sys )) );

    static constexpr auto first_eq = get_argument< 0 >( sys );
    static constexpr auto second_eq = get_argument< 1 >( sys );

    // verify the scalars 
    static_assert( scalar_of< x_type >( get_argument< 0 >( first_eq )) ==  1 );
    static_assert( scalar_of< y_type >( get_argument< 0 >( first_eq )) == -7 );
    static_assert( scalar_of< x_type >( get_argument< 0 >( second_eq )) == 5 );
    static_assert( scalar_of< y_type >( get_argument< 0 >( second_eq )) == 2 );

    // verify the non-homogeneous terms
    static_assert(( get_argument< 1 >( first_eq )  == -11 ) | eval() );
    static_assert(( get_argument< 1 >( second_eq ) == -18 ) | eval() );

    //static_assert( expressions::detail::LinearSystem< decltype( sys )>::value );

    static constexpr auto sol = solve_linear_system( sys ) | eval();
    static_assert( std::get< 0 >( sol ) == -4 );
    // it's so close!  0.99999994!
    //static_assert( std::get< 1 >( sol ) == 1 );
    
    //static constexpr auto sol2 = sys | solve_for( x, y );
    //static_assert( std::get< 0 >( sol2 ) == -4 );

    return true;
}

static_assert( test_is_linear() );


template< auto Value >
consteval bool basic_solvers()
{
    using value_type = std::remove_cv_t< decltype( Value )>;
    static constexpr Var< 0, value_type > x;
    Constant< Value > value;

    static_assert(( x == value | solve_for( x )) == Value );
    static_assert(( x == static_expr( Value ) | solve_for( x )) == Value );
    static_assert(( x + 1_c == value + 1_c | solve_for( x )) == Value );
    static_assert(( x + 2_c == value + 1_c + 1_c | solve_for( x )) == Value );


    return true;
}

static_assert( basic_solvers< 7 >() );

//static_assert( Solver< Equals< Var< 0, int >, Constant< 7 >>>{}( Var< 0, int >{} ) == 7 );
//static_assert( Solver< Equals< Constant< 7 >, Var< 0, int >>>{}( Var< 0, int >{} ) == 7 );
//static_assert( Solver< Equals< Sum< Var< 0, int >, Constant< 7 >>, Constant< 14 >>>{}( Var< 0, int >{} ) == 7 );
//static_assert( Solver< Equals< Sum< Var< 0, int >, Constant< 5 >, Constant< 2 >>, Constant< 14 >>>{}( Var< 0, int >{} ) == 7 );

constexpr bool test_constraints( )
{
    float minimum_chord_length = 0.1;
    auto minimum_chord_length2 = minimum_chord_length * minimum_chord_length;

    auto in_scope = declare_variables(
        var< float >( "θ" ),
        var< float >( "x" ),
        var< float >( "y" ));

    auto [ θ, x, y ] = in_scope.variables();

    // revolve a point
    auto constraint = (
        x == 4.f * cos( θ ) and
        y == 4.f * sin( θ ) and
        ( x - 4.f ) * ( x - 4.f ) + y * y > minimum_chord_length2 );

    //auto solved = constraint | solver::minimize( θ, in_scope );
    //auto solution = minimize( θ, constraint ) | solve( in_scope );    
    return true;
}

using std::print, std::println;

int main( int ac, char * av[] )
{
    println("TESING SOLVERS...");
    
    ensure( test_boolean_satisfaction, "Boolean Satisfaction" );
    ensure( test_canonicalization, "Canonicalization" );
    ensure( test_minimization, "Minimization" );
    ensure( test_constraints, "Constraints" );
    ensure( test_is_linear, "Linear Expressions" );

    println("SUCCESS.");
    return EXIT_SUCCESS;
}
