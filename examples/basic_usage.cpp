#define CMDKIT_USE_HASHMAP
#include <commandkit/commandkit.hpp>

#include <iostream>

int main() {
    commandkit::CommandManager cm;

    cm.command("add", [](int a, int b) {
        std::cout << "add: " << a + b << "\n";
    });

    cm.command("greet", [](std::string name) {
        std::cout << "greet: Hello, " << name << "!\n";
    });

    cm.command("announce", [](std::string title, commandkit::Rest msg) {
        std::cout << "announce: [" << title << "] " << msg.value << "\n";
    });

    cm.command("sum", [](commandkit::Greedy<int> nums) {
        int total = 0;
        for (int n : nums) total += n;
        std::cout << "sum: " << total << "\n";
    });

    cm.command("opt", [](std::optional<int> x) {
        std::cout << "opt: " << (x ? std::to_string(*x) : "none") << "\n";
    });

    auto run = [&](std::string_view s) {
        int r = cm.process_command(s);
        if (r != 0) std::cout << "ERROR " << r << " on: " << s << "\n";
    };

    run("add 10 20");
    run("greet Casey");
    run("greet \"John Doe\"");
    run("announce \"System Alert\" This is a test message.");
    run("sum 1 2 3 4 5");
    run("opt 42");
    run("opt");
    run("unknown");
    run("add 10 oops");

    return 0;
}