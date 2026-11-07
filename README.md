<!-- PROJECT LOGO -->
<br />
<div align="center">
  <img src="./docs/logo.svg" alt="commandkit logo" width="80" height="80">

<h3 align="center">commandkit</h3>

<p align="center">
  <strong>fast and easy to use library to parse string to commands</strong>
</p>

<p align="center">
  <a href="#usage">Usage</a> •
  <a href="#building">Building</a> •
  <a href="#todo">TODO</a> •
  <a href="#license">License</a>
</p>

</div>

---

## About
easy to use single header fast command manager library, 0 string allocations as much as possible while being extremely fast!
(my rewrite port of [python commandkit](https://github.com/programminglaboratorys/commandkit), made faster, better imo)

there is few options to its internal container to give you more control over its speed

## TODO
- rewrite features section
- make tests
- make more complex examples
- write small MVP doc

## Usage

`examples/basic_usage.cpp`
```cpp
//#define CMDKIT_USE_HASHMAP // use a hash map as the container for commands instead of sorted vector
#include <commandkit/commandkit.hpp>

#include <iostream>

int main() {
    commandkit::CommandManager cm;

    cm.command("add", [](int a, int b) { // example of 2 intagers
        std::cout << "add: " << a + b << "\n";
    });

    cm.command("greet", [](std::string name) { // example of a string
        std::cout << "greet: Hello, " << name << "!\n";
    });

    cm.command("announce", [](std::string title, commandkit::Rest msg) { // rest take the left over content
        std::cout << "announce: [" << title << "] " << msg.value << "\n";
    });

    cm.command("sum", [](commandkit::Greedy<int> nums) { // consume as much numbers possible
        int total = 0;
        for (int n : nums) total += n;
        std::cout << "sum: " << total << "\n";
    });

    cm.command("opt", [](std::optional<int> x) { // optional arguments are supported!
        std::cout << "opt: " << (x ? std::to_string(*x) : "none") << "\n";
    });

    auto run = [&](std::string_view s) {
        int r = cm.process_command(s);
        if (r != 0) std::cout << "ERROR " << r << " on: " << s << "\n";
    };

    run("add 10 20");
    run("greet Casey");
    run("greet \"John Doe\""); // respects qoutes, ' or " supported
    run("announce \"System Alert\" This is a test message."); // respects qoutes
    run("sum 1 2 3 4 5");
    run("opt 42");
    run("opt");
    run("unknown");
    run("add 10 oops");

    return 0;
}
```

### internal containers

there is 3 containers
- **std::vector** (*the default*): a sorted vector, uses binary search to find them ensuring 0 allocations and consistancy speed
- **unorderd_map**: hash map, enabled through CMDKIT_USE_HASHMAP macro, would use transparent lookup in C++20 otherwise it would sadly allocate strings to look them up
- **absl::flat_hash_map** (*recommended*): a flat hash map, enabled through CMDKIT_USE_ABSEIL macro, uses transparent lookup in C++20 AND C++17! its a fast SIMD container and keeps elements tight allowing for a better CPU cache locality, the best option


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
