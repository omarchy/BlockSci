//
//  block_py.cpp
//  blocksci
//
//  Created by Harry Kalodner on 7/4/17.
//
//

#include "block_py.hpp"
#include "caster_py.hpp"
#include "self_apply_py.hpp"
#include "ranges_py.hpp"

#include <blocksci/chain/access.hpp>

#include <pybind11/operators.h>

namespace py = pybind11;
using namespace blocksci;

void init_uint256(py::class_<uint256> &cl) {
    cl
    .def("__repr__", &uint256::GetHex)
    .def(py::pickle(
        [](const uint256 &hash) {
            std::vector<char> data;
            for (auto c : hash) {
                data.push_back(c);
            }
            return py::make_tuple(data);
        },
        [](py::tuple t) {
            if (t.size() != 1) {
                throw std::runtime_error("Invalid state!");
            }
            auto data = t[0].cast<std::vector<char>>();
            return uint256{data.begin(), data.end()};
        }
    ))
    ;
}

void init_uint160(py::class_<uint160> &cl) {
    cl
    .def("__repr__", &uint160::GetHex)
    .def(py::pickle(
        [](const uint160 &hash) {
            std::vector<char> data;
            for (auto c : hash) {
                data.push_back(c);
            }
            return py::make_tuple(data);
        },
        [](py::tuple t) {
            if (t.size() != 1) {
                throw std::runtime_error("Invalid state!");
            }
            auto data = t[0].cast<std::vector<char>>();
            return uint160{data.begin(), data.end()};
        }
    ))
    ;
}

void init_block(py::class_<Block> &cl) {
    addRangeMethods(cl);
    cl
    .def("__repr__", &Block::toString)
    .def(py::self == py::self)
    .def(hash(py::self))
    .def_property_readonly("_access", [](const Block &block) {
        return Access{&block.getAccess()};
    })
    .def("net_address_type_value", py::overload_cast<const Block &>(netAddressTypeValue), "Returns a set of the net change in the utxo pool after this block split up by address type")
    .def("net_full_type_value", py::overload_cast<const Block &>(netFullTypeValue), "Returns a set of the net change in the utxo pool after this block split up by full type")
    ;

    applyMethodsToSelf(cl, AddBlockMethods{});
}
