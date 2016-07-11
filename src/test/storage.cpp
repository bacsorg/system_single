#include <bacs/system/single/test/storage.hpp>

#include <bacs/problem/single/test/matcher.hpp>

#include <boost/iterator/filter_iterator.hpp>

namespace bacs {
namespace system {
namespace single {
namespace test {

storage::~storage() {}

std::vector<std::string> storage::test_sequence(
    const google::protobuf::RepeatedPtrField<problem::single::TestQuery>
        &queries) {
  const std::unordered_set<std::string> full_set = test_set();
  std::vector<std::string> sequence;
  for (const problem::single::TestQuery &query : queries) {
    const problem::single::test::matcher matcher(query);
    sequence.insert(sequence.end(),
                    boost::make_filter_iterator(matcher, full_set.begin()),
                    boost::make_filter_iterator(matcher, full_set.end()));
  }
  return sequence;
}

storage::test storage::get(const std::string &test_id) {
  return test(*this, test_id);
}

storage::test::test(storage &storage_, const std::string &test_id)
    : m_storage(&storage_), m_test_id(test_id) {}

void storage::test::copy(const std::string &data_id,
                         const boost::filesystem::path &path) const {
  m_storage->copy(m_test_id, data_id, path);
}

boost::filesystem::path storage::test::location(
    const std::string &data_id) const {
  return m_storage->location(m_test_id, data_id);
}

std::unordered_set<std::string> storage::test::data_set() const {
  return m_storage->data_set();
}

}  // namespace test
}  // namespace single
}  // namespace system
}  // namespace bacs
