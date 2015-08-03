#define BOOST_TEST_MODULE worker
#include <boost/test/unit_test.hpp>

#include <bacs/system/mock_system_verifier.hpp>
#include <bacs/system/single/mock_tester.hpp>
#include <bacs/system/single/test/mock_storage.hpp>
#include <bacs/system/single/worker.hpp>
#include <bunsan/broker/task/mock_channel.hpp>
#include <bunsan/protobuf/binary.hpp>
#include <bunsan/test/test_tools.hpp>

namespace proto = bunsan::protobuf;
namespace bp = bacs::problem;
namespace bs = bacs::system;
namespace bps = bp::single;
namespace bss = bs::single;

struct worker_fixture {
  bunsan::broker::task::mock_channel channel;
  std::unique_ptr<bs::mock_system_verifier> system_verifier =
      std::make_unique<bs::mock_system_verifier>();
  std::unique_ptr<bss::test::mock_storage> tests =
      std::make_unique<bss::test::mock_storage>();
  std::unique_ptr<bss::mock_tester> tester =
      std::make_unique<bss::mock_tester>();

  std::unordered_set<std::string> test_set;
  std::unordered_set<std::string> data_set = {"in", "out"};

  struct test_context {
    explicit test_context(std::string id_) : id(std::move(id_)) {}

    std::string id;
  };

  struct test_group_context {
    explicit test_group_context(std::string id_) : id(std::move(id_)) {}

    std::string id;
    bool executed = true;
    std::vector<test_context> tests;
  };
  std::vector<test_group_context> test_groups;

  bp::System system;
  bacs::process::Buildable solution;
  bp::Profile profile;
  bps::ProfileExtension profile_extension;
  bps::process::Settings process;
  bps::TestGroup *test_group = nullptr;

  worker_fixture() {
    system.set_problem_type("test problem");
    system.set_package("test/package");
    system.set_revision("test revision");
    solution.mutable_source()->set_data("solution");
    profile.set_description("mock profile");
    process.mutable_resource_limits()->set_time_limit_millis(1000);
    process.mutable_execution()->add_argument("binary");
    MOCK_EXPECT(system_verifier->verify)
        .calls([](const bp::System & /*system*/, bp::SystemResult &result) {
          result.set_status(bp::SystemResult::OK);
          return true;
        });
    MOCK_EXPECT(tests->test_set).calls([this] { return test_set; });
    MOCK_EXPECT(tests->data_set).calls([this] { return data_set; });
  }

  void pack_profile() {
    profile.mutable_extension()->PackFrom(profile_extension);
  }

  void add_test_group(const std::string &id = "") {
    test_groups.emplace_back(id);
    test_groups.back().id = id;
    test_group = profile_extension.add_test_group();
    test_group->set_id(id);
    *test_group->mutable_process() = process;
    test_group->mutable_tests()->set_order(bps::test::Sequence::IDENTITY);
    test_group->mutable_tests()->set_continue_condition(
        bps::test::Sequence::ALWAYS);
  }

  void add_test(const std::string &id, const bps::TestResult::Status status) {
    BOOST_REQUIRE(test_group);
    test_groups.back().tests.emplace_back(id);
    test_set.insert(id);
    test_group->mutable_tests()->add_query()->set_id(id);
    MOCK_EXPECT(tester->test)
        .once()
        .with(mock::any, mock::call([id](const bss::test::storage::test &test) {
          return test.id() == id;
        }),
              mock::any)
        .calls([id, status](const bps::process::Settings & /*settings*/,
                            const bss::test::storage::test & /*test*/,
                            bps::TestResult &result) {
          result.set_id(id);
          result.set_status(status);
          return status == bps::TestResult::OK;
        });
  }

  bss::worker make_worker() {
    return bss::worker(channel, std::move(system_verifier), std::move(tests),
                       std::move(tester));
  }

  auto expect_intermediate(const bps::IntermediateResult::State state) {
    return MOCK_EXPECT(channel.send_status)
        .once()
        .with(mock::call([state](const bunsan::broker::Status &status) {
          BOOST_CHECK_EQUAL(status.code(), 0);
          BOOST_CHECK_EQUAL(status.reason(), "");
          const auto result =
              proto::binary::parse_make<bps::IntermediateResult>(status.data());
          return result.state() == state;
        }));
  }

  auto expect_intermediate(const bps::IntermediateResult::State state,
                           const std::string &test_group_id,
                           const std::string &test_id) {
    return MOCK_EXPECT(channel.send_status)
        .once()
        .with(mock::call([state, test_group_id, test_id](
            const bunsan::broker::Status &status) {
          BOOST_CHECK_EQUAL(status.code(), 0);
          BOOST_CHECK_EQUAL(status.reason(), "");
          const auto result =
              proto::binary::parse_make<bps::IntermediateResult>(status.data());
          return result.state() == state &&
                 result.test_group_id() == test_group_id &&
                 result.test_id() == test_id;
        }));
  }

  auto expect_result() {
    auto w = MOCK_EXPECT(channel.send_result);
    w.once().calls([this](const bunsan::broker::Result &broker_result) {
      BOOST_CHECK_EQUAL(broker_result.status(), bunsan::broker::Result::OK);
      BOOST_CHECK_EQUAL(broker_result.reason(), "");
      const auto result =
          proto::binary::parse_make<bps::Result>(broker_result.data());
      BOOST_CHECK_EQUAL(result.system().status(), bp::SystemResult::OK);
      BOOST_CHECK_EQUAL(result.build().status(),
                        bacs::process::BuildResult::OK);
      BOOST_CHECK_EQUAL(result.build().output(), "build successful");
      BUNSAN_IF_CHECK_EQUAL(result.test_group_size(), test_groups.size()) {
        for (std::size_t i = 0; i < test_groups.size(); ++i) {
          const auto &tg = result.test_group(i);
          const auto &tgctx = test_groups[i];
          BOOST_CHECK_EQUAL(tg.id(), tgctx.id);
          BOOST_CHECK_EQUAL(tg.executed(), tgctx.executed);
          BUNSAN_IF_CHECK_EQUAL(tg.test_size(), tgctx.tests.size()) {
            for (std::size_t j = 0; j < tgctx.tests.size(); ++j) {
              const auto &t = tg.test(j);
              const auto &tctx = tgctx.tests[j];
              BOOST_CHECK_EQUAL(t.id(), tctx.id);
            }
          }
        }
      }
    });
    return w;
  }

  auto expect_build() {
    auto w = MOCK_EXPECT(tester->build);
    w.once().calls([this](const bacs::process::Buildable &buildable,
                          bacs::process::BuildResult &result) {
      BOOST_CHECK_EQUAL(&buildable, &solution);
      result.set_status(bacs::process::BuildResult::OK);
      result.set_output("build successful");
      return true;
    });
    return w;
  }
};

BOOST_FIXTURE_TEST_SUITE(worker, worker_fixture)

BOOST_AUTO_TEST_CASE(constructor) {
  expect_intermediate(bps::IntermediateResult::INITIALIZED);
  make_worker();
}

BOOST_AUTO_TEST_CASE(build) {
  mock::sequence s1;
  expect_intermediate(bps::IntermediateResult::INITIALIZED).in(s1);
  expect_intermediate(bps::IntermediateResult::BUILDING).in(s1);
  expect_build().in(s1);
  expect_result().in(s1);
  pack_profile();
  make_worker().test(system, solution, profile);
}

BOOST_AUTO_TEST_CASE(test_ok) {
  mock::sequence s1;
  expect_intermediate(bps::IntermediateResult::INITIALIZED).in(s1);
  expect_intermediate(bps::IntermediateResult::BUILDING).in(s1);
  expect_build().in(s1);
  expect_intermediate(bps::IntermediateResult::TESTING, "", "0").in(s1);
  expect_intermediate(bps::IntermediateResult::TESTING, "", "1").in(s1);
  expect_intermediate(bps::IntermediateResult::TESTING, "", "2").in(s1);
  expect_result().in(s1);
  add_test_group();
  add_test("0", bps::TestResult::OK);
  add_test("1", bps::TestResult::OK);
  add_test("2", bps::TestResult::OK);
  pack_profile();
  make_worker().test(system, solution, profile);
}

BOOST_AUTO_TEST_SUITE_END()  // worker
