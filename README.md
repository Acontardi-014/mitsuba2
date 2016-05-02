# Mitsuba Renderer

## Compiling

Compiling from scratch requires CMake and a recent version of XCode on Mac,
Visual Studio 2015 on Windows, and GCC on Linux.

On Linux and MacOS, compiling should be as simple as

```bash
git clone --recursive https://github.com/mitsuba-renderer/mitsuba2
cd mitsuba2

# Build using CMake & GNU Make (legacy)
cmake .
make -j 4

# OR: Build using CMake & Ninja (recommended, must install Ninja first)
cmake -GNinja .
ninja
```

On Windows, open the generated ``mitsuba.sln`` file after running
``cmake .`` and proceed building as usual from within Visual Studio.

Mitsuba organizes its software dependencies in a hierarchy of
sub-repositories using *git submodule*. Unfortunately, as of May 2016,
pulling from the main repository won't automatically keep the
sub-repositories in sync, which can lead to various problems. The
following command installs a git alias named ``pullall`` that automates
these two steps.

```bash
git config —global alias.pullall '!f(){ git pull "$@" && git submodule update --init --recursive; }; f'
```

Afterwards, simply write
```bash
git pullall
```
to stay in sync.
