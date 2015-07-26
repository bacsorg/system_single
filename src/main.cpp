#include <bacs/system/single/main.hpp>

#include <bacs/system/single/checker.hpp>
#include <bacs/system/single/error.hpp>
#include <bacs/system/single/test/storage.hpp>
#include <bacs/system/single/tester.hpp>
#include <bacs/system/single/worker.hpp>

#include <bacs/problem/single/task.pb.h>

#include <yandex/contest/invoker/All.hpp>
#include <yandex/contest/system/Trace.hpp>
#include <yandex/contest/TypeInfo.hpp>

#include <bunsan/broker/task/stream_channel.hpp>
#include <bunsan/protobuf/binary.hpp>

#include <iostream>

namespace bacs {
namespace system {
namespace single {

using namespace yandex::contest::invoker;
namespace unistd = yandex::contest::system::unistd;

static ContainerConfig container_config(ContainerConfig config) {
  if (!config.lxcConfig.mount)
    config.lxcConfig.mount = yandex::contest::invoker::lxc::MountConfig();
  if (!config.lxcConfig.mount->entries)
    config.lxcConfig.mount->entries = std::vector<unistd::MountEntry>();

  filesystem::Directory dir;
  dir.mode = 0555;

  dir.path = worker::PROBLEM_ROOT;
  config.filesystemConfig.createFiles.push_back(filesystem::CreateFile(dir));

  const boost::filesystem::path bin = boost::filesystem::current_path() / "bin";
  dir.path = worker::PROBLEM_BIN;
  config.filesystemConfig.createFiles.push_back(filesystem::CreateFile(dir));
  if (boost::filesystem::exists(bin))
    config.lxcConfig.mount->entries->push_back(
        unistd::MountEntry::bindRO(bin, worker::PROBLEM_BIN));

  const boost::filesystem::path lib = boost::filesystem::current_path() / "lib";
  dir.path = worker::PROBLEM_LIB;
  config.filesystemConfig.createFiles.push_back(filesystem::CreateFile(dir));
  if (boost::filesystem::exists(lib))
    config.lxcConfig.mount->entries->push_back(
        unistd::MountEntry::bindRO(lib, worker::PROBLEM_LIB));

  return config;
}

int main(std::istream &in, std::ostream &out) {
  try {
    yandex::contest::system::Trace::handle(SIGABRT);
    bunsan::broker::task::stream_channel channel(out);
    auto tests_plugin =
        test::storage::load_plugin("lib/bacs_single_system_test_storage");
    auto checker_mapper_plugin = checker::result_mapper::load_plugin(
        "lib/bacs_single_system_checker_result_mapper");
    auto checker_plugin =
        checker::load_plugin("lib/bacs_single_system_checker");
    auto tester_mapper_plugin = tester::result_mapper::load_plugin(
        "lib/bacs_single_system_tester_result_mapper");
    auto tester_plugin = tester::load_plugin("lib/bacs_single_system_tester");
    auto container =
        Container::create(container_config(ContainerConfig::fromEnvironment()));
    auto tests = tests_plugin.unique_instance();
    auto checker_mapper = checker_mapper_plugin.unique_instance();
    auto checker =
        checker_plugin.unique_instance(container, std::move(checker_mapper));
    auto tester_mapper = tester_mapper_plugin.unique_instance();
    auto tester = tester_plugin.unique_instance(
        container, std::move(tester_mapper), std::move(checker));
    bacs::system::single::worker worker(channel, std::move(tests),
                                        std::move(tester));
    const auto task =
        bunsan::protobuf::binary::parse_make<problem::single::Task>(in);
    worker.test(task.solution(), task.profile());
    return 0;
  } catch (std::exception &e) {
    std::cerr << "Program terminated due to exception of type \""
              << yandex::contest::typeinfo::name(e) << "\"." << std::endl;
    std::cerr << "what() returns the following message:" << std::endl
              << e.what() << std::endl;
    return 1;
  }
}

}  // namespace single
}  // namespace system
}  // namespace bacs
