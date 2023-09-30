# Overview

bgfxGWEN

a GWEN renderer over the bgfx framework

## History

- Last commit from upstream: 55a07b42d15e88741f354afdca908d0ff2697ec7
- project abandoned after 2013

## Building

### Original Build

The original build was using premake4.

The issue is premake4 is outdated, and the generator must have knowledge of output formats for various versions of build systems.

The stuff in bgfx (modern today's build) is premake5.

https://premake.github.io/docs/Using-Premake

That said, a bunch of flags are not valid.  This doesn't seem well maintained in bgfx either.

### CMake Build

The strategy to fix things is to switch this project to use cmake, and port forward all references to bgfx to use modern 10 years older BGFX.

```
cd build
cmake .. -DGWEN_HOME=PATH_TO_GWEN
make
```

## Running

TODO