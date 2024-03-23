#pragma once

#ifdef VE_PLATFORM_WINDOWS

#include <windows.h>
#include <thread>

void usleep(const int64_t sec)
{
  if (sec <= 0) {
    std::this_thread::yield(); //SwitchToThread();
    return;
  }
  const int32_t busywait = 2000;

  LARGE_INTEGER t0, freq;
  QueryPerformanceCounter(&t0);
  QueryPerformanceFrequency(&freq);
  if (sec - busywait > 0) {
    static bool resolutionSet = false;
    if (!resolutionSet) {
      HMODULE ntdll = GetModuleHandleA("ntdll.dll");
      if (ntdll) {
        auto ZwSetTimerResolution = (LONG(__stdcall*)(ULONG, BOOLEAN, PULONG))
          GetProcAddress(ntdll, "ZwSetTimerResolution");
        if (ZwSetTimerResolution) {
          ULONG actualResolution;
          ZwSetTimerResolution(1, true, &actualResolution);
        }
      }
      resolutionSet = true;
    }
    std::this_thread::sleep_for(std::chrono::microseconds(sec - busywait));
  }
  for (;;) {
    LARGE_INTEGER t1;
    QueryPerformanceCounter(&t1);
    int64_t waited = ((t1.QuadPart - t0.QuadPart) / freq.QuadPart) * 1000000LL +
      ((t1.QuadPart - t0.QuadPart) % freq.QuadPart) * 1000000LL / freq.QuadPart;
    if (waited >= sec) { break; }
    if (sec - waited > busywait / 10) { std::this_thread::yield(); } //SwitchToThread();
  }
}

#endif