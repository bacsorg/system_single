#pragma once

#include <bacs/problem/single/result.pb.h>

#include <yandex/contest/invoker/Forward.hpp>

#include <bunsan/plugin.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace bacs {
namespace system {
namespace single {

class checker : private boost::noncopyable {
 public:
  class result_mapper : private boost::noncopyable {
    BUNSAN_PLUGIN_AUTO_BODY_NESTED(checker, result_mapper)
   public:
    virtual ~result_mapper() {}

    virtual problem::single::JudgeResult::Status map(int exit_status) = 0;
  };
  BUNSAN_PLUGIN_TYPES(result_mapper)

 private:
  BUNSAN_PLUGIN_AUTO_BODY(
      checker, const yandex::contest::invoker::ContainerPointer &container,
      result_mapper_uptr mapper)

 public:
  using file_map = std::unordered_map<std::string, boost::filesystem::path>;

  virtual ~checker() {}

  virtual bool check(const file_map &test_files, const file_map &solution_files,
                     problem::single::JudgeResult &result) = 0;
};
BUNSAN_PLUGIN_TYPES(checker)

}  // namespace single
}  // namespace system
}  // namespace bacs
