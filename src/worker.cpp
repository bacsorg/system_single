#include <bacs/system/single/worker.hpp>

#include <bacs/system/single/error.hpp>

#include <bunsan/log/trivial.hpp>
#include <bunsan/protobuf/binary.hpp>

#include <boost/algorithm/string/classification.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>

#include <algorithm>
#include <functional>

#include <cstdint>

namespace bacs {
namespace system {
namespace single {

worker::worker(bunsan::broker::task::channel &channel, test::storage_uptr tests,
               tester_uptr tester)
    : m_channel(channel),
      m_tests(std::move(tests)),
      m_tester(std::move(tester)) {
  m_intermediate.set_state(problem::single::IntermediateResult::INITIALIZED);
  send_intermediate();
}

void worker::test(const bacs::process::Buildable &solution,
                  const problem::Profile &profile) {
  problem::single::ProfileExtension extension;
  if (!profile.extension().UnpackTo(&extension)) {
    BOOST_THROW_EXCEPTION(profile_extension_error()
                          << profile_extension_error::message(
                              "Unable to unpack profile extension"));
  }
  BUNSAN_LOG_TRACE << "Testing " << profile.DebugString() << " extended with "
                   << extension.DebugString();
  check_hash() && build(solution) && test(extension);
  send_result();
}

bool worker::check_hash() {
  BUNSAN_LOG_TRACE << "Checking hash";
  // TODO
  m_result.mutable_system()->set_status(problem::single::SystemResult::OK);
  return true;
}

bool worker::build(const bacs::process::Buildable &solution) {
  BUNSAN_LOG_TRACE << "Building solution";
  m_intermediate.set_state(problem::single::IntermediateResult::BUILDING);
  send_intermediate();
  return m_tester->build(solution, *m_result.mutable_build());
}

bool worker::test(const problem::single::ProfileExtension &profile) {
  test(profile, m_result);
  // top-level testing is always successful
  return true;
}

void worker::send_intermediate() {
  m_broker_status.set_code(0);
  m_broker_status.clear_reason();
  m_broker_status.set_data(bunsan::protobuf::binary::to_string(m_intermediate));
  m_channel.send_status(m_broker_status);
}

void worker::send_result() {
  m_broker_result.set_status(bunsan::broker::Result::OK);
  m_broker_result.clear_reason();
  m_broker_result.set_data(bunsan::protobuf::binary::to_string(m_result));
  m_channel.send_result(m_broker_result);
}

namespace {
bool satisfies_requirement(
    const problem::single::TestGroupResult &result,
    const problem::single::Dependency::Requirement requirement) {
  using problem::single::Dependency;
  using problem::single::TestResult;

  BOOST_ASSERT(result.has_executed());
  if (!result.executed()) return false;

  std::size_t all_number = 0, ok_number = 0, fail_number = 0;
  for (const TestResult &test : result.test()) {
    ++all_number;
    if (test.status() == TestResult::OK) {
      ++ok_number;
    } else {
      ++fail_number;
    }
  }

  switch (requirement) {
    case Dependency::ALL_OK:
      return ok_number == all_number;
    case Dependency::ALL_FAIL:
      return fail_number == all_number;
    case Dependency::AT_LEAST_ONE_OK:
      return ok_number >= 1;
    case Dependency::AT_MOST_ONE_FAIL:
      return fail_number >= 1;
    case Dependency::AT_LEAST_HALF_OK:
      return ok_number * 2 >= all_number;
    default:
      BOOST_ASSERT(false);
  }
}
}  // namespace

void worker::test(const problem::single::ProfileExtension &profile,
                  problem::single::Result &result) {
  using problem::single::TestGroup;
  using problem::single::Dependency;
  using problem::single::TestGroupResult;

  m_intermediate.set_state(problem::single::IntermediateResult::TESTING);

  std::unordered_map<std::string, const TestGroup *> test_groups;
  std::unordered_map<std::string, TestGroupResult *> results;

  for (const TestGroup &test_group : profile.test_group()) {
    test_groups[test_group.id()] = &test_group;
    TestGroupResult &r = *result.add_test_group();
    results[test_group.id()] = &r;
    r.set_id(test_group.id());
  }

  std::unordered_set<std::string> in_test_group;
  const std::function<void(const TestGroup &)> run =
      [&](const TestGroup &test_group) {
        if (in_test_group.find(test_group.id()) != in_test_group.end())
          BOOST_THROW_EXCEPTION(
              test_group_circular_dependencies_error()
              << test_group_circular_dependencies_error::test_group(
                  test_group.id()));
        in_test_group.insert(test_group.id());
        BOOST_SCOPE_EXIT_ALL(&) { in_test_group.erase(test_group.id()); };

        TestGroupResult &result = *results.at(test_group.id());

        if (result.has_executed()) return;

        for (const Dependency &dependency : test_group.dependency()) {
          const auto iter = test_groups.find(dependency.test_group());
          if (iter == test_groups.end())
            BOOST_THROW_EXCEPTION(
                test_group_dependency_not_found_error()
                << test_group_dependency_not_found_error::test_group(
                       test_group.id())
                << test_group_dependency_not_found_error::test_group_dependency(
                       dependency.test_group()));
          const TestGroup &dep_test_group = *iter->second;

          run(dep_test_group);

          const TestGroupResult &dep_result =
              *results.at(dependency.test_group());
          if (!satisfies_requirement(dep_result, dependency.requirement())) {
            result.set_executed(false);
            return;
          }
        }
        result.set_executed(true);
        test(test_group, result);
      };

  for (const TestGroup &test_group : profile.test_group()) run(test_group);
}

bool worker::test(const problem::single::TestGroup &test_group,
                  problem::single::TestGroupResult &result) {
  BUNSAN_LOG_TRACE << "Testing test group = " << test_group.id();
  m_intermediate.set_test_group_id(test_group.id());
  result.set_id(test_group.id());
  result.set_executed(true);
  std::vector<std::string> test_sequence =
      m_tests->test_sequence(test_group.tests().query());
  std::function<bool(const std::string &, const std::string &)> less;
  switch (test_group.tests().order()) {
    case problem::single::test::Sequence::IDENTITY:
      // preserve order
      break;
    case problem::single::test::Sequence::NUMERIC:
      less = [](const std::string &left, const std::string &right) {
        return boost::lexical_cast<std::uint64_t>(left) <
               boost::lexical_cast<std::uint64_t>(right);
      };
      // strip from non-integer values
      test_sequence.erase(
          std::remove_if(test_sequence.begin(), test_sequence.end(),
                         [](const std::string &s) {
                           return !std::all_of(s.begin(), s.end(),
                                               boost::algorithm::is_digit());
                         }),
          test_sequence.end());
      break;
    case problem::single::test::Sequence::LEXICOGRAPHICAL:
      less = std::less<std::string>();
      break;
  }
  if (less) {
    std::sort(test_sequence.begin(), test_sequence.end(), less);
  }
  bool skip = false;
  for (const std::string &test_id : test_sequence) {
    const bool ret =
        !skip ? test(test_group.process(), test_id, *result.add_test())
              : skip_test(test_id, *result.add_test());
    switch (test_group.tests().continue_condition()) {
      case problem::single::test::Sequence::ALWAYS:
        // ret does not matter
        break;
      case problem::single::test::Sequence::WHILE_OK:
        if (!ret) skip = true;
        break;
    }
  }
  return true;  // note: empty test group is OK
}

bool worker::test(const problem::single::process::Settings &settings,
                  const std::string &test_id,
                  problem::single::TestResult &result) {
  BUNSAN_LOG_TRACE << "Testing test = " << test_id;
  m_intermediate.set_test_id(test_id);
  send_intermediate();
  const bool ret = m_tester->test(settings, m_tests->get(test_id), result);
  result.set_id(test_id);
  return ret;
}

bool worker::skip_test(const std::string &test_id,
                       problem::single::TestResult &result) {
  BUNSAN_LOG_TRACE << "Skipping test = " << test_id;
  m_intermediate.set_test_id(test_id);
  send_intermediate();
  result.set_id(test_id);
  result.set_status(problem::single::TestResult::SKIPPED);
  return false;
}

const boost::filesystem::path worker::PROBLEM_ROOT = "/problem_root";
const boost::filesystem::path worker::PROBLEM_BIN = PROBLEM_ROOT / "bin";
const boost::filesystem::path worker::PROBLEM_LIB = PROBLEM_ROOT / "lib";

}  // namespace single
}  // namespace system
}  // namespace bacs
