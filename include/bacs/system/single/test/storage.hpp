#pragma once

#include <bacs/problem/single/problem.pb.h>

#include <bunsan/plugin.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <unordered_set>

namespace bacs {
namespace system {
namespace single {
namespace test {

class storage : private boost::noncopyable {
  BUNSAN_PLUGIN_AUTO_BODY(storage)
 public:
  class test;

  virtual ~storage();

  virtual void copy(const std::string &test_id, const std::string &data_id,
                    const boost::filesystem::path &path) = 0;

  virtual boost::filesystem::path location(const std::string &test_id,
                                           const std::string &data_id) = 0;
  virtual std::unordered_set<std::string> data_set() = 0;
  virtual std::unordered_set<std::string> test_set() = 0;

  std::vector<std::string> test_sequence(
      const google::protobuf::RepeatedPtrField<
          problem::single::test::Query> &queries);

  test get(const std::string &test_id);
};
BUNSAN_PLUGIN_TYPES(storage)

class storage::test {
 public:
  test() = default;
  test(const test &) = default;
  test &operator=(const test &) = default;

  test(storage &storage_, const std::string &test_id);

  void copy(const std::string &data_id,
            const boost::filesystem::path &path) const;

  boost::filesystem::path location(const std::string &data_id) const;

  std::unordered_set<std::string> data_set() const;

 private:
  storage *m_storage = nullptr;
  std::string m_test_id;
};

}  // namespace test
}  // namespace single
}  // namespace system
}  // namespace bacs
