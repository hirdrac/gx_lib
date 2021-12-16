//
// gx/Logger.cc
// Copyright (C) 2021 Richard Bradley
//

// TODO: threaded support
//   - IO operations handled on dedicated thread
//   - message string creation, time capture on calling thread
// TODO: naming threads
//   - at thread creation, thread ID given short name that is
//     used for logging instead of ID (main, render, log)

#include "Logger.hh"
#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <cstdio>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#if __has_include(<sys/syscall.h>)
#include <sys/syscall.h>
#endif
using namespace gx;


// **** Helper Functions ****
#ifdef SYS_gettid
// Linux specific thread id
// NOTE: 'gettid()' is available in glibc 2.30
//   for earlier versions, this function is needed
[[nodiscard]] static inline pid_t get_threadid() {
  return pid_t(syscall(SYS_gettid)); }
#else
// thread ID logging disabled
[[nodiscard]] static inline pid_t get_threadid() { return 0; }
#endif

namespace
{
  const pid_t mainThreadID = get_threadid();

  void logTime(std::ostream& os)
  {
    std::time_t t = std::time(nullptr);
    std::tm* td = std::localtime(&t);
    char str[32];
    int len = snprintf(str, sizeof(str), "%d-%02d-%02d %02d:%02d:%02d ",
		       td->tm_year + 1900, td->tm_mon + 1, td->tm_mday,
		       td->tm_hour, td->tm_min, td->tm_sec);
    os.write(str, std::streamsize(len));
  }

  [[nodiscard]] std::string fileTime()
  {
    std::time_t t = std::time(nullptr);
    std::tm* td = std::localtime(&t);
    char str[32];
    int len = snprintf(str, sizeof(str), "-%d%02d%02d_%02d%02d%02d",
		       td->tm_year + 1900, td->tm_mon + 1, td->tm_mday,
		       td->tm_hour, td->tm_min, td->tm_sec);
    return std::string(str, std::size_t(len));
  }

  [[nodiscard]] constexpr std::string_view levelStr(LogLevel l)
  {
    switch (l) {
      case LVL_TRACE: return "[TRACE] ";
      case LVL_INFO:  return "[INFO] ";
      case LVL_WARN:  return "[WARN] ";
      case LVL_ERROR: return "[ERROR] ";
      case LVL_FATAL: return "[FATAL] ";
      default:        return "[UNKNOWN] ";
    }
  }
}


// **** LoggerImpl class ****
class gx::LoggerImpl
{
 public:
  void setOStream(std::ostream& os)
  {
    std::lock_guard lg{_mutex};
    _fileName.clear();
    _fileStream.close();
    _os = &os;
  }

  void setFile(std::string_view fileName)
  {
    std::lock_guard lg{_mutex};
    _fileName = fileName;
    _fileStream = std::ofstream(_fileName, std::ios_base::app);
    _os = &_fileStream;
  }

  void log(const char* str, std::streamsize len)
  {
    std::lock_guard lg{_mutex};
    _os->write(str, len);
    _os->flush();
  }

  void log(std::string_view s) {
    return log(s.data(), std::streamsize(s.size())); }

  void rotate()
  {
    std::lock_guard lg{_mutex};
    if (!_fileStream) return;

    auto x = _fileName.rfind('.');
    std::string nf = _fileName.substr(0,x) + fileTime();
    if (x != std::string::npos) {
      nf += _fileName.substr(x);
    }

    _fileStream.close();
    std::rename(_fileName.c_str(), nf.c_str());
    _fileStream = std::ofstream(_fileName, std::ios_base::app);
  }

 private:
  std::ostream* _os = &std::cerr;
  std::ofstream _fileStream;
  std::string _fileName;
  std::mutex _mutex;
};


// **** Logger class ****
Logger::Logger() : _impl{new LoggerImpl} { }

Logger::~Logger() = default;
  // NOTE: default destructor needed here because LoggerImpl is not
  //   visible externally

void Logger::setOStream(std::ostream& os)
{
  _impl->setOStream(os);
}

void Logger::setFile(std::string_view fileName)
{
  _impl->setFile(fileName);
}

void Logger::rotate()
{
  _impl->rotate();
}

void Logger::header(std::ostringstream& os, LogLevel lvl)
{
  logTime(os);
  os << levelStr(lvl);
}

void Logger::logMsg(std::ostringstream& os, const char* file, int line)
{
  // footer
  os << " (";
  const pid_t tid = get_threadid();
  if (tid != mainThreadID) { os << "t=" << tid << ' '; }
  os << file << ':' << line << ")\n";

  // output to stream
  _impl->log(os.str());
}
