#define BOOST_TEST_MODULE test
#include <boost/test/unit_test.hpp>

#include <bacs/system/single/test/mock_storage.hpp>
#include <bunsan/test/test_tools.hpp>

#include <initializer_list>

namespace bps = bacs::problem::single;
namespace bss = bacs::system::single;

struct test_fixture {
  void set_test_set(std::initializer_list<std::string> t) {
    MOCK_EXPECT(tests.test_set).returns(std::unordered_set<std::string>(t));
  }

  std::vector<std::string> seq(std::initializer_list<std::string> t) {
    return std::vector<std::string>(t);
  }

  bss::test::mock_storage tests;
  google::protobuf::RepeatedPtrField<bps::test::Query> queries;
};

BOOST_FIXTURE_TEST_SUITE(test, test_fixture)

BOOST_AUTO_TEST_CASE(test_sequence) {
  set_test_set({"1", "2", "3"});
  queries.Add()->set_id("1");
  BOOST_CHECK(tests.test_sequence(queries) == seq({"1"}));
  queries.Add()->set_id("2");
  BOOST_CHECK(tests.test_sequence(queries) == seq({"1", "2"}));
  queries.Add()->set_id("3");
  BOOST_CHECK(tests.test_sequence(queries) == seq({"1", "2", "3"}));
}

BOOST_AUTO_TEST_SUITE_END()  // test
