# dottorrent

A C++20 library for working with BitTorrent metafiles.

This library is used in the [torrenttools](https://github.com/fbdtemme/torrenttools) project.
Use outside of the torrenttools project is currently not recommended due to a lack of documentation.

## Building

This project requires C++20.
Currently only GCC 10 or later is supported.

This project can be build with CMake.

```{bash}
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make 
```

The library can be installed as a CMake package.

```
make install
```