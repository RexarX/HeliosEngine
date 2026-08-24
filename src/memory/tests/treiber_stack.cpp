#include <doctest/doctest.h>

#include <helios/memory/treiber_stack.hpp>

#include <algorithm>
#include <array>
#include <barrier>
#include <cstddef>
#include <thread>
#include <utility>
#include <vector>

using namespace helios::mem;

namespace {

struct Node {
  Node* next = nullptr;
  int id = 0;
};

}  // namespace

TEST_SUITE("helios::mem::TreiberStack") {
  TEST_CASE("helios::mem::TreiberStack::ctor") {
    SUBCASE("Default constructed stack is empty") {
      const TreiberStack stack;
      CHECK(stack.Empty());
      CHECK_EQ(stack.Top(), nullptr);
    }
  }

  TEST_CASE("helios::mem::TreiberStack::ctor(TreiberStack&&)") {
    SUBCASE("Moved-into stack holds the nodes") {
      Node node{.id = 1};
      TreiberStack source;
      source.Push(&node);

      TreiberStack moved(std::move(source));

      CHECK_EQ(moved.Top(), &node);
      CHECK_FALSE(moved.Empty());
    }

    SUBCASE("Moved-from stack is empty") {
      Node node{.id = 1};
      TreiberStack source;
      source.Push(&node);

      TreiberStack moved(std::move(source));

      CHECK(source.Empty());
      CHECK_EQ(source.Top(), nullptr);
    }
  }

  TEST_CASE("helios::mem::TreiberStack::operator=") {
    SUBCASE("Move assignment transfers nodes") {
      Node node{.id = 1};
      TreiberStack source;
      source.Push(&node);
      TreiberStack dest;

      dest = std::move(source);

      CHECK_EQ(dest.Pop(), &node);
      CHECK(source.Empty());
    }

    SUBCASE("Self move assignment is a no-op") {
      Node node{.id = 1};
      TreiberStack stack;
      stack.Push(&node);

      stack = std::move(stack);

      CHECK_EQ(stack.Top(), &node);
    }
  }

  TEST_CASE("helios::mem::TreiberStack::Push") {
    SUBCASE("Pushes a node that Pop returns") {
      Node node{.id = 7};
      TreiberStack stack;

      stack.Push(&node);

      CHECK_EQ(stack.Pop(), &node);
    }

    SUBCASE("LIFO order for two nodes") {
      Node first{.id = 1};
      Node second{.id = 2};
      TreiberStack stack;

      stack.Push(&first);
      stack.Push(&second);

      CHECK_EQ(stack.Pop(), &second);
      CHECK_EQ(stack.Pop(), &first);
    }

    SUBCASE("Concurrent pushes preserve every node") {
      constexpr size_t kThreads = 8;
      constexpr size_t kNodesPerThread = 64;
      std::array<std::array<Node, kNodesPerThread>, kThreads> nodes = {};
      TreiberStack stack;
      std::barrier start(static_cast<std::ptrdiff_t>(kThreads));
      std::vector<std::thread> threads;
      threads.reserve(kThreads);

      for (size_t index = 0; index < kThreads; ++index) {
        threads.emplace_back([&, index] {
          start.arrive_and_wait();
          for (size_t node = 0; node < kNodesPerThread; ++node) {
            nodes[index][node].id =
                static_cast<int>(index * kNodesPerThread + node);
            stack.Push(&nodes[index][node]);
          }
        });
      }

      for (auto& thread : threads) {
        thread.join();
      }

      size_t popped = 0;
      while (stack.Pop() != nullptr) {
        ++popped;
      }
      CHECK_EQ(popped, kThreads * kNodesPerThread);
    }
  }

  TEST_CASE("helios::mem::TreiberStack::Pop") {
    SUBCASE("Returns nullptr on empty stack") {
      TreiberStack stack;
      CHECK_EQ(stack.Pop(), nullptr);
    }

    SUBCASE("Returns last pushed node") {
      Node node{.id = 3};
      TreiberStack stack;
      stack.Push(&node);

      CHECK_EQ(stack.Pop(), &node);
      CHECK(stack.Empty());
    }

    SUBCASE(
        "Interleaved push/pop returns unique live nodes after 16-bit wrap") {
      constexpr size_t kThreads = 8;
      // 8 threads × 4096 rounds × {pop, push} == 65536 ops, which wraps a
      // 16-bit Treiber ABA tag. The tagged head must survive that wrap.
      constexpr size_t kRounds = 4096;
      std::array<Node, kThreads> nodes = {};
      TreiberStack stack;
      for (size_t index = 0; index < kThreads; ++index) {
        nodes[index].id = static_cast<int>(index);
        stack.Push(&nodes[index]);
      }

      std::vector<void*> held(kThreads);
      std::barrier sync(static_cast<std::ptrdiff_t>(kThreads + 1));
      std::vector<std::thread> threads;
      threads.reserve(kThreads);

      for (size_t index = 0; index < kThreads; ++index) {
        threads.emplace_back([&, index] {
          for (size_t round = 0; round < kRounds; ++round) {
            sync.arrive_and_wait();
            held[index] = stack.Pop();
            sync.arrive_and_wait();
          }
        });
      }

      for (size_t round = 0; round < kRounds; ++round) {
        sync.arrive_and_wait();
        sync.arrive_and_wait();

        std::vector<void*> round_ptrs = held;
        std::ranges::sort(round_ptrs);
        CHECK_EQ(std::ranges::adjacent_find(round_ptrs), round_ptrs.end());

        for (void* ptr : held) {
          CHECK_NE(ptr, nullptr);
          if (ptr != nullptr) {
            stack.Push(ptr);
          }
        }
      }

      for (auto& thread : threads) {
        thread.join();
      }

      CHECK_FALSE(stack.Empty());
    }
  }

  TEST_CASE("helios::mem::TreiberStack::Clear") {
    SUBCASE("Empty after Clear") {
      Node node{.id = 1};
      TreiberStack stack;
      stack.Push(&node);

      stack.Clear();

      CHECK(stack.Empty());
      CHECK_EQ(stack.Pop(), nullptr);
    }

    SUBCASE("Clear on empty stack is safe") {
      TreiberStack stack;
      stack.Clear();
      CHECK(stack.Empty());
    }

    SUBCASE("Clear does not visit nodes") {
      Node first{.id = 1};
      Node second{.id = 2};
      TreiberStack stack;
      stack.Push(&first);
      stack.Push(&second);

      stack.Clear();

      CHECK_EQ(TreiberStack::Next(&second), &first);
      CHECK_EQ(TreiberStack::Next(&first), nullptr);
    }
  }

  TEST_CASE("helios::mem::TreiberStack::Empty") {
    SUBCASE("True for a default stack") {
      const TreiberStack stack;
      CHECK(stack.Empty());
    }

    SUBCASE("False after Push") {
      Node node{.id = 1};
      TreiberStack stack;
      stack.Push(&node);
      CHECK_FALSE(stack.Empty());
    }
  }

  TEST_CASE("helios::mem::TreiberStack::Top") {
    SUBCASE("Returns nullptr when empty") {
      const TreiberStack stack;
      CHECK_EQ(stack.Top(), nullptr);
    }

    SUBCASE("Returns last pushed node without removing it") {
      Node node{.id = 4};
      TreiberStack stack;
      stack.Push(&node);

      CHECK_EQ(stack.Top(), &node);
      CHECK_EQ(stack.Top(), &node);
      CHECK_FALSE(stack.Empty());
    }
  }

  TEST_CASE("helios::mem::TreiberStack::Next") {
    SUBCASE("Returns nullptr for a single pushed node") {
      Node node{.id = 1};
      TreiberStack stack;
      stack.Push(&node);

      CHECK_EQ(TreiberStack::Next(&node), nullptr);
    }

    SUBCASE("Returns the previous top after a second Push") {
      Node first{.id = 1};
      Node second{.id = 2};
      TreiberStack stack;
      stack.Push(&first);
      stack.Push(&second);

      CHECK_EQ(stack.Top(), &second);
      CHECK_EQ(TreiberStack::Next(&second), &first);
      CHECK_EQ(TreiberStack::Next(&first), nullptr);
    }
  }
}
