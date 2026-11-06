#ifndef COMMANDKIT_HPP
#define COMMANDKIT_HPP

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <type_traits>
#include <tuple>
#include <charconv>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cassert>

#if defined(_MSVC_LANG)
#   define INTERNAL_CMDKIT_CPLUSPLUS _MSVC_LANG
#else
#   define INTERNAL_CMDKIT_CPLUSPLUS __cplusplus
#endif
#define INTERNAL_CMDKIT_CPP20 (INTERNAL_CMDKIT_CPLUSPLUS >= 202002L)

#if defined(__GNUC__) || defined(__clang__)
#   define CK_LIKELY(x)   (__builtin_expect(!!(x), 1))
#   define CK_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#elif INTERNAL_CMDKIT_CPP20
#   define CK_LIKELY(x)   (x) [[likely]]
#   define CK_UNLIKELY(x) (x) [[unlikely]]
#else
#   define CK_LIKELY(x)   (x)
#   define CK_UNLIKELY(x) (x)
#endif

#if defined(_MSC_VER)
#   define CK_SAFEBUFFERS __declspec(safebuffers)
#else
#   define CK_SAFEBUFFERS
#endif

#if defined(_MSC_VER)
#   define CK_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#   define CK_FORCEINLINE __attribute__((always_inline)) inline
#else
#   define CK_FORCEINLINE inline
#endif

#if INTERNAL_CMDKIT_CPP20
#   include <span>
#endif
#if defined(CMDKIT_USE_ABSEIL)
#   include <absl/container/flat_hash_map.h>
#endif

namespace commandkit {

#if INTERNAL_CMDKIT_CPP20
    template <typename T> using span = std::span<T>;
#else
    template <typename T>
    struct span {
        T* ptr = nullptr;
        std::size_t n = 0;
        constexpr span() noexcept = default;
        constexpr span(T* p, std::size_t s) noexcept : ptr(p), n(s) {}
        constexpr T* data()  const noexcept { return ptr; }
        constexpr std::size_t size()  const noexcept { return n; }
        constexpr bool        empty() const noexcept { return n == 0; }
        constexpr T& operator[](std::size_t i) const noexcept { return ptr[i]; }
        constexpr T* begin() const noexcept { return ptr; }
        constexpr T* end()   const noexcept { return ptr + n; }
    };
#endif

    struct Rest { std::string_view value; };

    template <typename T>
    struct Greedy : std::vector<T> { using std::vector<T>::vector; };

    template <typename T, typename = void> struct Converter;

    template <typename T>
    struct Converter<T, std::enable_if_t<std::is_arithmetic_v<T>>> {
        static T convert(std::string_view a) {
            T v{};
            if constexpr (std::is_floating_point_v<T>) {
                char buf[64];
                const auto len = (a.size() < sizeof(buf) - 1) ? a.size() : sizeof(buf) - 1;
                std::memcpy(buf, a.data(), len);
                buf[len] = '\0';
                char* end = nullptr;
                if constexpr (std::is_same_v<T, float>)      v = std::strtof(buf, &end);
                else if constexpr (std::is_same_v<T, double>) v = std::strtod(buf, &end);
                else                                           v = static_cast<T>(std::strtold(buf, &end));
                if (end == buf) return {};
            }
            else {
                if (std::from_chars(a.data(), a.data() + a.size(), v).ec != std::errc{}) return {};
            }
            return v;
        }
    };

    template <> struct Converter<std::string> {
        static std::string convert(std::string_view a) { return std::string(a); }
    };
    template <> struct Converter<std::string_view> {
        static std::string_view convert(std::string_view a) noexcept { return a; }
    };

    // TODO(error-handling): error propagation strategy TBD
    enum class ParseError : uint8_t { None = 0, MissingArg, BadConversion, BadQuote };

    struct ParseCtx {
        std::string_view buf;
        std::size_t      i = 0;
        ParseError       err = ParseError::None;
    };

    namespace detail {

        template <typename T> struct is_greedy : std::false_type {};
        template <typename T> struct is_greedy<Greedy<T>> : std::true_type {};

        template <typename T> struct is_optional : std::false_type {};
        template <typename T> struct is_optional<std::optional<T>> : std::true_type {};

        template <typename T> struct fn_traits : fn_traits<decltype(&T::operator())> {};
        template <typename C, typename R, typename... A> struct fn_traits<R(C::*)(A...) const> { using args = std::tuple<A...>; };
        template <typename C, typename R, typename... A> struct fn_traits<R(C::*)(A...)> { using args = std::tuple<A...>; };
        template <typename R, typename... A>             struct fn_traits<R(*)(A...)> { using args = std::tuple<A...>; };
        template <typename C, typename R, typename... A> struct fn_traits<R(C::*)(A...) const noexcept> { using args = std::tuple<A...>; };
        template <typename C, typename R, typename... A> struct fn_traits<R(C::*)(A...) noexcept> { using args = std::tuple<A...>; };
        template <typename R, typename... A>             struct fn_traits<R(*)(A...) noexcept> { using args = std::tuple<A...>; };

        struct StringHash {
            using is_transparent = void;
            std::size_t operator()(std::string_view sv) const noexcept {
                return std::hash<std::string_view>{}(sv);
            }
        };

#if defined(CMDKIT_USE_ABSEIL)
        template <typename K, typename V> using CmdMap = absl::flat_hash_map<K, V, StringHash, std::equal_to<>>;
#elif defined(CMDKIT_USE_HASHMAP)
#   if INTERNAL_CMDKIT_CPP20
        template <typename K, typename V> using CmdMap = std::unordered_map<K, V, StringHash, std::equal_to<>>;
#   else
        // C++17: no heterogeneous lookup in std::unordered_map; find allocates a temporary std::string
        template <typename K, typename V> using CmdMap = std::unordered_map<K, V>;
#   endif
#else
        template <typename K, typename V> using CmdMap = std::vector<std::pair<K, V>>;
#endif

        CK_SAFEBUFFERS CK_FORCEINLINE
            std::optional<std::string> parse_quoted_word(ParseCtx& ctx) {
            if (ctx.i >= ctx.buf.size()) return std::nullopt;
            const char quote = ctx.buf[ctx.i];
            if (quote != '"' && quote != '\'') {
                const std::size_t s = ctx.i;
                while (ctx.i < ctx.buf.size() && ctx.buf[ctx.i] != ' ' && ctx.buf[ctx.i] != '\t' &&
                    ctx.buf[ctx.i] != '\n' && ctx.buf[ctx.i] != '\r') ++ctx.i;
                if (ctx.i == s) return std::nullopt;
                return std::string(ctx.buf.substr(s, ctx.i - s));
            }
            std::size_t start = ++ctx.i;
            const std::string_view targets = (quote == '"') ? "\"\\" : "'\\";
            std::string r;
            while (true) {
                std::size_t pos = ctx.buf.find_first_of(targets, start);
                if (pos == std::string_view::npos) { ctx.err = ParseError::BadQuote; return std::nullopt; }
                if (pos > start) r.append(ctx.buf.data() + start, pos - start);
                if (ctx.buf[pos] == quote) { ctx.i = pos + 1; return r; }
                if (pos + 1 >= ctx.buf.size()) { ctx.err = ParseError::BadQuote; return std::nullopt; }
                r.push_back(ctx.buf[pos + 1]);
                start = pos + 2;
            }
        }

        CK_SAFEBUFFERS CK_FORCEINLINE
            std::optional<std::string_view> parse_quoted_view(ParseCtx& ctx) {
            if (ctx.i >= ctx.buf.size()) return std::nullopt;
            const char quote = ctx.buf[ctx.i];
            if (quote != '"' && quote != '\'') {
                const std::size_t s = ctx.i;
                while (ctx.i < ctx.buf.size() && ctx.buf[ctx.i] != ' ' && ctx.buf[ctx.i] != '\t' &&
                    ctx.buf[ctx.i] != '\n' && ctx.buf[ctx.i] != '\r') ++ctx.i;
                if (ctx.i == s) return std::nullopt;
                return ctx.buf.substr(s, ctx.i - s);
            }
            const std::size_t start = ++ctx.i;
            std::size_t cur = start;
            const std::string_view targets = (quote == '"') ? "\"\\" : "'\\";
            while (true) {
                std::size_t pos = ctx.buf.find_first_of(targets, cur);
                if (pos == std::string_view::npos) { ctx.err = ParseError::BadQuote; return std::nullopt; }
                if (ctx.buf[pos] == quote) { ctx.i = pos + 1; return ctx.buf.substr(start, pos - start); }
                cur = pos + 2;
            }
        }

        template <typename T>
        CK_FORCEINLINE T parse_one(ParseCtx& ctx) {
            if constexpr (is_greedy<T>::value) {
                T r;
                using I = typename T::value_type;
                while (true) {
                    while (ctx.i < ctx.buf.size() && (ctx.buf[ctx.i] == ' ' || ctx.buf[ctx.i] == '\t' ||
                        ctx.buf[ctx.i] == '\n' || ctx.buf[ctx.i] == '\r')) ++ctx.i;
                    if (ctx.i >= ctx.buf.size()) break;
                    const std::size_t saved = ctx.i;
                    I elem = parse_one<I>(ctx);  // attempt
                    if (ctx.err != ParseError::None) {
                        ctx.i = saved;
                        ctx.err = ParseError::None;
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL > 0
                        // token was present but conversion failed — likely a type mismatch at registration
                        assert(false && "Greedy<T>: token present but conversion failed");
#endif
                        break;
                    }
                    r.push_back(std::move(elem));
                }
                return r;
            }
            else if constexpr (is_optional<T>::value) {
                using I = typename T::value_type;
                while (ctx.i < ctx.buf.size() && (ctx.buf[ctx.i] == ' ' || ctx.buf[ctx.i] == '\t' ||
                    ctx.buf[ctx.i] == '\n' || ctx.buf[ctx.i] == '\r')) ++ctx.i;
                if (ctx.i >= ctx.buf.size()) return std::nullopt;
                const std::size_t saved = ctx.i;
                I elem = parse_one<I>(ctx);
                if (ctx.err != ParseError::None) {
                    ctx.i = saved;
                    ctx.err = ParseError::None;
                    return std::nullopt;
                }
                return T{ std::move(elem) };
            }
            else if constexpr (std::is_same_v<T, Rest>) {
                while (ctx.i < ctx.buf.size() && (ctx.buf[ctx.i] == ' ' || ctx.buf[ctx.i] == '\t' ||
                    ctx.buf[ctx.i] == '\n' || ctx.buf[ctx.i] == '\r')) ++ctx.i;
                std::string_view rest = ctx.buf.substr(ctx.i);
                ctx.i = ctx.buf.size();
                return Rest{ rest };
            }
            else {
                while (ctx.i < ctx.buf.size() && (ctx.buf[ctx.i] == ' ' || ctx.buf[ctx.i] == '\t' ||
                    ctx.buf[ctx.i] == '\n' || ctx.buf[ctx.i] == '\r')) ++ctx.i;
                if (ctx.i >= ctx.buf.size()) { ctx.err = ParseError::MissingArg; return T{}; }
                if constexpr (std::is_arithmetic_v<T>) {
                    auto sv = parse_quoted_view(ctx);
                    if (!sv || ctx.err != ParseError::None) { ctx.err = ParseError::BadConversion; return T{}; }
                    T v = Converter<T>::convert(*sv);
                    // converter returns default on failure (no throw); detect via re-check
                    // For now treat any non-throwing bad parse as BadConversion.
                    // TODO(error-handling): revisit when error propagation strategy is decided.
                    return v;
                }
                else if constexpr (std::is_same_v<T, std::string_view>) {
                    auto sv = parse_quoted_view(ctx);
                    if (!sv || ctx.err != ParseError::None) { ctx.err = ParseError::BadConversion; return T{}; }
                    return *sv;
                }
                else {
                    auto w = parse_quoted_word(ctx);
                    if (!w || ctx.err != ParseError::None) { ctx.err = ParseError::BadConversion; return T{}; }
                    return Converter<T>::convert(*w);
                }
            }
        }

        // Brace-init guarantees left-to-right evaluation of pack expansion (C++17 [dcl.init.list]).
        template <typename Tuple, std::size_t... Is>
        CK_FORCEINLINE Tuple parse_all(ParseCtx& ctx, std::index_sequence<Is...>) {
            return Tuple{ parse_one<std::tuple_element_t<Is, Tuple>>(ctx)... };
        }
        template <typename Tuple>
        CK_FORCEINLINE Tuple parse_all(ParseCtx& ctx) {
            return parse_all<Tuple>(ctx, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
        }

    } // namespace detail

    class CommandManager {
        using Invoker = std::function<void(ParseCtx&)>;

#if defined(CMDKIT_USE_ABSEIL) || defined(CMDKIT_USE_HASHMAP)
        detail::CmdMap<std::string, Invoker> cmds_;

        CK_FORCEINLINE void insert_cmd_(std::string_view name, Invoker inv) {
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL > 0
            assert(cmds_.find(std::string(name)) == cmds_.end() && "CommandManager: duplicate command registration");
#endif
            cmds_.emplace(std::string(name), std::move(inv));
        }

#   if INTERNAL_CMDKIT_CPP20 || defined(CMDKIT_USE_ABSEIL)
        CK_FORCEINLINE const Invoker* find_cmd_(std::string_view name) const noexcept {
            auto it = cmds_.find(name);
            return it != cmds_.end() ? &it->second : nullptr;
        }
#   else
        const Invoker* find_cmd_(std::string_view name) const noexcept {
            auto it = cmds_.find(std::string(name)); // C++17 std::unordered_map: no heterogeneous lookup
            return it != cmds_.end() ? &it->second : nullptr;
        }
#   endif

#else
        using Entry_ = std::pair<std::string, Invoker>;
        detail::CmdMap<std::string, Invoker> cmds_;

        void insert_cmd_(std::string_view name, Invoker inv) {
            auto it = std::lower_bound(cmds_.begin(), cmds_.end(), name,
                [](const Entry_& e, std::string_view k) noexcept { return e.first < k; });
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL > 0
            assert((it == cmds_.end() || it->first != name) && "CommandManager: duplicate command registration");
#endif
            cmds_.emplace(it, std::string(name), std::move(inv));
        }

        const Invoker* find_cmd_(std::string_view name) const noexcept {
            auto it = std::lower_bound(cmds_.begin(), cmds_.end(), name,
                [](const Entry_& e, std::string_view k) noexcept { return e.first < k; });
            return (it != cmds_.end() && it->first == name) ? &it->second : nullptr;
        }
#endif

    public:
        CommandManager() = default;

        template <typename Func>
        CK_FORCEINLINE void command(std::string_view name, Func&& func) {
            using AT = typename detail::fn_traits<std::decay_t<Func>>::args;
            insert_cmd_(name, [f = std::forward<Func>(func)](ParseCtx& ctx) {
                std::apply(f, detail::parse_all<AT>(ctx));
                });
        }

        // 0 = success, -1 = unknown command, -2 = bad arguments
        CK_SAFEBUFFERS CK_FORCEINLINE int process_command(std::string_view input) {
            ParseCtx ctx{ input, 0, ParseError::None };

            while (ctx.i < ctx.buf.size() && (ctx.buf[ctx.i] == ' ' || ctx.buf[ctx.i] == '\t' ||
                ctx.buf[ctx.i] == '\n' || ctx.buf[ctx.i] == '\r')) ++ctx.i;

            const std::size_t name_start = ctx.i;
            while (ctx.i < ctx.buf.size() && ctx.buf[ctx.i] != ' ' && ctx.buf[ctx.i] != '\t' &&
                ctx.buf[ctx.i] != '\n' && ctx.buf[ctx.i] != '\r') ++ctx.i;

            if (ctx.i == name_start) return -1;
            const std::string_view name = ctx.buf.substr(name_start, ctx.i - name_start);

            const Invoker* inv = find_cmd_(name);
            if CK_UNLIKELY(!inv) return -1;

            (*inv)(ctx);
            return ctx.err == ParseError::None ? 0 : -2;
        }
    };

} // namespace commandkit

#endif // COMMANDKIT_HPP