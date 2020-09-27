//
// gx/Logger.cc
// Copyright (C) 2020 Richard Bradley
//

#include "Logger.hh"
#include <string>
#include <string_view>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#if __has_include(<sys/syscall.h>)
#include <sys/syscall.h>
#endif


// **** Helper Functions ****
#ifdef SYS_gettid
// Linux specific thread id
// NOTE: 'gettid()' is available in glibc 2.30
//   for earlier versions, this function is needed
static inline pid_t get_threadid() { return syscall(SYS_gettid); }
#else
// thread ID logging disabled
static inline pid_t get_threadid() { return 0; }
#endif

namespace {
  const pid_t mainThreadID = get_threadid();

  void logTime(std::ostream& os)
  {
    std::time_t t = std::time(nullptr);
    std::tm* td = std::localtime(&t);
    char str[32];
    int len = snprintf(str, sizeof(str), "%d-%02d-%02d %02d:%02d:%02d ",
		       td->tm_year + 1900, td->tm_mon + 1, td->tm_mday,
		       td->tm_hour, td->tm_min, td->tm_sec);
    os.write(str, len);
  }

  std::string fileTime()
  {
    std::time_t t = std::time(nullptr);
    std::tm* td = std::localtime(&t);
    char str[32];
    int len = snprintf(str, sizeof(str), "-%d%02d%02d_%02d%02d%02d",
		       td->tm_year + 1900, td->tm_mon + 1, td->tm_mday,
		       td->tm_hour, td->tm_min, td->tm_sec);
    return std::string(str, len);
  }
}


// **** LoggerImpl class ****
class gx::LoggerImpl
{
 public:
  virtual ~LoggerImpl() = default;
  virtual void log(const char* str, std::streamsize len) = 0;
  virtual void rotate() { }

  void log(std::string_view s) { return log(s.data(), s.size()); }
};

namespace
{
  class OStreamLogger final : public gx::LoggerImpl
  {
   public:
    OStreamLogger(std::ostream& os) : _os(os) { }

    void log(const char* str, std::streamsize len) override
    {
      _os.write(str, len);
      _os.flush();
    }

   private:
    std::ostream& _os;
  };

  class FileLogger final : public gx::LoggerImpl
  {
   public:
    FileLogger(std::string_view file)
      : _filename(file), _os(_filename, std::ios_base::app) { }

    void log(const char* str, std::streamsize len) override
    {
      _os.write(str, len);
      _os.flush();
    }

    void rotate() override
    {
      auto x = _filename.rfind('.');
      std::string nf = _filename.substr(0,x) + fileTime();
      if (x != std::string::npos) {
	nf += _filename.substr(x);
      }

      _os.close();
      std::rename(_filename.c_str(), nf.c_str());
      _os = std::ofstream(_filename);
    }

   private:
    std::string _filename;
    std::ofstream _os;
  };

  constexpr std::string_view levelStr(gx::LogLevel l)
  {
    switch (l) {
      case gx::LVL_TRACE: return "[TRACE] ";
      case gx::LVL_INFO:  return "[INFO] ";
      case gx::LVL_WARN:  return "[WARN] ";
      case gx::LVL_ERROR: return "[ERROR] ";
      case gx::LVL_FATAL: return "[FATAL] ";
      default:            return "[UNKNOWN] ";
    }
  }
}


// **** Logger class ****
gx::Logger::Logger() : _level(LVL_INFO)
{
}

gx::Logger::~Logger() = default;
  // NOTE: default destructor needed here because LoggerImpl is not
  //   visible externally

void gx::Logger::setOStream(std::ostream& os)
{
  _impl.reset(new OStreamLogger(os));
}

void gx::Logger::setFile(std::string_view fileName)
{
  _impl.reset(new FileLogger(fileName));
}

void gx::Logger::rotate()
{
  if (_impl) { _impl->rotate(); }
}

void gx::Logger::header(std::ostringstream& os, LogLevel lvl)
{
  logTime(os);
  os << levelStr(lvl);
}

void gx::Logger::logMsg(std::ostringstream& os, const char* file, int line)
{
  // footer
  os << " (";
  pid_t tid = get_threadid();
  if (tid != mainThreadID) { os << "t=" << tid << ' '; }
  os << file << ':' << line << ")\n";

  // output to stream
  if (!_impl) { setOStream(std::cerr); }
  _impl->log(os.str());
}
