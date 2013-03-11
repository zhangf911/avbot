//
// yield.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "coro.hpp"

#ifndef reenter
# define reenter(c) CORO_REENTER(c)
#endif

#ifndef coyield
# define coyield CORO_YIELD
#endif

#ifndef cofork
# define cofork CORO_FORK
#endif
