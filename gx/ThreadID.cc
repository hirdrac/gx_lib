//
// gx/ThreadID.cc
// Copyright (C) 2024 Richard Bradley
//

#include "ThreadID.hh"

#if __has_include(<sys/syscall.h>)
#include <unistd.h>
#include <sys/syscall.h>
#endif

#ifndef SYS_gettid
#include <thread>
#endif


const uint64_t gx::mainThreadID = gx::getThreadID();

uint64_t gx::getThreadID()
{
#ifdef SYS_gettid
  // Linux specific thread id
  // NOTE: 'gettid()' is available in glibc 2.30
  //   for earlier versions, this function is needed
  return static_cast<uint64_t>(syscall(SYS_gettid));
#else
  static const std::hash<std::thread::id> hasher;
  return hasher(std::this_thread::get_id());
#endif
}
