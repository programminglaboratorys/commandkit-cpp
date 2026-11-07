//#define CMDKIT_USE_HASHMAP // use a hash map as the container for commands instead of sorted vector
#include <commandkit/commandkit.hpp>

#include <iostream>

int main() {
    commandkit::CommandManager cm;

    cm.command("add", [](int a, int b) { // example of 2 integers
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
    run("greet \"John Doe\""); // respects quotes, ' or " supported
    run("announce \"System Alert\" This is a test message."); // respects quotes
    run("sum 1 2 3 4 5");
    run("opt 42");
    run("opt");
    run("unknown");
    run("add 10 oops");

    return 0;
}