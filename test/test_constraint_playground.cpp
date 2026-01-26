#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <variant>
#include <vector>
#include <tuple>

using std::shared_ptr, std::make_shared, std::dynamic_pointer_cast;
using std::optional;
using std::variant, std::monostate, std::holds_alternative;
using std::vector;
using std::pair, std::tuple, std::get, std::tie;

// static constexpr double undefined = std::numeric_limits< double >::min();

struct Expression;
struct Constraint;

struct Equal;
struct Addition;
struct Conjunction;
struct Disjunction;
struct Negation;
struct Variable;
struct Constant;

struct Evaluator
{
    virtual bool variable( Variable& ) { return false; }
    virtual bool equal( Equal& ) { return false; }
    virtual bool add( Addition& ) { return false; }
    virtual bool conjunction( Conjunction& ) { return false; }
    virtual bool disjunction( Disjunction& ) { return false; }
    virtual bool negation( Negation& ) { return false; }
    virtual bool constant( Constant& ) { return false; }
};

struct Evalable 
{
    // eval visits constraints and expressions
    // returns false to stop evaluating 
    virtual bool eval( Evaluator& ) { return false; } 
};

struct Expression : virtual Evalable
{ 
    virtual double value() = 0;
};

struct Constraint : virtual Evalable
{ 
    virtual bool value() = 0;
};

struct Variable : virtual Expression
{ 
    bool eval( Evaluator& visitor ) override
    { return visitor.variable( *this ); }

    double value() override { return *_value; }

    void set_value( double new_value ) { _value = new_value; }
private:
    optional< double > _value;
};

struct Addition : virtual Expression
{
    bool eval( Evaluator& visitor ) override 
    {
        if( not first->eval( visitor ))
            return false;

        if( not second->eval( visitor ))
            return false;
        
        return visitor.add( *this );
    }

    shared_ptr< Expression > first;
    shared_ptr< Expression > second;
};

struct Constant : virtual Expression
{
    bool eval( Evaluator& visitor ) override
    { return visitor.constant( *this ); }

    double value() override { return constant_value; }

    double constant_value;
};

struct Equal : virtual Constraint
{
    bool eval( Evaluator& visitor ) override
    {
        if( not first->eval( visitor ))
            return false;
        
        if( not second->eval( visitor ))
            return false;

        return visitor.equal( *this );
    }

    template< typename ExpType1, typename ExpType2 >
    Equal( shared_ptr< ExpType1 > first, shared_ptr< ExpType2 > second ) :
        first{ dynamic_pointer_cast< Expression >( first )},
        second{ dynamic_pointer_cast< Expression >( second )}
    { }

    shared_ptr< Expression > first;
    shared_ptr< Expression > second;
};

struct Conjunction : virtual Constraint
{
    bool eval( Evaluator& visitor ) override
    { 
        if( not first->eval( visitor ))
            return false;
        
        if( not second->eval( visitor ))
            return false;

        return visitor.conjunction( *this );
    }

    shared_ptr< Constraint > first;
    shared_ptr< Constraint > second;
};

struct Disjunction : virtual Constraint
{
    bool eval( Evaluator& visitor ) override
    {  
        if( not first->eval( visitor ))
            return false;

        if( not second->eval( visitor ))
            return false;

        return visitor.disjunction( *this );
    }

    shared_ptr< Constraint > first;
    shared_ptr< Constraint > second;
};

struct Negation : virtual Constraint
{
    bool eval( Evaluator& visitor ) override
    {
        if( not argument->eval( visitor ))
            return false;
        
        return visitor.negation( *this );
    }

    shared_ptr< Constraint > argument;
};

struct VariableEvaluator : virtual Evaluator
{
    using work_type = variant< monostate, 
        bool, double, Variable* >;
    using stack_type = vector< work_type >;

    // stack helpers
    template< typename T >
    optional< T > peek()
    {
        if( stack.size() == 0 )
            return {};

        auto item = stack.back();
        if( not has_alternative< T >( item ))
            return {};

        return get< T >( item );
    }

    work_type peek_any() { return stack.back(); }

    void pop()
    { stack.pop_back(); }

    bool is_expression( work_type item )
    { return holds_alternative< double >( item ) || holds_alternative< Variable* >( item ); }


    bool variable( Variable& ) override { return false; }
    bool equal( Equal& expr ) override
    {
        // pop the last two things off the stack.
        // if they are both doubles, push the result of an equality test
        // if one is a variable with no value, set the value of the variable
        //    to the double, push true on the stack
        // if both are variables
        //    if only one has a value, set it equal, along with any other 
        //       variables with no value that are marked as equivalent to the 
        //       other and push true
        //    if both have a value, push an equality test of both onto the stack
        //    if neither have a value, store the fact that both must be equal
        //       and push true onto the stack.
    }
    bool add( Addition& expr ) override
    { 
        // we can add double's and variables but not constraints
        auto first = peek_any();
        if( not is_expression( first ))
            return false;
        pop();

        auto second = peek_any();
        if( not is_expression( first ))
        {
            // return the stack to the original state
            stack.push_back( first );
            return false;
        }
        pop();

        // ex: 0.1 + 1.5
        if( holds_alternative< double >( first ) and holds_alternative< double >( second ))
        {
            stack.push_back( get< double >( first ) + get< double >( second ));
            return true;
        }
        // ex: 5. + x
        if( holds_alternative< double >( first ) and holds_alternative< Variable* >( second ))
        {
            Variable* var = get< Variable* >( second );
            add_to_variable( var, get< double >( first ));
            stack.push_back( var );
            return true;
        }
        // ex: x + 5.
        if( holds_alternative< Variable* >( first ) and holds_alternative< double >( second ))
        {
            Variable* var = get< Variable* >( second );
            add_to_variable( var, get< double >( first ));
            stack.push_back( var );
            return true;
        }
        
        return true;
    }
    bool conjunction( Conjunction& ) override { return false; }
    bool disjunction( Disjunction& ) override { return false; }
    bool negation( Negation& ) override { return false; }
    bool constant( Constant& expr ) override
    { stack.push_back( expr.value() ); return true; }

    stack_type stack;
};

int main( int ac, char* av[] )
{
    double width;

    auto x = make_shared< Variable >();

    auto ten = make_shared< Constant >( 10. );
    auto two = make_shared< Constant >(  2. );
    auto pi =  make_shared< Constant >( std::numbers::pi );

    auto ten_plus_two = make_shared< Addition >( ten, two );
    auto x_equal_twelve = make_shared< Equal >( x, ten_plus_two );

    auto visitor = VariableEvaluator{};

    if( x_equal_twelve->eval( visitor ))
        std::cout << x->value() << std::endl;
    else
        std::cout << "constraint not met" << std::endl;

    return EXIT_SUCCESS;
}