#pragma once

#include <bacs/system/single/tester.hpp>

#include <turtle/mock.hpp>

namespace bacs {
namespace system {
namespace single {

MOCK_BASE_CLASS(mock_tester, tester) {
  MOCK_BASE_CLASS(mock_result_mapper, result_mapper) {
    MOCK_METHOD(map, 1, problem::single::JudgeResult::Status(int exit_status));
  };

  MOCK_METHOD(build, 2, bool(const bacs::process::Buildable &solution,
                             bacs::process::BuildResult &result))
  MOCK_METHOD(test, 3, bool(const problem::single::process::Settings &settings,
                            const test::storage::test &test,
                            problem::single::TestResult &result))
};

}  // namespace single
}  // namespace system
}  // namespace bacs
