#pragma once

#include <bacs/system/error.hpp>

namespace bacs {
namespace system {
namespace single {

struct error : virtual bacs::system::error {};

struct profile_error : virtual error {};
struct profile_extension_error : virtual profile_error {};

struct test_group_error : virtual error {
  using test_group = boost::error_info<struct tag_test_group, std::string>;
};
struct test_group_dependency_error : virtual test_group_error {
  using test_group_dependency =
      boost::error_info<struct tag_test_group, std::string>;
};
struct test_group_dependency_not_found_error
    : virtual test_group_dependency_error {};
struct test_group_circular_dependencies_error
    : virtual test_group_dependency_error {};

}  // namespace single
}  // namespace system
}  // namespace bacs
