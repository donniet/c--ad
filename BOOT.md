# Bootstrapping of Expressions

The expression library is inherently self-referential and requires bootstrapping of fundamental operations.  The most important of these operations is `Substitution` which must be declared prior to compound expressions, but cannot be implemented until afterward.

## Variable Order

The order of an expression is the maximum order of the variables in the expression. The order of a variable is 1 + the order of the variable's `value_type`, or the number of substitutions of zero-order expressions that must be made into the variable before it can be evaluated. Substitution directly into a variable increases the order of the variable by one by replacing the `value_type` with the Substitution Expression.