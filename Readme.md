# local-bind-cpp-library v0.01 (6th Oct 2014)

(C) 2014 Niall Douglas http://www.nedproductions.biz/

This is a tool which can generate a set of local bindings of some C++ library into some namespace. It does this
by parsing the source C++ library using clang's AST library and extracting a regular expression matched API into
a sequence of template alias, using, and typedefs in the local namespace.

Usage looks like this:

    ./genmap include/thread STL11_MAP_ "std::([^_].*)" thread

This would grok through everything declared into namespace std by doing
`#include <thread>` not prefixed with an underscore and generate a set of bindings into include/thread.

In case that outputs too much to be portable, you can reduce to a least common subset too:

    ./genmap include/mutex STL11_MAP_ "std::([^_].*)" mutex "boost::([^_].*)" boost/thread.hpp

This takes all items common to both `#include <mutex>` and `#include <boost/thread.hpp>` and generates
bindings just for those. If one regular expression isn't enough, you can separate multiple regexs with commas.

To then include those local namespace bindings into let's say namespace `boost::spinlock` one simply does this:

    #define STL11_MAP_BEGIN_NAMESPACE namespace boost { namespace spinlock { inline namespace stl11 {
    #define STL11_MAP_END_NAMESPACE } } }
    #include "local-bind-cpp-library/include/stl11/atomic"
    #include "local-bind-cpp-library/include/stl11/chrono"
    #include "local-bind-cpp-library/include/stl11/mutex"
    #include "local-bind-cpp-library/include/stl11/thread"
    #undef STL11_MAP_BEGIN_NAMESPACE
    #undef STL11_MAP_END_NAMESPACE
    
You'll probably note the use of an inline namespace for the innermost namespace - this is best practice, and
the bindings will actually appear in `boost::spinlock`.


##  Why on earth would you possibly want such a thing?

Ah, because now you can write code without caring where your dependent library comes from. Let's say you're
writing code which could use the C++ 11 STL or Boost as a substitute - the former is declared into namespace
std, the latter into namespace boost. Ordinarily you have to `#ifdef` your way along, either at every point
of use, or write some brittle bindings which manually replicate STL items into an internal namespace as a thin
shim to the real implementation.

With the bindings produced by this tool, and thanks to template aliasing in C++ 11 onwards, all that shim
writing goes away! You can now write code **once**, using an internal namespace containing your preferred combination
of dependent library implementation, and you no longer need to care about who is the real implementation.

This lets you custom configure, per library, some arbitrary mix of dependent libraries. For example, if you want
all C++ 11 STL *except* for future<T> promise<T> which you want from Boost.Thread, this binding tool makes
achieving that far easier. You generate your bindings and then hand edit them to reflect your particular
custom configuration. Much easier!

It also lets one break off Boost libraries away from Boost and become capable of being used standalone or
with a very minimal set of dependencies instead of dragging in the entire of Boost for even a small library.
This makes actual modular Boost usage possible (at last!).


## What state is this tool in?

Very, very early. It's basically hacked together right now, and doesn't yet support typedef binding, variable
binding or function binding. It does mostly support template type binding and enum binding (both scoped and
traditional) well enough to provide usable bindings for most of the C++ 11 STL:

* array
* atomic
* chrono
* condition_variable
* filesystem (defaulted to Boost's implementation)
* future
* mutex
* random
* ratio
* system_error
* thread
* tuple (the lack of make_tuple is a pain though)
* type_traits
* typeindex

Your compiler *must* support template aliasing for these bindings to work, and you can find a prebuilt set in
include/stl11. Note that these bindings always bind to the C++ 11 STL, but it's very easy to have them target
boost instead - simply swap the order of parameters in withgcc.sh.

Bindings really needing function binding support to be useful are:

* functional
* regex

There is no technical reason these aren't in there yet, just I don't need them so far.
