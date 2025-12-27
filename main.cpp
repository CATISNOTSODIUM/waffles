#include <string>
#include <iostream>
#include <type_traits>


namespace DFA {
template<bool accept, typename... EDGES>
struct Node;


template<char C, typename NEXT_NODE>
struct Edge {
    using next = NEXT_NODE;
    static inline constexpr bool check(char c) {
       return c == C;
    }
};

static_assert(std::is_same_v<Edge<'a', int>::next, int>);

template<typename T>
struct is_edge {
    static constexpr bool value = false;
};

template<char c, typename T>
struct is_edge<Edge<c, T>> {
    static constexpr bool value = true;
};

template<typename T>
static constexpr bool is_edge_v = is_edge<T>::value;

template<typename T>
struct is_node {
    static constexpr bool value = false;
};

template<bool accept, typename ...EDGES>
struct is_node<Node<accept, EDGES...>> {
    static constexpr bool value = true;
};

template<typename T>
static constexpr bool is_node_v = is_node<T>::value;


// Return the first possible edge
template<char c, typename NODE, typename... EDGES>
struct TransitionFinder;

// Forward declaration of runners
template<typename CURRENT>
struct RTDFARunner;

template<bool _accept, typename... EDGES>
struct Node {
    static constexpr bool accept = _accept;
    static_assert((is_edge_v<EDGES> && ...));
    static constexpr bool is_terminal = (sizeof...(EDGES) == 0);

    template<char c>
    static inline constexpr bool match_any = (EDGES::check(c) || ...);

    template<char c>
    using next = TransitionFinder<c, Node<_accept, EDGES...>, EDGES...>::result;

    // Runtime utilities
    static bool dispatch(const std::string& input, int index) {
        if (index >= input.length()) return accept;

        char c = input[index];
        bool result = false;

        // loops to EDGES (at compile time)
        auto _ = ((EDGES::check(c) ? (result = RTDFARunner<typename EDGES::next>::helper(input, index + 1), true) : false) || ...);

        return result;
    }
};


template<char c, typename NODE, typename FIRST, typename... REST>
struct TransitionFinder<c, NODE, FIRST, REST...> {
    using result = std::conditional_t<FIRST::check(c), typename FIRST::next, 
        typename TransitionFinder<c, NODE, REST...>::result>;
};

template<char c, typename NODE>
struct TransitionFinder<c, NODE> {
    using result = NODE;
};


template<size_t N>
struct FixedString {
    char data[N];
    constexpr FixedString(const char (&str)[N]) {
        for (int i = 0; i < N; ++i) data[i] = str[i];
    }
};

// Compile-time DFA
template<typename CURRENT, FixedString S, size_t INDEX = 0>
struct CTDFARunner {
    static constexpr bool run() {
        constexpr char c = S.data[INDEX];
        if constexpr (c == '\0') {
            return CURRENT::accept;
        } else if constexpr (CURRENT::template match_any<c>) {
            using NEXT_NODE = typename CURRENT::template next<c>;
            return CTDFARunner<NEXT_NODE, S, INDEX + 1>::run();
        } else {
            return false;
        }
    }
};

// Run-time DFA
template<typename CURRENT>
struct RTDFARunner {
    static bool helper(const std::string& input, int index) {
        return CURRENT::dispatch(input, index);
    }

    static bool run(const std::string& input) {
        return helper(input, 0);
    }
};

}

// Example
namespace TEST_MATCH_ANY {
    using namespace DFA;
    using Q1 = Node<true>;
    using Q0 = Node<false, Edge<'0', Q1>>;
    static_assert(Q0::match_any<'0'>);
    static_assert(!Q0::match_any<'1'>);
};

namespace TEST_TRANSITION {
    using namespace DFA;
    struct Q0; struct Q1; struct Q2; struct Q3;
    // Detect 01010101 string
    struct Q0 : Node<true, Edge<'0', Q1>>{};
    struct Q1 : Node<false, Edge<'1', Q2>>{};
    struct Q2 : Node<false, Edge<'0', Q3>>{};
    struct Q3 : Node<true, Edge<'1', Q2>>{};

    static_assert(std::is_same_v<Q0::next<'0'>, Q1>);
    static_assert(std::is_same_v<Q1::next<'1'>, Q2>);
    static_assert(std::is_same_v<Q2::next<'0'>, Q3>);
    static_assert(std::is_same_v<Q3::next<'1'>, Q2>);
};

// {01}*
namespace DFA_01_STAR {
    using namespace DFA;
    struct Q0; struct Q1; struct Q2; struct Q3;
    struct Q0 : Node<true, Edge<'0', Q1>>{};
    struct Q1 : Node<false, Edge<'1', Q2>>{};
    struct Q2 : Node<true, Edge<'0', Q3>>{};
    struct Q3 : Node<false, Edge<'1', Q2>>{};

    // Positive test cases
    static_assert(CTDFARunner<Q0, "">::run());
    static_assert(CTDFARunner<Q0, "01">::run());
    static_assert(CTDFARunner<Q0, "0101">::run());

    // Negative test cases
    static_assert(!CTDFARunner<Q0, "011">::run());
    static_assert(!CTDFARunner<Q0, "x">::run());
}

namespace DFA_BRANCH {
    using namespace DFA;
    struct Q0; struct Q1; struct Q2;
    struct Q0 : Node<false, Edge<'0', Q1>, Edge<'1', Q2>>{};
    struct Q1 : Node<true>{};
    struct Q2 : Node<true>{};
    static_assert(!CTDFARunner<Q0, "">::run());
    static_assert(CTDFARunner<Q0, "0">::run());
    static_assert(CTDFARunner<Q0, "1">::run());
}


int main() {
    return 0;
}