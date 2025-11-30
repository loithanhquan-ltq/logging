#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#define PROFILING 1
#ifdef PROFILING
#define PROFILE_SCOPE(name) ProfilerTimer time##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
#endif

using namespace std;

struct ProfileResult {
  std::string name = "Default";
  long long start = 0;
  long long end = 0;
  size_t threadID = 0;
};

class Profiler {
    std::string m_outfile = "result.json";
    size_t m_profileCount = 0;
    std::ofstream m_outputStream;
    std::mutex m_lock;
    Profiler()
    {
        m_outputStream = std::ofstream(m_outfile);
        writeHeader();
    }
    void writeHeader() {
        m_outputStream << "{\"otherData\": {},\"traceEvents\": [";
    }
    void writeFooter() {
        m_outputStream << "]}";
    }

 public:
  static Profiler& Instance() {
    static Profiler instance;
    return instance;
  }

  void writeProfile(const ProfileResult& result) {
    std::lock_guard<std::mutex> lock(m_lock);
    if(m_profileCount++ > 0) {
        m_outputStream << ",";
    }
    std::string name = result.name;
    std::replace(name.begin(), name.end(), '"', '\'');
    m_outputStream << "\n{";
    m_outputStream << "\"cat\":\"function\",";
    m_outputStream << "\"dur\":" << (result.end - result.start) << ",";
    m_outputStream << "\"name\":\"" << name << "\",";
    m_outputStream << "\"ph\":\"X\",";
    m_outputStream << "\"pid\":0,";
    m_outputStream << "\"tid\":" << result.threadID << ",";
    m_outputStream << "\"ts\":" << result.start;
    m_outputStream << "}";

  }
  ~Profiler() {
    writeFooter();
  }
};

class ProfilerTimer {
  ProfileResult m_result;
  bool m_stopped = false;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_stp;
  void stop();

 public:
  ProfilerTimer(const std::string& name) {
    m_result.name = name;
    m_stp = std::chrono::high_resolution_clock::now();
  }
  ~ProfilerTimer() { stop(); }
};

void ProfilerTimer::stop() {
  using namespace std::chrono;
  if (m_stopped) {
    return;
  }
  m_stopped = true;
  auto etp = high_resolution_clock::now();
  m_result.start =
      time_point_cast<microseconds>(m_stp).time_since_epoch().count();
  m_result.end = time_point_cast<microseconds>(etp).time_since_epoch().count();
  m_result.threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());

  Profiler::Instance().writeProfile(m_result);
}

void printHello() { std::cout << "Hello, World!" << std::endl; }

void foo() {
  PROFILE_FUNCTION();
  std::this_thread::sleep_for(std::chrono::microseconds(5));
  {
    PROFILE_SCOPE("sleep 100ms");
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }
}

int main() {
  PROFILE_FUNCTION();
  for (size_t i = 0; i < 10; i++) {
    PROFILE_SCOPE("LOOP");
    printHello();
  }
  foo();

  return 0;
}