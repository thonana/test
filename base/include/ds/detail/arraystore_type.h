#pragma once

#include "ds/array.h"
#include <H5Cpp.h>
#include <string>

namespace ds {
namespace detail {

static constexpr size_t the_arraystore_chunk_size = 1048576;
static constexpr size_t the_arraystore_vlen_chunk_size =
  the_arraystore_chunk_size / 32;

template<typename>
struct ArrayStoreType;

// char
template<>
struct ArrayStoreType<char>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(char);
    const H5::PredType obj{ H5::PredType::NATIVE_CHAR };
};

template<>
struct ArrayStoreType<Array<char>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(char);
    const H5::VarLenType obj{ H5::PredType::NATIVE_CHAR };
};

template<>
struct ArrayStoreType<ArrayPtr<char>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(char);
    const H5::VarLenType obj{ H5::PredType::NATIVE_CHAR };
};

// signed char
template<>
struct ArrayStoreType<signed char>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(signed char);
    const H5::PredType obj{ H5::PredType::NATIVE_SCHAR };
};

template<>
struct ArrayStoreType<Array<signed char>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(signed char);
    const H5::VarLenType obj{ H5::PredType::NATIVE_SCHAR };
};

template<>
struct ArrayStoreType<ArrayPtr<signed char>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(signed char);
    const H5::VarLenType obj{ H5::PredType::NATIVE_SCHAR };
};

// unsigned char
template<>
struct ArrayStoreType<unsigned char>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(unsigned char);
    const H5::PredType obj{ H5::PredType::NATIVE_UCHAR };
};

template<>
struct ArrayStoreType<Array<unsigned char>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(unsigned char);
    const H5::VarLenType obj{ H5::PredType::NATIVE_UCHAR };
};

template<>
struct ArrayStoreType<ArrayPtr<unsigned char>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(unsigned char);
    const H5::VarLenType obj{ H5::PredType::NATIVE_UCHAR };
};

// singed short
template<>
struct ArrayStoreType<signed short>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(signed short);
    const H5::PredType obj{ H5::PredType::NATIVE_SHORT };
};

template<>
struct ArrayStoreType<Array<signed short>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(signed short);
    const H5::VarLenType obj{ H5::PredType::NATIVE_SHORT };
};

template<>
struct ArrayStoreType<ArrayPtr<signed short>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(signed short);
    const H5::VarLenType obj{ H5::PredType::NATIVE_SHORT };
};

// unsigned short
template<>
struct ArrayStoreType<unsigned short>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(unsigned short);
    const H5::PredType obj{ H5::PredType::NATIVE_USHORT };
};

template<>
struct ArrayStoreType<Array<unsigned short>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(unsigned short);
    const H5::VarLenType obj{ H5::PredType::NATIVE_USHORT };
};

template<>
struct ArrayStoreType<ArrayPtr<unsigned short>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(unsigned short);
    const H5::VarLenType obj{ H5::PredType::NATIVE_USHORT };
};

// signed int
template<>
struct ArrayStoreType<signed int>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(signed int);
    const H5::PredType obj{ H5::PredType::NATIVE_INT };
};

template<>
struct ArrayStoreType<Array<signed int>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(signed int);
    const H5::VarLenType obj{ H5::PredType::NATIVE_INT };
};

template<>
struct ArrayStoreType<ArrayPtr<signed int>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(signed int);
    const H5::VarLenType obj{ H5::PredType::NATIVE_INT };
};

// unsigned int
template<>
struct ArrayStoreType<unsigned int>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(unsigned int);
    const H5::PredType obj{ H5::PredType::NATIVE_UINT };
};

template<>
struct ArrayStoreType<Array<unsigned int>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(unsigned int);
    const H5::VarLenType obj{ H5::PredType::NATIVE_UINT };
};

template<>
struct ArrayStoreType<ArrayPtr<unsigned int>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(unsigned int);
    const H5::VarLenType obj{ H5::PredType::NATIVE_UINT };
};

// signed long
template<>
struct ArrayStoreType<signed long>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(signed long);
    const H5::PredType obj{ H5::PredType::NATIVE_LONG };
};

template<>
struct ArrayStoreType<Array<signed long>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(signed long);
    const H5::VarLenType obj{ H5::PredType::NATIVE_LONG };
};

template<>
struct ArrayStoreType<ArrayPtr<signed long>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(signed long);
    const H5::VarLenType obj{ H5::PredType::NATIVE_LONG };
};

// unsigned long
template<>
struct ArrayStoreType<unsigned long>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(unsigned long);
    const H5::PredType obj{ H5::PredType::NATIVE_ULONG };
};

template<>
struct ArrayStoreType<Array<unsigned long>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(unsigned long);
    const H5::VarLenType obj{ H5::PredType::NATIVE_ULONG };
};

template<>
struct ArrayStoreType<ArrayPtr<unsigned long>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(unsigned long);
    const H5::VarLenType obj{ H5::PredType::NATIVE_ULONG };
};

// signed long long
template<>
struct ArrayStoreType<signed long long>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(signed long long);
    const H5::PredType obj{ H5::PredType::NATIVE_LLONG };
};

template<>
struct ArrayStoreType<Array<signed long long>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(signed long long);
    const H5::VarLenType obj{ H5::PredType::NATIVE_LLONG };
};

template<>
struct ArrayStoreType<ArrayPtr<signed long long>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(signed long long);
    const H5::VarLenType obj{ H5::PredType::NATIVE_LLONG };
};

// unsigned long long
template<>
struct ArrayStoreType<unsigned long long>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(unsigned long long);
    const H5::PredType obj{ H5::PredType::NATIVE_ULLONG };
};

template<>
struct ArrayStoreType<Array<unsigned long long>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(unsigned long long);
    const H5::VarLenType obj{ H5::PredType::NATIVE_ULLONG };
};

template<>
struct ArrayStoreType<ArrayPtr<unsigned long long>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(unsigned long long);
    const H5::VarLenType obj{ H5::PredType::NATIVE_ULLONG };
};

// float
template<>
struct ArrayStoreType<float>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(float);
    const H5::PredType obj{ H5::PredType::NATIVE_FLOAT };
};

template<>
struct ArrayStoreType<Array<float>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(float);
    const H5::VarLenType obj{ H5::PredType::NATIVE_FLOAT };
};

template<>
struct ArrayStoreType<ArrayPtr<float>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(float);
    const H5::VarLenType obj{ H5::PredType::NATIVE_FLOAT };
};

// double
template<>
struct ArrayStoreType<double>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(double);
    const H5::PredType obj{ H5::PredType::NATIVE_DOUBLE };
};

template<>
struct ArrayStoreType<Array<double>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(double);
    const H5::VarLenType obj{ H5::PredType::NATIVE_DOUBLE };
};

template<>
struct ArrayStoreType<ArrayPtr<double>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(double);
    const H5::VarLenType obj{ H5::PredType::NATIVE_DOUBLE };
};

// long double
template<>
struct ArrayStoreType<long double>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_chunk_size / sizeof(long double);
    const H5::PredType obj{ H5::PredType::NATIVE_LDOUBLE };
};

template<>
struct ArrayStoreType<Array<long double>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(long double);
    const H5::VarLenType obj{ H5::PredType::NATIVE_LDOUBLE };
};

template<>
struct ArrayStoreType<ArrayPtr<long double>>
{
    static constexpr size_t default_chunk_size =
      the_arraystore_vlen_chunk_size / sizeof(long double);
    const H5::VarLenType obj{ H5::PredType::NATIVE_LDOUBLE };
};

// string
template<>
struct ArrayStoreType<std::string>
{
    static constexpr size_t default_chunk_size = the_arraystore_vlen_chunk_size;
    const H5::StrType       obj{ 0, H5T_VARIABLE };

    ArrayStoreType()
    {
        obj.setCset(H5T_CSET_UTF8);
        obj.setStrpad(H5T_STR_NULLTERM);
    }
};

} // namespace detail
} // namespace ds