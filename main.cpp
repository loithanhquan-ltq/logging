#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>

#define PROFILING 1
#ifdef PROFILING
    #define PROFILE_SCOPE(name) \
        ProfilerTimer time##__LINE__(name)
    #define PROFILE_FUNCTION() \
        PROFILE_SCOPE(__FUNCTION__)
#endif

using namespace std;

struct ProfileResult {
  std::string name = "Default";
  long long start = 0;
  long long end = 0;
  size_t threadID = 0;
};

class Profiler {
  // Profiler writes timing results to a JSON file in the format:
  // {
  //   "profiles": [
  //     {"name": "...", "start": <microseconds>, "end": <microseconds>, "threadID": <hash>},
  //     ...
  //   ]
  // }
//   Each call to writeProfile appends one JSON object (comma separated).
  ofstream m_fout = ofstream("result.json");
  bool m_first = true; // track comma insertion
  Profiler() { // private constructor writes JSON header
    m_fout << "{\n\"profiles\": [\n";
  }
 public:
  static Profiler& Instance() {
    static Profiler instance;
    return instance;
  }

  void writeProfile(const ProfileResult& r)
  {
    static std::mutex s_mutex; // simple thread-safety for concurrent timers
    std::lock_guard<std::mutex> lock(s_mutex);
    if(!m_fout.is_open()) {
      return;
    }
    if(!m_first) {
      m_fout << ",\n";
    }
    m_first = false;
    // rudimentary JSON string escaping for quotes
    std::string safeName;
    safeName.reserve(r.name.size());
    for(char c : r.name) {
      if(c == '"') safeName += "\\\""; else safeName += c;
    }
    m_fout << "  {\"name\": \"" << safeName << "\", \"start\": " << r.start
           << ", \"end\": " << r.end << ", \"threadID\": " << r.threadID << "}";
  }
  ~Profiler() {
    if(m_fout.is_open()) {
      m_fout << "\n]\n}\n";
      m_fout.close();
    }
  }
};

class ProfilerTimer
{
    ProfileResult m_result;
    bool m_stopped = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_stp;
    void stop();
    public:
    ProfilerTimer(const std::string& name)
    {
        m_result.name = name;
        m_stp = std::chrono::high_resolution_clock::now();
    }
    ~ProfilerTimer()
    {
        stop();
    }

};

void ProfilerTimer::stop()
{
    using namespace std::chrono;
    if(m_stopped)
    {
        return;
    }
    m_stopped = true;
    auto etp = high_resolution_clock::now();
    m_result.start = time_point_cast<microseconds>(m_stp).time_since_epoch().count();
    m_result.end = time_point_cast<microseconds>(etp).time_since_epoch().count();
    m_result.threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());

    Profiler::Instance().writeProfile(m_result);
}

void printHello()
{
    PROFILE_FUNCTION();
    std::cout << "Hello, World!" << std::endl;
}

int main()
{
    printHello();
   return 0;
}