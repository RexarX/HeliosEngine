#include <doctest/doctest.h>

#include <helios/profile/tracy/lock.hpp>

#include <mutex>
#include <shared_mutex>

TEST_SUITE("helios::profile Lock Macros (Tracy)") {
  TEST_CASE("HELIOS_PROFILE_LOCKABLE") {
    SUBCASE("Macro compiles and declares a lockable variable") {
      HELIOS_PROFILE_LOCKABLE(std::mutex, test_mutex);
      CHECK(true);
    }
  }

  TEST_CASE("HELIOS_PROFILE_SHARED_LOCKABLE") {
    SUBCASE("Macro compiles and declares a shared lockable variable") {
      HELIOS_PROFILE_SHARED_LOCKABLE(std::shared_mutex, test_mutex);
      CHECK(true);
    }
  }

  TEST_CASE("HELIOS_PROFILE_LOCK_MARK") {
    SUBCASE("Macro compiles on a lockable variable") {
      HELIOS_PROFILE_LOCKABLE(std::mutex, mtx);
      HELIOS_PROFILE_LOCK_MARK(mtx);
      CHECK(true);
    }
  }

  TEST_CASE("HELIOS_PROFILE_LOCK_NAME") {
    SUBCASE("Macro compiles with a name") {
      HELIOS_PROFILE_LOCKABLE(std::mutex, mtx);
      HELIOS_PROFILE_LOCK_NAME(mtx, "UILock");
      CHECK(true);
    }
  }
}
