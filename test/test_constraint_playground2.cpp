/**
 * test_constraint_playground2.cpp is the second experiment in solving simple
 * differntiable functions using newtons method and autodifferentiation.  I'd 
 * like this to be used to solve the dimensions system in c++ad.
 */

#include "utility.hpp"
#include "matrix.hpp"

#include <iostream>
#include <string>
#include <numeric>
#include <optional>
#include <functional>
#include <map>
#include <vector>
#include <variant>
#include <cmath>
#include <ranges>
#include <set>
#include <stdexcept>

using std::string;
using std::optional;
using std::function;
using std::map;
using std::set;
using std::vector;
using std::variant, std::monostate, std::holds_alternative;
using std::abs;
using std::invalid_argument;

template< typename ExprTuple >
struct evaluated_type;

template< typename... Expr >
struct evaluated_type< std::tuple< Expr... >>
{ 
    static constexpr const size_t size = sizeof...( Expr );
    using type = std::array< double, size >; 
};

template< typename ExprTuple >
using evaluated_type_of = evaluated_type< ExprTuple >::type;

struct Variable;

using VariableValues = map< Variable, double >;

struct Variable 
{ 
    string name; 
    int dots = 0; 
    bool operator<( Variable const& other ) const
    {
        if( name < other.name )
            return true;
        if( other.name < name )
            return false;
        return dots < other.dots;
    }

    double operator()( VariableValues const& values )
    { return values.find( *this )->second; }
};

struct Constant
{
    double value;

    double operator()( VariableValues const& )
    { return value; }
};

template< typename... Expr >
struct Formulae
{
    static constexpr const size_t size = sizeof...( Expr );
    using tuple_type = std::tuple< Expr... >;
    using eval_type = evaluated_type_of< tuple_type >;

    eval_type operator()( VariableValues const& values )
    { return eval_helper( values, std::make_index_sequence< size >{} ); }
private:
    template< size_t... Is >
    eval_type eval_helper( VariableValues const& values )
    { return { std::get< Is >( expressions )( values )... }; }

    tuple_type expressions;
};

template< typename Expr1, typename Expr2 >
struct Multiply
{ 
    double operator()( VariableValues const& values )
    { return first( values ) * second( values ); }

    Expr1 first; 
    Expr2 second; 
};

// TODO: continue these maybe?
template< typename... Exprs1, typename... Exprs2 >
requires ( sizeof...( Exprs1 ) == sizeof... ( Exprs2 ) )
struct Multiply< Formulae< Exprs1... >, Formulae< Exprs2... >>
{
    static constexpr const size_t size = sizeof...( Exprs1 );
    using eval_type = typename Formulae< Exprs1... >::eval_type;

    eval_type operator()( VariableValues const& values )
    { return eval_helper( values, std::make_index_sequence< size >{} ); }

private:
    template< size_t... Is >
    eval_type eval_helper( VariableValues const& values )
    { return { (std::get< Is >( first.expressions )( values ) * 
        std::get< Is >( second.expressions ))... }; }

    Formulae< Exprs1... > first;
    Formulae< Exprs2... > second;
};

template< typename Expr1, typename Expr2 >
struct Add
{ 
    double operator()( VariableValues const& values )
    { return first( values ) + second( values ); }

    Expr1 first; 
    Expr2 second; 
};

template< typename Expr >
struct Sine
{
    double operator()( VariableValues const& values )
    { return std::sin( first( values )); }

    Expr first;
};

template< typename Expr >
struct Cosine
{
    double operator()( VariableValues const& values )
    { return std::cos( first( values )); }

    Expr first;
};

template< typename Expr >
struct Exponent
{
    double operator()( VariableValues const& values )
    { return std::exp( first( values )); }

    Expr first;
};

template< typename Expr >
struct Logarithm
{
    double operator()( VariableValues const& values )
    { return std::log( first( values )); }

    Expr first;
};

template< typename Expr >
struct EqualsZero
{ 
    bool operator()( VariableValues const& values )
    { return 0 == first( values ); }

    Expr first; 
};

template< typename... >
struct Conjunction;

// DT: currently we only support conjunctions of EqualsZero expressions
template< typename... Exprs >
struct Conjunction< EqualsZero< Exprs >... >
{
    using tuple_type = std::tuple< EqualsZero< Exprs >... >;
    static constexpr const size_t size = sizeof...( Exprs );

    bool operator()( VariableValues const& values )
    { return eval_helper( values, std::make_index_sequence< size >{} ); }

private:
    template< size_t... Is >
    bool eval_helper( VariableValues const& values, std::index_sequence< Is... > )
    { return ( std::get< Is >( expressions )( values ) and ... ); }

public:
    tuple_type expressions;
};

/**
 * helper functions
 */
Constant constant( double value )
{ return { value }; }

Variable variable( string name, int dots = 0 )
{ return { name, dots }; }

template< typename... Expr >
Formulae< Expr... > formulae( Expr... exprs )
{ return { exprs... }; }

template< typename Expr1, typename Expr2 >
Multiply< Expr1, Expr2 > multiply( Expr1 expr1, Expr2 expr2 )
{ return { expr1, expr2 }; }

template< typename Expr >
Multiply< Expr, Constant > multiply( Expr expr, double scalar )
{ return { expr, constant( scalar )}; }

template< typename Expr >
Multiply< Constant, Expr > multiply( double scalar, Expr expr )
{ return { constant( scalar ), expr }; }

constexpr double multiply( double scalar1, double scalar2 )
{ return scalar1 * scalar2 ; }

template< typename Expr1, typename Expr2 >
Add< Expr1, Expr2 > add( Expr1 expr1, Expr2 expr2 )
{ return { expr1, expr2 }; }

template< typename Expr >
Add< Expr, Constant > add( Expr expr, double scalar )
{ return { expr, constant( scalar )}; }

template< typename Expr >
Add< Constant, Expr > add( double scalar, Expr expr )
{ return { constant( scalar ), expr }; }

constexpr double add( double scalar1, double scalar2 )
{ return scalar1 + scalar2; }

template< typename Expr >
Sine< Expr > sine( Expr expr )
{ return { expr }; }

Constant sine( double value )
{ return { std::sin( value ) }; }

template< typename Expr >
Cosine< Expr > cosine( Expr expr )
{ return { expr }; }

constexpr double cosine( double value )
{ return std::cos( value );  }

template< typename Expr >
auto tangent( Expr expr )
{ return quotient( sine( expr ), cosine( expr )); }

template< typename Expr >
Exponent< Expr > exponent( Expr expr )
{ return { expr }; }

constexpr double exponent( double value )
{ return std::exp( value ); }

template< typename Expr >
Logarithm< Expr > logarithm( Expr expr )
{ return { expr }; }

constexpr double logarithm( double value )
{ return std::log( value ); }

template< typename Expr1, typename Expr2 >
auto power( Expr1 expr1, Expr2 expr2 )
{ return exponent( multiply( logarithm( expr1 ), expr2 )); }

template< typename Expr1, typename Expr2 >
auto quotient( Expr1 expr1, Expr2 expr2 )
{ return multiply( expr1, power( expr2, -1. )); }

template< typename Expr >
EqualsZero< Expr > equals_zero( Expr expr )
{ return { expr }; }

EqualsZero< Constant > equals_zero( double value )
{ return {{ value }}; }

/**
 * Derivatives of expressions
 */
Variable d( Variable var )
{ return { var.name, var.dots+1 }; }

Constant partial( Variable expr, Variable by )
{ return { expr.name == by.name ? 1. : 0. }; }

Constant d( Constant )
{ return { 0. }; }

Constant partial( Constant, Variable )
{ return { 0. }; }

template< typename Expr1, typename Expr2 >
auto d( Add< Expr1, Expr2 > expr )
{ return add( d( expr.first ), d( expr.second )); }

template< typename Expr1, typename Expr2 >
auto partial( Add< Expr1, Expr2 > expr, Variable by )
{ return add( partial( expr.first, by ), partial( expr.second, by )); }

template< typename Expr1, typename Expr2 >
auto d( Multiply< Expr1, Expr2 > expr )
{ return add( multiply( d( expr.first ), expr.second ), multiply( expr.first, d( expr.second ))); }

template< typename Expr1, typename Expr2 >
auto partial( Multiply< Expr1, Expr2 > expr, Variable by )
{ return add( multiply( partial( expr.first, by ), expr.second ), multiply( expr.first, partial( expr.second, by ))); }

template< typename Expr >
auto d( Sine< Expr > expr )
{ return multiply( cosine( expr.first ), d( expr.first )); }

template< typename Expr >
auto partial( Sine< Expr > expr, Variable by )
{ return multiply( cosine( expr.first ), partial( expr.first, by )); }

template< typename Expr >
auto d( Cosine< Expr > expr )
{ return multiply( multiply( -1., sine( expr.first )), d( expr.first )); }

template< typename Expr >
auto partial( Cosine< Expr > expr, Variable by )
{ return multiply( multiply( -1., sine( expr.first )), partial( expr.first, by )); }

template< typename Expr >
auto d( Exponent< Expr > expr )
{ return multiply( expr, d( expr.first )); }

template< typename Expr >
auto partial( Exponent< Expr > expr, Variable by )
{ return multiply( expr, partial( expr.first, by )); }

template< typename Expr >
auto d( Logarithm< Expr > expr )
{ return quotient( d( expr.first ), expr.first ); }

template< typename Expr >
auto partial( Logarithm< Expr > expr, Variable by )
{ return quotient( partial( expr.first, by ), expr.first ); }

template< typename... Expr >
auto d( Formulae< Expr... > expr )
{ return formulae_derivative_helper( expr, std::make_index_sequence< sizeof...( Expr )>{} ); }

template< typename... Expr >
auto partial( Formulae< Expr... > expr, Variable by )
{ return formulae_derivative_helper( expr, by, std::make_index_sequence< sizeof...( Expr )>{} ); }

template< typename FormulaeType, size_t... Is >
auto formulae_derivative_helper( FormulaeType expr, std::index_sequence< Is... > )
{ return formulae( d( std::get< Is >( expr ))... ); }

template< typename FormulaeType, size_t... Is >
auto formulae_derivative_helper( FormulaeType expr, Variable by, std::index_sequence< Is... > )
{ return formulae( partial( std::get< Is >( expr ), by )... ); }

template< typename Expr >
auto make_partial_of( Expr expr )
{ return [&expr]( Variable by ) { return partial( expr, by ); }; }

template< typename ConstraintType, typename VariableRange >
auto jacobian( ConstraintType, VariableRange );

// jacobian is only defined on a conjunction of equal_zero expressions
// template< typename... Expr, typename VariableRange >
// auto jacobian( Conjunction< EqualsZero< Expr... >> constraints, 
//     VariableRange const& by_vars)
// { 
//     static constexpr const size_t size = sizeof...( Expr );
//     return jacobian_helper( constraints, by_vars, std::make_index_sequence< size >{} );
// }

// template< typename ConstraintsType, typename VariableRange, size_t... Is >
// auto jacobian_helper( ConstraintsType constraints, VariableRange const& by_vars, 
//     std::index_sequence< Is... > )
// { return matrix_by_rows( 
//     jacobian_row_helper( get< Is >( constraints ), by_vars )... ); }

// template< typename ConsraintType, typename VariableRange >
// auto jacobian_row_helper( ConstraintType constraint, VariableRange const& by_vars )
// { 

//         by_vars | transform( make_partial_of( constraint ));
// }


/**
 * operators namespace:  inclusion enables operators
 */
namespace operators {

template< typename Expr1, typename Expr2 >
auto pow( Expr1 base, Expr2 exponent )
{ return power( base, exponent ); }

template< typename Expr >
auto log( Expr expr )
{ return logarithm( expr ); }

template< typename Expr >
auto exp( Expr expr )
{ return exponent( expr ); }

template< typename Expr >
auto cos( Expr expr )
{ return cosine( expr ); }

template< typename Expr >
auto sin( Expr expr )
{ return sine( expr ); }

template< typename Expr1, typename Expr2 >
auto operator+( Expr1 first, Expr2 second )
{ return add( first, second ); }

template< typename Expr1, typename Expr2 >
auto operator-( Expr1 first, Expr2 second )
{ return add( first, multiply( -1.0, second )); }

template< typename Expr1, typename Expr2 >
auto operator*( Expr1 first, Expr2 second )
{ return multiply( first, second ); }

template< typename Expr1, typename Expr2 >
auto operator==( Expr1 first, Expr2 second )
{ return equals_zero( first - second ); }

} // operators


// variable collection
template< typename Expr >
set< Variable > collect_variables_from( Expr expr )
{ 
    set< Variable > collection;
    collect_variables( expr, collection );
    return collection;
}

// the compiler can clean all this up, right?  do I just constexpr all this from
// orbit?  is it the only way to be sure?
template< typename... Exprs >
void collect_variables( Conjunction< Exprs... > expr, set< Variable >& collection )
{ collect_variables_conjunction( expr, collection, std::make_index_sequence< sizeof...( Exprs )>{} ); }

template< typename ConjunctionType, size_t... Is >
void collect_variables_conjunction( ConjunctionType expr, set< Variable >& collection, std::index_sequence< Is... > )
{ ( collect_variables( std::get< Is >( expr.expressions ), collection ), ... ); }

template< typename Expr >
void collect_variables( EqualsZero< Expr > expr, set< Variable >& collection )
{ collect_variables( expr.first, collection ); }

template< typename Expr1, typename Expr2 >
void collect_variables( Add< Expr1, Expr2 > expr, set< Variable >& collection )
{
    collect_variables( expr.first, collection );
    collect_variables( expr.second, collection );
}

template< typename Expr1, typename Expr2 >
void collect_variables( Multiply< Expr1, Expr2 > expr, set< Variable >& collection )
{
    collect_variables( expr.first, collection );
    collect_variables( expr.second, collection );
}

template< typename Expr >
void collect_variables( Exponent< Expr > expr, set< Variable >& collection )
{ collect_variables( expr.first, collection ); }

template< typename Expr >
void collect_variables( Logarithm< Expr > expr, set< Variable >& collection )
{ collect_variables( expr.first, collection ); }

template< typename Expr >
void collect_variables( Sine< Expr > expr, set< Variable >& collection )
{ collect_variables( expr.first, collection ); }

template< typename Expr >
void collect_variables( Cosine< Expr > expr, set< Variable >& collection )
{ collect_variables( expr.first, collection ); }

void collect_variables( Constant expr, set< Variable >& collection )
{ }

void collect_variables( Variable expr, set< Variable >& collection )
{ collection.insert( expr ); }

template< typename... Expr >
std::tuple< Expr... > extract_constraint_expressions( 
    Conjunction< EqualsZero< Expr >... > constraint )
{ return extract_constraint_expressions_helper( constraint, 
    std::make_index_sequence< sizeof...( Expr )>{} ); }

template< typename ConstraintType, size_t... Is >
auto extract_constraint_expression_helper( ConstraintType constraint,
    std::index_sequence< Is... > )
{ return std::make_tuple( std::get< Is >( constraint.expressions )... ); }

bool has_mixed_derivatives( set< Variable > const& variables )
{
    int dots;
    for( auto i = variables.begin(); i != variables.end(); ++i )
    {
        if( i == variables.begin() )
            dots = i->dots;
        else if( dots != i->dots )
            return true;
    }
    return false;
}


template< typename ConstraintType >
VariableValues newton( ConstraintType expr, 
    double eps = 1e-4, size_t iterations = 100 );


/**
 * newton runs a 1d newton's method on the expr == 0 using auto differentiation.
 * doesn't that sound cool? 
 * - assumes variables found with collect_variables are indepdendent so 
 */
template< typename Expr >
VariableValues newton( EqualsZero< Expr > expr, 
    double eps = 1e-4, size_t iterations = 100 )
{
    set< Variable > variables;
    collect_variables( expr, variables );
    if( variables.size() != 1 )
        throw invalid_argument( "must have an equal number of formulae and"
            " variables (for now)." );

    auto f = expr.first;
    auto df = d( f );

    Variable x = *variables.begin();
    auto dx = d( x );
    
    VariableValues values;
    values[ x ] = 0.;
    values[ dx ] = eps;

    // iterate
    for( ++iterations; iterations > 0; --iterations )
    {
        // extract a vector of initial values of variables excluding differentials
        auto x0 = values[ x ];
        double fx = f( values );
        double dfx = df( values );
        double x1;

        // is this almost flat?
        if( abs( dfx / eps ) <= eps )
        {
            // guess fx next time
            x1 = x0 - fx;
        }
        else
        {
            // x_1 = x_0 - f(x_0) / f'(x_0)
            x1 = x0 - fx * eps / dfx;

            if( abs( x1 - x0 ) < eps )
                return values;
        }
        values[ x ] = x1;
    }

    return values;
}

// template< typename... Expr >
// VariableValues newton( Conjunction< EqualsZero< Expr >... > expr, double eps = 1e-4, size_t iterations = 100 )
// {
//     using std::views::transform;
//     using std::views::filter;

//     static auto assign_zero = []( Variable x ) -> std::pair< Variable, double >
//     { return { x, 0. }; };
//     static auto diff_variables = []( Variable x ) 
//     { return d( x ); };
//     static auto values_for = []( set< Variable > const& vars )
//     { return [&vars]( std::pair< Variable, double > const& var_val )
//       { return vars.contains( var_val.first ); }; };
//     static constexpr size_t constraints_size = sizeof...( Expr );

//     set< Variable > variables = collect_variables( expr );
//     if( has_mixed_derivatives( variables ))
//         throw invalid_argument( "cannot calculate newtons-method on"
//             " differentials" );

//     if( constraints_size != variables.size() )
//         throw invalid_argument( "must have an equal number of formulae and"
//             " variables (for now)." );

//     // initial guess x == 0
//     VariableValues values = variables | transform( assign_zero );
//     set< Variable > dv = variables | transform( diff_variables );
//     std::tuple< Expr... > f = extract_constraint_expressions( expr );
//     auto df = d( f );

//     // set dx == eps
//     for( auto dx : dv ) 
//         values[ dx ] = eps;

//     // iterate
//     for( ++iterations; iterations > 0; --iterations )
//     {
//         // extract a vector of initial values of variables excluding differentials
//         auto x0 = values | filter( values_for( variables )) | transform( std::get< 1 > );
//         double fx = f( values );
//         double dfx = df( values );
//         double x1;

//         // is this almost flat?
//         if( abs( dfx / eps ) <= eps )
//         {
//             // guess fx next time
//             x1 = x0 - fx;
//         }
//         else
//         {
//             // x_1 = x_0 - f(x_0) / f'(x_0)
//             x1 = x0 - fx * eps / dfx;

//             if( abs( x1 - x0 ) < eps )
//                 return values;
//         }
//         values[ x ] = x1;
//     }

//     return values;
// }

// input/output
std::ostream& operator<<( std::ostream& os, Constant const& val )
{ return os << val.value; }

std::ostream& operator<<( std::ostream& os, Variable const& var )
{
    for( int i = 0; i > var.dots; --i )
        os << "'";
    os << var.name;
    for( int i = 0; i < var.dots; ++i )
        os << "'";

    return os;
}

template< typename Expr1, typename Expr2 >
std::ostream& operator<<( std::ostream& os, Multiply< Expr1, Expr2 > const& mul )
{ return os << "( " << mul.first << " * " << mul.second << " )"; }

template< typename Expr1, typename Expr2 >
std::ostream& operator<<( std::ostream& os, Add< Expr1, Expr2 > const& expr )
{ return os << "( " << expr.first << " + " << expr.second << " )"; }

template< typename Expr >
std::ostream& operator<<( std::ostream& os, Sine< Expr > const& expr )
{ return os << "sin( " << expr.first << " )"; }

template< typename Expr >
std::ostream& operator<<( std::ostream& os, Cosine< Expr > const& expr )
{ return os << "cos( " << expr.first << " )"; }

template< typename Expr >
std::ostream& operator<<( std::ostream& os, Exponent< Expr > const& expr )
{ return os << "exp( " << expr.first << " )"; }

template< typename Expr >
std::ostream& operator<<( std::ostream& os, Logarithm< Expr > const& expr )
{ return os << "log( " << expr.first << " )"; }

template< typename Expr >
std::ostream& operator<<( std::ostream& os, EqualsZero< Expr > const& expr )
{ return os << expr.first << " == 0"; }


int main( int ac, char* av[] )
{
    using std::cout, std::endl;
    using namespace operators;

    auto x = Variable{ "x" };
    auto y = Variable{ "y" };
    auto zero = Constant{ 0. };
    auto one = Constant{ 1. };
    auto two = Constant{ 2. };
    auto pi = Constant{ std::numbers::pi };
    auto million = Constant{ 1'000'000. };

    // example from https://en.wikipedia.org/wiki/Newton%27s_method#Solution_of_cos(x)_=_x3_using_Newton's_method
    auto f = ( cos(x) - x*x*x );
    // TODO: this doesn't work right now
    // auto f = ( cos(x) - pow(x, 3.) );

    auto values = newton( EqualsZero{ f });

    cout << "  f   == " << f << endl;
    cout << " df   == " << d( f ) << endl;
    cout << " solving " << ( f == 0 ) << endl;
    cout << " x    == " << values[ x ] << endl;
    cout << " f(x) == " << f(values) << endl;


    return EXIT_SUCCESS;
}