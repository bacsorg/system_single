#pragma once

#include <bacs/problem/single/problem.pb.h>
#include <bacs/problem/single/result.pb.h>
#include <bacs/problem/single/task.pb.h>
#include <bacs/process.pb.h>
#include <bacs/system/single/checker.hpp>
#include <bacs/system/single/test/storage.hpp>

#include <yandex/contest/invoker/Forward.hpp>

#include <bunsan/plugin.hpp>

#include <boost/noncopyable.hpp>

namespace bacs {
namespace system {
namespace single {

class tester : private boost::noncopyable {
 public:
  class result_mapper : private boost::noncopyable {
    BUNSAN_PLUGIN_AUTO_BODY_NESTED(tester, result_mapper)
   public:
    virtual ~result_mapper() {}
    virtual problem::single::JudgeResult::Status map(int exit_status) = 0;
  };
  BUNSAN_PLUGIN_TYPES(result_mapper)

 private:
  BUNSAN_PLUGIN_AUTO_BODY(
      tester, const yandex::contest::invoker::ContainerPointer &container,
      result_mapper_uptr mapper, checker_uptr checker)

 public:
  explicit tester();
  virtual ~tester() {}

  virtual bool build(const bacs::process::Buildable &solution,
                     bacs::process::BuildResult &result) = 0;

  virtual bool test(const problem::single::process::Settings &settings,
                    const test::storage::test &test,
                    problem::single::TestResult &result) = 0;
};
BUNSAN_PLUGIN_TYPES(tester)

}  // namespace single
}  // namespace system
}  // namespace bacs
