#include "Profiler.h"

#include "pch.h"

namespace Helios::Utils {
  Profiler::~Profiler() {
    EndSession();
  }

  void Profiler::Clear() {
    if (m_CurrentSession.empty()) { CORE_ASSERT(false, "No active session!"); return; }

    if (!m_OutputStream.eof()) {
      m_ProfileCount = 0;
      m_OutputStream.close();
      m_OutputStream.open(m_CurrentSession, std::ofstream::out | std::ofstream::trunc);
      WriteHeader();
    }
  }

  void Profiler::BeginSession(std::string_view name) {
    if (!m_CurrentSession.empty()) {
      CORE_ERROR("Trying to start session '{}' when '{}' is already open!", name, m_CurrentSession);
      InternalEndSession();
    }

    std::string filepath = std::format("Profiling/{}.json", name);
    std::filesystem::create_directories("Profiling");

    m_OutputStream.open(filepath, std::ofstream::out | std::ofstream::trunc);
    if (m_OutputStream.is_open()) {
      m_CurrentSession = std::move(filepath);
      m_Active = true;
      WriteHeader();
    } else {
      CORE_ERROR("Profiler could not open file '{}'!", filepath);
    }
  }

  void Profiler::EndSession() {
    std::lock_guard lock(m_Mutex);
    InternalEndSession();
  }

  void Profiler::WriteProfile(const ProfileResult& result) {
    std::stringstream json;
    json << std::setprecision(3) << std::fixed;

    if (m_ProfileCount > 0) { json << ','; }
    json << '{';
    json << "\"cat\":\"function\",";
    json << "\"dur\":" << result.elapsedTime.count() << ',';
    json << "\"name\":\"" << result.name << "\",";
    json << "\"ph\":\"X\",";
    json << "\"pid\":0,";
    json << "\"tid\":" << result.threadID << ',';
    json << "\"ts\":" << result.start.count();
    json << '}';

    std::lock_guard lock(m_Mutex);
    if (!m_CurrentSession.empty()) {
      m_OutputStream << json.view();
      m_OutputStream.flush();
      ++m_ProfileCount;
    }
  }

  void Profiler::SetActive(bool active) {
    if (active) {
      Clear();
      m_Active = true;
    }
    else { m_Active = false; }
  }

  void Profiler::WriteHeader() {
    m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
    m_OutputStream.flush();
  }

  void Profiler::WriteFooter() {
    m_OutputStream << "]}";
    m_OutputStream.flush();
  }

  void Profiler::InternalEndSession() {
    if (!m_CurrentSession.empty()) {
      WriteFooter();
      m_OutputStream.close();
      m_CurrentSession.clear();
      m_ProfileCount = 0;
    }
  }

  ProfilerTimer::ProfilerTimer(std::string_view name, bool singleUse)
    : m_Name(name), m_SingleUse(singleUse) {
    if (Profiler::Get().IsActive()) {
      m_StartTimepoint = std::chrono::steady_clock::now();
    }
  }

  ProfilerTimer::~ProfilerTimer() {
    Profiler& profiler = Profiler::Get();
    if (!m_Stopped && profiler.IsActive()) { Stop(); }
    if (m_SingleUse) { profiler.SetActive(false); }
  }

  void ProfilerTimer::Stop() {
    auto start = std::chrono::duration<double, std::micro>(m_StartTimepoint.time_since_epoch());
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::time_point_cast<std::chrono::microseconds>(end).time_since_epoch() - 
                   std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

    Profiler::Get().WriteProfile({ m_Name, start, elapsed, std::this_thread::get_id() });

    m_Stopped = true;
  }
}