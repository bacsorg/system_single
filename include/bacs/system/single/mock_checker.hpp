#pragma once

#include <bacs/system/single/checker.hpp>

#include <turtle/mock.hpp>

namespace bacs {
namespace system {
namespace single {

MOCK_BASE_CLASS(mock_checker, checker) {
  MOCK_BASE_CLASS(mock_result_mapper, result_mapper) {
    MOCK_METHOD(map, 1, problem::single::JudgeResult::Status(int exit_status));
  };

  MOCK_METHOD(check, 3, bool(const file_map &test_files,
                             const file_map &solution_files,
                             problem::single::JudgeResult &result))
};

}  // namespace single
}  // namespace system
}  // namespace bacs
