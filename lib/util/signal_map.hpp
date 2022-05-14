//
// Copyright (c) 2022 Richard Hodges (hodges.r@gmail.com)
// Copyright (c) 2022 Alexander Hodges (alexander.hodges.personal@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/arby
//

#ifndef ARBY_LIB_UTIL_SIGNAL_MAP_HPP
#define ARBY_LIB_UTIL_SIGNAL_MAP_HPP

#include <boost/unordered_map.hpp>
#include <boost/functional/hash.hpp>
#include <boost/utility/string_view.hpp>
#include "config/json.hpp"

namespace arby::util {
    namespace detail {

        template<class String>
        struct sv_comp_equ
                : boost::hash<boost::string_view>, std::equal_to<> {
            using boost::hash<boost::string_view>::operator();
            using std::equal_to<>::operator();

            bool
            operator()(String const &s) const {
                return (*this)(boost::string_view(s.data(), s.size()));
            }
        };
    }
    template<class String, class Signal>
    using signal_map = boost::unordered_map<String, Signal, detail::sv_comp_equ<String>, detail::sv_comp_equ<String> >;
}

#endif   // ARBY_LIB_UTIL_SIGNAL_MAP_HPP
