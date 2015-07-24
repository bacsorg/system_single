#pragma once

#include <bacs/system/single/callback.hpp>
#include <bacs/system/single/test/storage.hpp>
#include <bacs/system/single/tester.hpp>

#include <bacs/problem/single/problem.pb.h>
#include <bacs/problem/single/result.pb.h>

namespace bacs {
namespace system {
namespace single {

class worker {
 public:
  static const boost::filesystem::path PROBLEM_ROOT;
  static const boost::filesystem::path PROBLEM_BIN;
  static const boost::filesystem::path PROBLEM_LIB;

 public:
  worker(const problem::single::Task::Callbacks &callbacks,
         test::storage_uptr tests, tester_uptr tester);

  void test(const bacs::process::Buildable &solution,
            const problem::single::ProfileExtension &profile);

 private:
  bool check_hash();

  bool build(const bacs::process::Buildable &solution);

  bool test(const problem::single::ProfileExtension &profile);

  bool skip_test(const std::string &test_id,
                 problem::single::TestResult &result);

 private:
  void send_intermediate();
  void send_result();

 private:
  /// Test submit.
  void test(const problem::single::ProfileExtension &profile,
            problem::single::Result &result);

  /// Test single test group.
  bool test(const problem::single::TestGroup &test_group,
            problem::single::TestGroupResult &result);

  /// Test single test.
  bool test(const problem::single::process::Settings &settings,
            const std::string &test_id, problem::single::TestResult &result);

 private:
  test::storage_uptr m_tests;
  tester_uptr m_tester;
  callback::result m_result_cb;
  callback::intermediate m_intermediate_cb;
  problem::single::IntermediateResult m_intermediate;
  problem::single::Result m_result;
};

}  // namespace single
}  // namespace system
}  // namespace bacs
