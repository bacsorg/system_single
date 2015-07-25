#define BOOST_TEST_MODULE out_stdout
#include <boost/test/unit_test.hpp>

#include <bacs/system/single/check.hpp>

using bacs::problem::single::JudgeResult;
using namespace bacs::system::single;

JudgeResult::Status test_equal(const std::string &out,
                               const std::string &hint) {
  std::stringstream out_(out), hint_(hint);
  return check::equal(out_, hint_);
}

BOOST_AUTO_TEST_CASE(ln) {
  std::stringstream ss("\n\n");
  BOOST_CHECK(check::seek_eof(ss) == JudgeResult::OK);
  ss.clear();
  ss.str("");
  BOOST_CHECK(check::seek_eof(ss) == JudgeResult::OK);
}

BOOST_AUTO_TEST_CASE(equal) {
  BOOST_CHECK(test_equal("", "") == JudgeResult::OK);
  BOOST_CHECK(test_equal("\n\n", "\n") == JudgeResult::OK);
  BOOST_CHECK(test_equal("n\n", "n") == JudgeResult::OK);
  BOOST_CHECK(test_equal("n", "n\n\n") == JudgeResult::OK);
  BOOST_CHECK(test_equal("n\nn", "n\nn\n") == JudgeResult::OK);
  BOOST_CHECK(test_equal("123", "1234") == JudgeResult::WRONG_ANSWER);
  BOOST_CHECK(test_equal("123\n", "1234") == JudgeResult::WRONG_ANSWER);
  BOOST_CHECK(test_equal("123\n", "123") == JudgeResult::OK);
}

// note: CR should not treated different
BOOST_AUTO_TEST_CASE(CR) {
  BOOST_CHECK(test_equal("\r\n", "\r") == JudgeResult::OK);
  BOOST_CHECK(test_equal("123", "\r123\r\n\n") == JudgeResult::OK);
  BOOST_CHECK(test_equal("\r\r123\n\n\n\n\r\n", "123\n\n") == JudgeResult::OK);
  BOOST_CHECK(test_equal("123\n", "123\r\n") == JudgeResult::OK);
  BOOST_CHECK(test_equal("1\r2\r3", "123\n") == JudgeResult::OK);
  BOOST_CHECK(test_equal("123\r\n321", "123\n321") == JudgeResult::OK);
}
