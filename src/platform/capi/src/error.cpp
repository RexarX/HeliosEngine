#include <helios/capi/capi.h>

namespace {

constexpr unsigned kLastErrorCapacity = 512U;

thread_local char g_last_error[kLastErrorCapacity] = {};

}  // namespace

extern "C" {

const char* helios_last_error_message(void) {
  return g_last_error;
}

void helios_set_last_error(const char* message) {
  if (message == nullptr) {
    g_last_error[0] = '\0';
    return;
  }

  unsigned index = 0;
  for (; index + 1U < kLastErrorCapacity && message[index] != '\0'; ++index) {
    g_last_error[index] = message[index];
  }
  g_last_error[index] = '\0';
}

}  // extern "C"
