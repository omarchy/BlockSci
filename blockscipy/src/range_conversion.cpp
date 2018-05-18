//
//  range_conversion.cpp
//  blocksci_interface
//
//  Created by Harry Kalodner on 4/21/18.
//

#include "range_conversion.hpp"

#include <blocksci/chain/block.hpp>
#include <blocksci/core/bitcoin_uint256.hpp>
#include <blocksci/address/equiv_address.hpp>
#include <blocksci/scripts/script_variant.hpp>
#include <blocksci/cluster/cluster.hpp>

#include <pybind11/pybind11.h>

#include <range/v3/range_for.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/filter.hpp>

#include <chrono>

using namespace blocksci;
namespace py = pybind11;

struct NumpyConverter {

    template <typename T>
    T operator()(const T &val) {
        return val;
    }

    NumpyDatetime operator()(const std::chrono::system_clock::time_point &val) {
        return {std::chrono::duration_cast<std::chrono::nanoseconds>(val.time_since_epoch()).count()};
    }

    std::array<char, 64> operator()(const blocksci::uint256 &val) {
        auto hexStr = val.GetHex();
        std::array<char, 64> ret;
        std::copy_n(hexStr.begin(), 64, ret.begin());
        return ret;
    }

    std::array<char, 40> operator()(const blocksci::uint160 &val) {
        auto hexStr = val.GetHex();
        std::array<char, 40> ret;
        std::copy_n(hexStr.begin(), 40, ret.begin());
        return ret;
    }

    NumpyBool operator()(const bool &val) {
        return {val};
    }
};

struct BlockSciTypeConverter {
    blocksci::AnyScript operator()(const blocksci::Address &address) {
        return address.getScript();
    }

    int64_t operator()(uint16_t val) {
        return static_cast<int64_t>(val);
    }

    int64_t operator()(int16_t val) {
        return static_cast<int64_t>(val);
    }

    int64_t operator()(uint32_t val) {
        return static_cast<int64_t>(val);
    }

    int64_t operator()(int32_t val) {
        return static_cast<int64_t>(val);
    }

    // Potential overflow
    int64_t operator()(uint64_t val) {
        return static_cast<int64_t>(val);
    }

    template <typename T>
    auto operator()(const ranges::optional<T> &val) -> ranges::optional<decltype(this->operator()(*val))> {
        if (val) {
            return this->operator()(*val);
        } else {
            return ranges::nullopt;
        }
    }
};

template <typename T>
pybind11::array_t<decltype(NumpyConverter{}(std::declval<ranges::range_value_type_t<T>>()))>
convertRandomSizedNumpy(T && t) {
    auto numpy_converted = ranges::view::transform(std::move(t), NumpyConverter{});
    auto rangeSize = static_cast<size_t>(ranges::size(numpy_converted));
    pybind11::array_t<ranges::range_value_type_t<decltype(numpy_converted)>> ret{rangeSize};
    auto retPtr = ret.mutable_data();
    ranges::copy(numpy_converted, retPtr);
    return ret;
}

template <typename T>
pybind11::array_t<decltype(NumpyConverter{}(std::declval<ranges::range_value_type_t<T>>()))>
convertInputNumpy(T && t) {
    auto ret = ranges::to_vector(ranges::view::transform(std::move(t), NumpyConverter{}));
    return pybind11::array_t<typename decltype(ret)::value_type>{ret.size(), ret.data()};
}

template <typename T>
py::list convertRandomSizedPy(T && t) {
    auto rangeSize = static_cast<size_t>(ranges::size(t));
    pybind11::list list{rangeSize};
    RANGES_FOR(auto && a, t) {
        list.append(std::forward<decltype(a)>(a));
    }
    return list;
}

template <typename T>
py::list convertInputPy(T && t) {
    pybind11::list list;
    RANGES_FOR(auto && a, std::move(t)) {
        list.append(std::forward<decltype(a)>(a));
    }
    return list;
}

template <typename T>
ranges::any_view<decltype(BlockSciTypeConverter{}(std::declval<ranges::range_value_type_t<T>>()))>
convertInputBlockSci(T && t) {
    return {ranges::view::transform(std::forward<T>(t), BlockSciTypeConverter{})};
}

template <typename T>
ranges::any_view<decltype(BlockSciTypeConverter{}(std::declval<ranges::range_value_type_t<T>>())), random_access_sized>
convertRandomSizedBlockSci(T && t) {
    return {ranges::view::transform(std::forward<T>(t), BlockSciTypeConverter{})};
}

template <typename T>
ranges::any_view<ranges::range_value_t<typename ranges::range_value_t<T>::value_type>> flattenNestedOptional(T && t) {
    return {ranges::view::join(ranges::view::transform(ranges::view::filter(std::forward<T>(t), isOptional), derefOptional))};
}

ranges::any_view<ranges::optional<int64_t>> PythonConversionTypeConverter::operator()(ranges::any_view<ranges::optional<int16_t>> && t) { return convertInputBlockSci(std::move(t)); }
ranges::any_view<ranges::optional<int64_t>> PythonConversionTypeConverter::operator()(ranges::any_view<ranges::optional<uint16_t>> && t) { return convertInputBlockSci(std::move(t)); }
ranges::any_view<ranges::optional<int64_t>> PythonConversionTypeConverter::operator()(ranges::any_view<ranges::optional<int32_t>> && t) { return convertInputBlockSci(std::move(t)); }
ranges::any_view<ranges::optional<int64_t>> PythonConversionTypeConverter::operator()(ranges::any_view<ranges::optional<uint32_t>> && t) { return convertInputBlockSci(std::move(t)); }
ranges::any_view<ranges::optional<int64_t>> PythonConversionTypeConverter::operator()(ranges::any_view<ranges::optional<uint64_t>> && t) { return convertInputBlockSci(std::move(t)); }
ranges::any_view<blocksci::AnyScript> PythonConversionTypeConverter::operator()(ranges::any_view<blocksci::Address> && t) { return convertInputBlockSci(std::move(t)); }
ranges::any_view<ranges::optional<blocksci::AnyScript>> PythonConversionTypeConverter::operator()(ranges::any_view<ranges::optional<blocksci::Address>> && t) { return convertInputBlockSci(std::move(t)); }

ranges::any_view<ranges::optional<int64_t>, random_access_sized> PythonConversionTypeConverter::operator()(ranges::any_view<ranges::optional<int16_t>, random_access_sized> && t) { return convertRandomSizedBlockSci(std::move(t)); }
ranges::any_view<ranges::optional<int64_t>, random_access_sized> PythonConversionTypeConverter::operator()(ranges::any_view<ranges::optional<uint16_t>, random_access_sized> && t) { return convertRandomSizedBlockSci(std::move(t)); }
ranges::any_view<ranges::optional<int64_t>, random_access_sized> PythonConversionTypeConverter::operator()(ranges::any_view<ranges::optional<int32_t>, random_access_sized> && t) { return convertRandomSizedBlockSci(std::move(t)); }
ranges::any_view<ranges::optional<int64_t>, random_access_sized> PythonConversionTypeConverter::operator()(ranges::any_view<ranges::optional<uint32_t>, random_access_sized> && t) { return convertRandomSizedBlockSci(std::move(t)); }
ranges::any_view<ranges::optional<int64_t>, random_access_sized> PythonConversionTypeConverter::operator()(ranges::any_view<ranges::optional<uint64_t>, random_access_sized> && t) { return convertRandomSizedBlockSci(std::move(t)); }
ranges::any_view<blocksci::AnyScript, random_access_sized> PythonConversionTypeConverter::operator()(ranges::any_view<blocksci::Address, random_access_sized> && t) { return convertRandomSizedBlockSci(std::move(t)); }
ranges::any_view<ranges::optional<blocksci::AnyScript>, random_access_sized> PythonConversionTypeConverter::operator()(ranges::any_view<ranges::optional<blocksci::Address>, random_access_sized> && t) { return convertRandomSizedBlockSci(std::move(t)); }

pybind11::list PythonConversionTypeConverter::operator()(ranges::any_view<pybind11::bytes> && t) { return convertInputPy(std::move(t)); }
pybind11::list PythonConversionTypeConverter::operator()(ranges::any_view<pybind11::list> && t) { return convertInputPy(std::move(t)); }
pybind11::list PythonConversionTypeConverter::operator()(ranges::any_view<std::string> && t) { return convertInputPy(std::move(t)); }
pybind11::list PythonConversionTypeConverter::operator()(ranges::any_view<blocksci::AddressType::Enum> && t) { return convertInputPy(std::move(t)); }

pybind11::list PythonConversionTypeConverter::operator()(ranges::any_view<pybind11::bytes, random_access_sized> && t) { return convertRandomSizedPy(std::move(t)); }
pybind11::list PythonConversionTypeConverter::operator()(ranges::any_view<pybind11::list, random_access_sized> && t) { return convertRandomSizedPy(std::move(t)); }
pybind11::list PythonConversionTypeConverter::operator()(ranges::any_view<std::string, random_access_sized> && t) { return convertRandomSizedPy(std::move(t)); }
pybind11::list PythonConversionTypeConverter::operator()(ranges::any_view<blocksci::AddressType::Enum, random_access_sized> && t) { return convertRandomSizedPy(std::move(t)); }

pybind11::array_t<int64_t> PythonConversionTypeConverter::operator()(ranges::any_view<int64_t> && t) { return convertInputNumpy(std::move(t)); }
pybind11::array_t<uint64_t> PythonConversionTypeConverter::operator()(ranges::any_view<uint64_t> && t) { return convertInputNumpy(std::move(t)); }
pybind11::array_t<int32_t> PythonConversionTypeConverter::operator()(ranges::any_view<int32_t> && t) { return convertInputNumpy(std::move(t)); }
pybind11::array_t<uint32_t> PythonConversionTypeConverter::operator()(ranges::any_view<uint32_t> && t) { return convertInputNumpy(std::move(t)); }
pybind11::array_t<int16_t> PythonConversionTypeConverter::operator()(ranges::any_view<int16_t> && t) { return convertInputNumpy(std::move(t)); }
pybind11::array_t<uint16_t> PythonConversionTypeConverter::operator()(ranges::any_view<uint16_t> && t) { return convertInputNumpy(std::move(t)); }
pybind11::array_t<NumpyBool> PythonConversionTypeConverter::operator()(ranges::any_view<bool> && t) { return convertInputNumpy(std::move(t)); }
pybind11::array_t<NumpyDatetime> PythonConversionTypeConverter::operator()(ranges::any_view<std::chrono::system_clock::time_point> && t) { return convertInputNumpy(std::move(t)); }
pybind11::array_t<std::array<char, 40>> PythonConversionTypeConverter::operator()(ranges::any_view<uint160> && t) { return convertInputNumpy(std::move(t)); }
pybind11::array_t<std::array<char, 64>> PythonConversionTypeConverter::operator()(ranges::any_view<uint256> && t) { return convertInputNumpy(std::move(t)); }

pybind11::array_t<int64_t> PythonConversionTypeConverter::operator()(ranges::any_view<int64_t, random_access_sized> && t) { return convertRandomSizedNumpy(std::move(t)); }
pybind11::array_t<uint64_t> PythonConversionTypeConverter::operator()(ranges::any_view<uint64_t, random_access_sized> && t) { return convertRandomSizedNumpy(std::move(t)); }
pybind11::array_t<int32_t> PythonConversionTypeConverter::operator()(ranges::any_view<int32_t, random_access_sized> && t) { return convertRandomSizedNumpy(std::move(t)); }
pybind11::array_t<uint32_t> PythonConversionTypeConverter::operator()(ranges::any_view<uint32_t, random_access_sized> && t) { return convertRandomSizedNumpy(std::move(t)); }
pybind11::array_t<int16_t> PythonConversionTypeConverter::operator()(ranges::any_view<int16_t, random_access_sized> && t) { return convertRandomSizedNumpy(std::move(t)); }
pybind11::array_t<uint16_t> PythonConversionTypeConverter::operator()(ranges::any_view<uint16_t, random_access_sized> && t) { return convertRandomSizedNumpy(std::move(t)); }
pybind11::array_t<NumpyBool> PythonConversionTypeConverter::operator()(ranges::any_view<bool, random_access_sized> && t) { return convertRandomSizedNumpy(std::move(t)); }
pybind11::array_t<NumpyDatetime> PythonConversionTypeConverter::operator()(ranges::any_view<std::chrono::system_clock::time_point, random_access_sized> && t) { return convertInputNumpy(std::move(t)); }
pybind11::array_t<std::array<char, 40>> PythonConversionTypeConverter::operator()(ranges::any_view<uint160, random_access_sized> && t) { return convertRandomSizedNumpy(std::move(t)); }
pybind11::array_t<std::array<char, 64>> PythonConversionTypeConverter::operator()(ranges::any_view<uint256, random_access_sized> && t) { return convertRandomSizedNumpy(std::move(t)); }



ranges::any_view<blocksci::Input> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Input, random_access_sized>, random_access_sized> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Input> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Input>, random_access_sized> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Input> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Input, random_access_sized>> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Input> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Input>> && t) { return {ranges::view::join(std::move(t))}; }

ranges::any_view<blocksci::Output> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Output, random_access_sized>, random_access_sized> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Output> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Output>, random_access_sized> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Output> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Output, random_access_sized>> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Output> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Output>> && t) { return {ranges::view::join(std::move(t))}; }

ranges::any_view<blocksci::Transaction> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Transaction, random_access_sized>, random_access_sized> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Transaction> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Transaction>, random_access_sized> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Transaction> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Transaction, random_access_sized>> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Transaction> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Transaction>> && t) { return {ranges::view::join(std::move(t))}; }

ranges::any_view<blocksci::Address> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Address, random_access_sized>, random_access_sized> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Address> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Address>, random_access_sized> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Address> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Address, random_access_sized>> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Address> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::Address>> && t) { return {ranges::view::join(std::move(t))}; }

ranges::any_view<blocksci::TaggedAddress> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::TaggedAddress, random_access_sized>, random_access_sized> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::TaggedAddress> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::TaggedAddress>, random_access_sized> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::TaggedAddress> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::TaggedAddress, random_access_sized>> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::TaggedAddress> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::any_view<blocksci::TaggedAddress>> && t) { return {ranges::view::join(std::move(t))}; }

ranges::any_view<blocksci::Output> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<blocksci::OutputRange, random_access_sized> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Output> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<blocksci::OutputRange> && t) { return {ranges::view::join(std::move(t))}; }

ranges::any_view<blocksci::Input> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<blocksci::InputRange, random_access_sized> && t) { return {ranges::view::join(std::move(t))}; }
ranges::any_view<blocksci::Input> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<blocksci::InputRange> && t) { return {ranges::view::join(std::move(t))}; }


ranges::any_view<blocksci::Input> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Input, random_access_sized>>, random_access_sized> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Input> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Input>>, random_access_sized> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Input> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Input, random_access_sized>>> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Input> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Input>>> && t) { return flattenNestedOptional(std::move(t)); }

ranges::any_view<blocksci::Output> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Output, random_access_sized>>, random_access_sized> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Output> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Output>>, random_access_sized> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Output> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Output, random_access_sized>>> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Output> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Output>>> && t) { return flattenNestedOptional(std::move(t)); }

ranges::any_view<blocksci::Transaction> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Transaction, random_access_sized>>, random_access_sized> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Transaction> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Transaction>>, random_access_sized> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Transaction> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Transaction, random_access_sized>>> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Transaction> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Transaction>>> && t) { return flattenNestedOptional(std::move(t)); }

ranges::any_view<blocksci::Address> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Address, random_access_sized>>, random_access_sized> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Address> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Address>>, random_access_sized> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Address> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Address, random_access_sized>>> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Address> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::Address>>> && t) { return flattenNestedOptional(std::move(t)); }

ranges::any_view<blocksci::TaggedAddress> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::TaggedAddress, random_access_sized>>, random_access_sized> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::TaggedAddress> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::TaggedAddress>>, random_access_sized> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::TaggedAddress> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::TaggedAddress, random_access_sized>>> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::TaggedAddress> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<ranges::any_view<blocksci::TaggedAddress>>> && t) { return flattenNestedOptional(std::move(t)); }


ranges::any_view<blocksci::Output> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<blocksci::OutputRange>, random_access_sized> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Output> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<blocksci::OutputRange>> && t) { return flattenNestedOptional(std::move(t)); }

ranges::any_view<blocksci::Input> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<blocksci::InputRange>, random_access_sized> && t) { return flattenNestedOptional(std::move(t)); }
ranges::any_view<blocksci::Input> ConvertedRangeTypeTagConverter::operator()(ranges::any_view<ranges::optional<blocksci::InputRange>> && t) { return flattenNestedOptional(std::move(t)); }


template <typename T>
using random_range = ranges::any_view<T, random_access_sized>;

// #define createNumpyConvertedRange(functype) \
//     template pybind11::array_t<decltype(NumpyConverter{}(std::declval<ranges::range_value_type_t<functype>>()))> \
//     PythonConversionTypeConverter::template operator()<functype>(numpy_tag, functype && t);

// #define createNumpyRangeFuncs(functype) \
//     createNumpyConvertedRange(ranges::any_view<functype>) \
//     createNumpyConvertedRange(random_range<functype>) \

// createNumpyRangeFuncs(uint160)
// createNumpyRangeFuncs(uint256)
// createNumpyRangeFuncs(uint16_t)
// createNumpyRangeFuncs(int16_t)
// createNumpyRangeFuncs(uint32_t)
// createNumpyRangeFuncs(int32_t)
// createNumpyRangeFuncs(uint64_t)
// createNumpyRangeFuncs(int64_t)
// createRangeFuncs(bool)

// #define createRangeFuncs(functype) \
//     createRangeFuncsImpl(functype) \
//     createRangeFuncsImpl(ranges::optional<functype>) \

// createRangeFuncs(AddressType::Enum)
// createRangeFuncs(Input)
// createRangeFuncs(Output)
// createRangeFuncs(Transaction)
// createRangeFuncs(Block)
// createRangeFuncs(Address)
// createRangeFuncs(AnyScript)
// createRangeFuncs(EquivAddress)
// createRangeFuncs(script::Pubkey)
// createRangeFuncs(script::PubkeyHash)
// createRangeFuncs(script::WitnessPubkeyHash)
// createRangeFuncs(script::MultisigPubkey)
// createRangeFuncs(script::Multisig)
// createRangeFuncs(script::OpReturn)
// createRangeFuncs(script::Nonstandard)
// createRangeFuncs(script::ScriptHash)
// createRangeFuncs(script::WitnessScriptHash)
// createRangeFuncs(Cluster)
// createRangeFuncs(TaggedCluster)
// createRangeFuncs(TaggedAddress)
// createRangeFuncs(std::string)
// createRangeFuncs(std::chrono::system_clock::time_point)
// createRangeFuncs(py::list)
// createRangeFuncs(py::bytes)
// using intAddressPair = std::pair<int64_t, Address>;
// using addressIntPair = std::pair<Address, int64_t>;
// createRangeFuncs(intAddressPair)
// createRangeFuncs(addressIntPair)