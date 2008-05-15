//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_INTO_TYPE_H_INCLUDED
#define SOCI_INTO_TYPE_H_INCLUDED

#include "soci-backend.h"
#include "type-ptr.h"
#include "exchange-traits.h"

#include <string>
#include <vector>

namespace soci
{

class session;

namespace details
{

class prepare_temp_type;
class standard_into_type_backend;
class vector_into_type_backend;
class statement_impl;

// this is intended to be a base class for all classes that deal with
// defining output data
class into_type_base
{
public:
    virtual ~into_type_base() {}

    virtual void define(statement_impl & st, int & position) = 0;
    virtual void pre_fetch() = 0;
    virtual void post_fetch(bool gotData, bool calledFromFetch) = 0;
    virtual void clean_up() = 0;

    virtual std::size_t size() const = 0;  // returns the number of elements
    virtual void resize(std::size_t /* sz */) {} // used for vectors only
};

typedef type_ptr<into_type_base> into_type_ptr;

// standard types

class SOCI_DECL standard_into_type : public into_type_base
{
public:
    standard_into_type(void * data, exchange_type type)
        : data_(data), type_(type), ind_(NULL), backEnd_(NULL) {}
    standard_into_type(void * data, exchange_type type, indicator & ind)
        : data_(data), type_(type), ind_(&ind), backEnd_(NULL) {}

    virtual ~standard_into_type();

protected:
    virtual void post_fetch(bool gotData, bool calledFromFetch);

private:
    virtual void define(statement_impl & st, int & position);
    virtual void pre_fetch();
    virtual void clean_up();

    virtual std::size_t size() const { return 1; }

    // conversion hook (from base type to arbitrary user type)
    virtual void convert_from_base() {}

    void * data_;
    exchange_type type_;
    indicator * ind_;

    standard_into_type_backend * backEnd_;
};

// into type base class for vectors
class SOCI_DECL vector_into_type : public into_type_base
{
public:
    vector_into_type(void * data, exchange_type type)
        : data_(data), type_(type), indVec_(NULL), backEnd_(NULL) {}

    vector_into_type(void * data, exchange_type type,
        std::vector<indicator> & ind)
        : data_(data), type_(type), indVec_(&ind), backEnd_(NULL) {}

    ~vector_into_type();

protected:
    virtual void post_fetch(bool gotData, bool calledFromFetch);

private:
    virtual void define(statement_impl & st, int & position);
    virtual void pre_fetch();
    virtual void clean_up();
    virtual void resize(std::size_t sz);
    virtual std::size_t size() const;

    void * data_;
    exchange_type type_;
    std::vector<indicator> * indVec_;

    vector_into_type_backend * backEnd_;

    virtual void convert_from_base() {}
};

// implementation for the basic types (those which are supported by the library
// out of the box without user-provided conversions)

template <typename T>
class into_type : public standard_into_type
{
public:
    into_type(T & t)
        : standard_into_type(&t,
            static_cast<exchange_type>(exchange_traits<T>::x_type)) {}
    into_type(T & t, indicator & ind)
        : standard_into_type(&t,
            static_cast<exchange_type>(exchange_traits<T>::x_type), ind) {}
};

template <typename T>
class into_type<std::vector<T> > : public vector_into_type
{
public:
    into_type(std::vector<T> & v)
        : vector_into_type(&v,
            static_cast<exchange_type>(exchange_traits<T>::x_type)) {}
    into_type(std::vector<T> & v, std::vector<indicator> & ind)
        : vector_into_type(&v,
            static_cast<exchange_type>(exchange_traits<T>::x_type), ind) {}
};

// special cases for char* and char[]

template <>
class into_type<char *> : public standard_into_type
{
public:
    into_type(char * str, std::size_t bufSize)
        : standard_into_type(&str_, x_cstring), str_(str, bufSize) {}
    into_type(char * str, indicator & ind, std::size_t bufSize)
        : standard_into_type(&str_, x_cstring, ind), str_(str, bufSize) {}

private:
    cstring_descriptor str_;
};

template <std::size_t N>
class into_type<char[N]> : public into_type<char *>
{
public:
    into_type(char str[]) : into_type<char *>(str, N) {}
    into_type(char str[], indicator & ind)
        : into_type<char *>(str, ind, N) {}
};

// helper dispatchers for basic types

template <typename T>
into_type_ptr do_into(T & t, basic_type_tag)
{
    return into_type_ptr(new into_type<T>(t));
}

template <typename T>
into_type_ptr do_into(T & t, indicator & ind, basic_type_tag)
{
    return into_type_ptr(new into_type<T>(t, ind));
}

template <typename T>
into_type_ptr do_into(T & t, std::vector<indicator> & ind, basic_type_tag)
{
    return into_type_ptr(new into_type<T>(t, ind));
}

} // namespace details

} // namespace SOCI

#endif // SOCI_INTO_TYPE_H_INCLUDED
