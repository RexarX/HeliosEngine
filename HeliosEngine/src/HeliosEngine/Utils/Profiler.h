#pragma once

#include "Core.h"

#include <string_view>
#include <chrono>
#include <fstream>
#include <thread>
#include <mutex>

namespace Helios::Utils {
  class HELIOSENGINE_API Profiler {
  public:
    struct ProfileResult {
      std::string_view name;
      std::chrono::duration<double, std::micro> start;
      std::chrono::microseconds elapsedTime;
      std::thread::id threadID;
    };

    Profiler(const Profiler&) = delete;
    Profiler(Profiler&&) = delete;
    ~Profiler();

    void Clear();

    void BeginSession(std::string_view name);
    void EndSession();
    void WriteProfile(const ProfileResult& result);

    void SetActive(bool active);

    inline bool IsActive() const { return m_Active; }

    static inline Profiler& Get() {
      static Profiler profiler;
      return profiler;
    }

  private:
    Profiler() = default;

    void WriteHeader();
    void WriteFooter();
    void InternalEndSession();

  private:
    std::string m_CurrentSession;
    uint32_t m_ProfileCount = 0;
    std::ofstream m_OutputStream;
    bool m_Active = true;

    std::mutex m_Mutex;
  };

  class HELIOSENGINE_API ProfilerTimer {
  public:
    explicit ProfilerTimer(std::string_view name, bool singleUse = false);
    ~ProfilerTimer();

    void Stop();

  private:
    std::string_view m_Name;
    std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
    bool m_Stopped = false;
    bool m_SingleUse = false;
  };
}

#ifdef ENABLE_PROFILING
  #if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
    #define FUNC_SIG __PRETTY_FUNCTION__
  #elif defined(__DMC__) && (__DMC__ >= 0x810)
    #define FUNC_SIG __PRETTY_FUNCTION__
  #elif defined(__FUNCSIG__) || defined(_MSC_VER)
    #define FUNC_SIG __FUNCSIG__
  #elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
    #define FUNC_SIG __FUNCTION__
  #elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
    #define FUNC_SIG __FUNC__
  #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
    #define FUNC_SIG __func__
  #elif defined(__cplusplus) && (__cplusplus >= 201103)
    #define FUNC_SIG __func__
  #else
    #define FUNC_SIG "FUNC_SIG unknown!"
  #endif
    
  #define PROFILE_BEGIN_SESSION(name) Helios::Utils::Profiler::Get().BeginSession(name)
  #define PROFILE_END_SESSION() Helios::Utils::Profiler::Get().EndSession()

  #define PROFILE_SCOPE_LINE(name, line) Helios::Utils::ProfilerTimer timer_##line(name)
  #define PROFILE_SCOPE(name) PROFILE_SCOPE_LINE(name, __LINE__)
  #define PROFILE_FUNCTION() PROFILE_SCOPE(FUNC_SIG)

  #define PROFILE_SCOPE_LINE_ONCE(name, line) Helios::Utils::ProfilerTimer timer_##line(name, true)
  #define PROFILE_SCOPE_ONCE(name) PROFILE_SCOPE_LINE_ONCE(name, __LINE__)
  #define PROFILE_FUNCTION_ONCE() PROFILE_SCOPE_ONCE(FUNC_SIG)

  #define ACTIVATE_PROFILER() Helios::Utils::Profiler::Get().SetActive(true)
                              
  #define DEACTIVATE_PROFILER() Helios::Utils::Profiler::Get().SetActive(false)
#else
  #define PROFILE_BEGIN_SESSION(name)
  #define PROFILE_END_SESSION()
  #define PROFILE_SCOPE(name)
  #define PROFILE_FUNCTION()
  #define PROFILE_SCOPE_LINE_ONCE(name, line)
  #define PROFILE_SCOPE_ONCE(name)
  #define PROFILE_FUNCTION_ONCE()
  #define ACTIVATE_PROFILER()
  #define DEACTIVATE_PROFILER()
#endif