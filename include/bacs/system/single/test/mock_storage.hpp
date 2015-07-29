#pragma once

#include <bacs/system/single/test/storage.hpp>

#include <turtle/mock.hpp>

namespace bacs {
namespace system {
namespace single {
namespace test {

MOCK_BASE_CLASS(mock_storage, storage) {
  MOCK_METHOD(copy, 3, void(const std::string &test_id,
                            const std::string &data_id,
                            const boost::filesystem::path &path))
  MOCK_METHOD(location, 2, boost::filesystem::path(const std::string &test_id,
                                                   const std::string &data_id))
  MOCK_METHOD(data_set, 0, std::unordered_set<std::string>())
  MOCK_METHOD(test_set, 0, std::unordered_set<std::string>())
};

}  // namespace test
}  // namespace single
}  // namespace system
}  // namespace bacs
