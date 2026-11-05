#include <commandkit/commandkit.hpp>

#include <iostream>

static int failures = 0;

// Unlike assert(), this still runs when NDEBUG is defined (Release builds).
#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << __FILE__ << ':' << __LINE__                       \
                      << ": CHECK failed: " #cond "\n";                    \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

int main() {
    CHECK(!commandkit::greeting().empty());

    if (failures == 0) std::cout << "all checks passed\n";
    return failures == 0 ? 0 : 1;
}
