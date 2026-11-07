<!-- PROJECT LOGO -->
<br />
<div align="center">
  <img src="./docs/logo.svg" alt="commandkit logo" width="80" height="80">

<h3 align="center">commandkit</h3>

<p align="center">
  <strong>one sentence about what it does..</strong>
</p>

<p align="center">
  <a href="#usage">Usage</a> •
  <a href="#building">Building</a> •
  <a href="#project-structure">Project Structure</a> •
  <a href="#license">License</a>
</p>

</div>

---

## About

What this project is, and what it is not.

## Usage

```cpp
#include <commandkit/commandkit.hpp>

auto text = commandkit::greeting();
```

## Building


Requires CMake 3.21+ and a C++17 compiler. on Windows, open the folder in Visual Studio 2022 and pick a configuration from the dropdown, or from a terminal (after loading the matching `vcvars64.bat`/`vcvars32.bat`):
on linux its the same process

```
cmake --preset x64-Debug
cmake --build --preset x64-Debug
ctest --preset x64-Debug
```

Presets: `x64`/`x86` combined with `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`, `ReleaseNoOptWithDeb` (no optimization, debug info, `NDEBUG` on) and `DebugASan` (AddressSanitizer). Output goes to `build/<preset>/`.

## License

The source code is licensed under the [AGPL v3 License](./LICENSE).

---

<div align="center">
  <p>Made with ❤️</p>
</div>
