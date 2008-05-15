//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_MYSQL_SOURCE
#include "soci-mysql.h"
#include "common.h"
#include <soci-platform.h>
#include <ciso646>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using namespace soci::details::mysql;


void mysql_vector_into_type_backend::define_by_pos(
    int &position, void *data, exchange_type type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void mysql_vector_into_type_backend::pre_fetch()
{
    // nothing to do here
}

namespace // anonymous
{

template <typename T>
void set_invector_(void *p, int indx, T const &val)
{
    std::vector<T> *dest =
        static_cast<std::vector<T> *>(p);

    std::vector<T> &v = *dest;
    v[indx] = val;
}

} // namespace anonymous

void mysql_vector_into_type_backend::post_fetch(bool gotData, indicator *ind)
{
    if (gotData)
    {
        // Here, rowsToConsume_ in the Statement object designates
        // the number of rows that need to be put in the user's buffers.

        // PostgreSQL column positions start at 0
        int pos = position_ - 1;

        int const endRow = statement_.currentRow_ + statement_.rowsToConsume_;

        mysql_data_seek(statement_.result_, statement_.currentRow_);
        for (int curRow = statement_.currentRow_, i = 0;
             curRow != endRow; ++curRow, ++i)
        {
            MYSQL_ROW row = mysql_fetch_row(statement_.result_);
            // first, deal with indicators
            if (row[pos] == NULL)
            {
                if (ind == NULL)
                {
                    throw soci_error(
                        "Null value fetched and no indicator defined.");
                }

                ind[i] = i_null;
            }
            else
            {
                if (ind != NULL)
                {
                    ind[i] = i_ok;
                }
            }

            // buffer with data retrieved from server, in text format
            const char *buf = row[pos] != NULL ? row[pos] : "";

            switch (type_)
            {
            case x_char:
                set_invector_(data_, i, *buf);
                break;
            case x_stdstring:
                set_invector_<std::string>(data_, i, buf);
                break;
            case x_short:
                {
                    long val = std::strtol(buf, NULL, 10);
                    set_invector_(data_, i, static_cast<short>(val));
                }
                break;
            case x_integer:
                {
                    long val = std::strtol(buf, NULL, 10);
                    set_invector_(data_, i, static_cast<int>(val));
                }
                break;
            case x_unsigned_long:
                {
                    long long val = std::strtoll(buf, NULL, 10);
                    set_invector_(data_, i, static_cast<unsigned long>(val));
                }
                break;
            case x_long_long:
                {
                    long long val = std::strtoll(buf, NULL, 10);
                    set_invector_(data_, i, val);
                }
                break;
            case x_double:
                {
                    double val = strtod(buf, NULL);
                    set_invector_(data_, i, val);
                }
                break;
            case x_stdtm:
                {
                    // attempt to parse the string and convert to std::tm
                    std::tm t;
                    parse_std_tm(buf, t);

                    set_invector_(data_, i, t);
                }
                break;

            default:
                throw soci_error("Into element used with non-supported type.");
            }
        }
    }
    else // no data retrieved
    {
        // nothing to do, into vectors are already truncated
    }
}

namespace // anonymous
{

template <typename T>
void resizevector_(void *p, std::size_t sz)
{
    std::vector<T> *v = static_cast<std::vector<T> *>(p);
    v->resize(sz);
}

} // namespace anonymous

void mysql_vector_into_type_backend::resize(std::size_t sz)
{
    switch (type_)
    {
        // simple cases
    case x_char:         resizevector_<char>         (data_, sz); break;
    case x_short:        resizevector_<short>        (data_, sz); break;
    case x_integer:      resizevector_<int>          (data_, sz); break;
    case x_unsigned_long: resizevector_<unsigned long>(data_, sz); break;
    case x_long_long:     resizevector_<long long>    (data_, sz); break;
    case x_double:       resizevector_<double>       (data_, sz); break;
    case x_stdstring:    resizevector_<std::string>  (data_, sz); break;
    case x_stdtm:        resizevector_<std::tm>      (data_, sz); break;

    default:
        throw soci_error("Into vector element used with non-supported type.");
    }
}

std::size_t mysql_vector_into_type_backend::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
        // simple cases
    case x_char:         sz = get_vector_size<char>         (data_); break;
    case x_short:        sz = get_vector_size<short>        (data_); break;
    case x_integer:      sz = get_vector_size<int>          (data_); break;
    case x_unsigned_long: sz = get_vector_size<unsigned long>(data_); break;
    case x_long_long:     sz = get_vector_size<long long>    (data_); break;
    case x_double:       sz = get_vector_size<double>       (data_); break;
    case x_stdstring:    sz = get_vector_size<std::string>  (data_); break;
    case x_stdtm:        sz = get_vector_size<std::tm>      (data_); break;

    default:
        throw soci_error("Into vector element used with non-supported type.");
    }

    return sz;
}

void mysql_vector_into_type_backend::clean_up()
{
    // nothing to do here
}
