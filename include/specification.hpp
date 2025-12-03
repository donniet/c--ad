#ifndef __SPECIFICATION_HPP__
#define __SPECIFICATION_HPP__

namespace specification 
{

template<typename T>
struct value
{
    using value_type = T;
};

template<class V>
concept value_type = requires (V v)
{
    typename V::value_type;
    typename V::value_type x;
    x = v;
    v = x;
};

template<value_type V, value_constraint C>
struct constrained
{

};


} // namespace specification


#endif