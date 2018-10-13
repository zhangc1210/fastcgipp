# fastcgi++ #

**Author:** Eddie Carle

**Version:** 3.1alpha

## News ##

**October 12, 2018** - PostgreSQL functionality is now complete and I'm really
hoping to get people testing and giving feedback as soon as possible. I'm
pretty proud of this particular iteration at integrating database functionality
into fastcgi++ so go easy on me! Check out the [example][15], [unit test][16]
and [reference documentation][17] for further details.

**April 5, 2018** - It's been long enough, 3.0 is now official and marked
stable.

**May 28, 2016** - Some good examples are now done and in the documentation.
I've got said documentation hosted online now so [check it out][13].

**May 18, 2016** - The re-write is now effectively complete. What I mean by this
is that the library now builds and passes some basic benchmarking tests. I'm
really hoping to get as much feedback as possible so please test away. The docs
are not web hosted anywhere yet so you'll have to build them yourself as
described below. Note that the only example/tutorial that works so far is the
helloworld one.

**April 10, 2016** - Fastcgi++ is going through a dramatic rewrite now and the
master branch does not work at all. If you're here hoping for a functional
version scroll down to the releases section. If you'd like to read a bit more
about the rewrite and fastcgi++ in general, check out [[Ten years of
fastcgi++][1]].

[1]: https://eddie.isatec.ca/2016/04/10/ten-years-of-fastcgi++.html
[15]: https://isatec.ca/fastcgipp/sql.html
[16]: https://github.com/eddic/fastcgipp/blob/master/tests/sql.cpp
[17]: https://isatec.ca/fastcgipp/namespaceFastcgipp_1_1SQL.html

## About ##

This library is intended as a high-efficiency C++14 api for web development. It
allows your applications to communicate with web servers through the FastCGI
protocol, tabulates all your environment data, manages character encoding, and
allows requests to effectively share CPU time. If you want any further
information check the Doxygen documentation associated with the respective
release, or build it yourself. Be sure to read through the examples.

## Releases ##

Your best bet for releases and documentation is to clone the Git repository,
checkout the tag you want and see the building section of either this file or
the Doxygen documentation. If you're too lazy for that, however, you can take
the risk and try the following links.
 - [[fastcgipp-3.0.tar.gz][12]] [[Documentation][13]] [[Tree][14]]
 - [[fastcgi++-2.1.tar.bz2][2]] [[Documentation][3]] [[Tree][10]]
 - [[fastcgi++-2.0.tar.bz2][4]] [[Documentation][5]] [[Tree][11]]
 - [[fastcgi++-1.2.tar.bz2][6]] [[Documentation][7]]
 - [[fastcgi++-1.1.tar.bz2][8]]
 - [[fastcgi++-1.0.tar.bz2][9]]

[2]: http://download.savannah.nongnu.org/releases/fastcgipp/fastcgi++-2.1.tar.bz2
[3]: http://www.nongnu.org/fastcgipp/doc/2.1
[4]: http://download.savannah.nongnu.org/releases/fastcgipp/fastcgi++-2.0.tar.bz2
[5]: http://www.nongnu.org/fastcgipp/doc/2.0
[6]: http://download.savannah.nongnu.org/releases/fastcgipp/fastcgi++-1.2.tar.bz2
[7]: http://www.nongnu.org/fastcgipp/doc/1.2
[8]: http://download.savannah.nongnu.org/releases/fastcgipp/fastcgi++-1.1.tar.bz2
[9]: http://download.savannah.nongnu.org/releases/fastcgipp/fastcgi++-1.0.tar.bz2 
[10]: https://github.com/eddic/fastcgipp/tree/2.1
[11]: https://github.com/eddic/fastcgipp/tree/2.0
[12]: https://github.com/eddic/fastcgipp/archive/3.0.tar.gz
[13]: https://isatec.ca/fastcgipp
[14]: https://github.com/eddic/fastcgipp/tree/3.0

## Building ##

This should provide you with all the basic stuff you need to do to get fastcgi++
built and installed. The build system is CMake and the following instructions
assume you are in Bash.

First we need to clone.

    git clone https://github.com/eddic/fastcgipp.git fastcgi++

Then we make a build directory.

    mkdir fastcgi++.build
    cd fastcgi++.build

Now we need run cmake.

    cmake -DCMAKE_BUILD_TYPE=RELEASE ../fastcgi++

And if we want to build the PostgreSQL stuff we need to actually make it do so.

    cmake -DCMAKE_BUILD_TYPE=RELEASE -DSQL=true ../fastcgi++

Note that that was to do a release build. That means heavily optimized and not
good for debugging. If you want to do some debugging to either fastcgi++ or an
application you are developing that uses fastcgi++, do a debug build.

    cmake -DCMAKE_BUILD_TYPE=DEBUG ../fastcgi++

Or if you want some really hardcore debug and diagnostics info

    cmake -DCMAKE_BUILD_TYPE=DEBUG -D LOG_LEVEL:INT=4 ../fastcgi++

Now let's build the library itself.

    make

Then we can build the documentation if we so desire.

    make doc

Now let's install it all (doc included if it was built).

    make install

Maybe we should build the unit tests?

    make tests

And of course we should run them as well.

    make test

And hey, let's build the examples too!

    make examples
