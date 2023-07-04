# ConicBundle

This repository was initialized as a GitHub clone Christoph Helmberg's
[ConicBundle](https://www-user.tu-chemnitz.de/~helmberg/ConicBundle/Manual/index.html), version 1.a.2.
The source code itself is almost unchanged, but an additional set of export functions have been provided for a lot of the C++
interface.
Note that the C++ part was generated automatically; a manual cleanup would certainly be desirable.
A project file for Visual Studio 2022 was added and the binary for Windows x64 are provided. While it was not tried to compile
the project on Linux, this should be very doable, as this was ConicBundle's original platform.

# Compilation
For the compilation of ConicBundle under Linux, just run `make` in this directory, which will create both a static library
`libcb.a` and a shared object file `ConicBundle.so` in `lib`.
For more options, see the [the ConicBundle readme](README).
For Windows, the repository comes with a Visual Studio 2022 project file which you can just open and compile. The output DLL
(in x64 Release mode) should go to the `lib` directory automatically.

# C++ interface
As said before, the C++ interface was generated automatically, to be used by the (also automatically-generated)
[Julia bindings for ConicBundle](https://github.com/projekter/ConicBundle.jl/). You can find the Python script for the
generation the [`cppinterface`](cppinterface/adapter.py) folder: just run the Python script from this subfolder. Make sure that
the `ConicBundle.jl` package exists on the same level as this package, so that its headers are also replaced accordingly.
Note that Python package `regex` is required, which can be easily installed using pip.

# License
ConicBundle is licensed under GPL v3.0 or later.