#pragma once

#include <bacs/problem/single/problem.pb.h>
#include <bacs/problem/single/result.pb.h>
#include <bacs/system/single/checker.hpp>
#include <bacs/system/single/error.hpp>
#include <bacs/system/single/test/storage.hpp>

#include <yandex/contest/invoker/All.hpp>

#include <bunsan/tempfile.hpp>

#include <boost/noncopyable.hpp>

#include <utility>

namespace bacs {
namespace system {
namespace single {

class tester_util_mkdir_hook : private boost::noncopyable {
 public:
  tester_util_mkdir_hook(
      const yandex::contest::invoker::ContainerPointer &container,
      const boost::filesystem::path &testing_root);
};

namespace unistd = yandex::contest::system::unistd;

class tester_util : private tester_util_mkdir_hook {
 public:
  using Container = yandex::contest::invoker::Container;
  using ContainerPointer = yandex::contest::invoker::ContainerPointer;
  using ProcessGroup = yandex::contest::invoker::ProcessGroup;
  using ProcessGroupPointer = yandex::contest::invoker::ProcessGroupPointer;
  using Process = yandex::contest::invoker::Process;
  using ProcessPointer = yandex::contest::invoker::ProcessPointer;
  using Pipe = yandex::contest::invoker::Pipe;
  using NotificationStream = yandex::contest::invoker::NotificationStream;
  using AccessMode = yandex::contest::invoker::AccessMode;
  using File = yandex::contest::invoker::File;

  struct error : virtual single::error {};

  struct receive_type {
    std::string id;
    boost::filesystem::path path;
    bacs::file::Range range;
  };

 public:
  tester_util(const ContainerPointer &container,
              const boost::filesystem::path &testing_root);

  ContainerPointer container() { return m_container; }
  ProcessGroupPointer process_group() { return m_process_group; }

  struct reset_error : virtual error {};
  void reset();

  struct create_process_error : virtual error {};
  template <typename... Args>
  ProcessPointer create_process(Args &&... args) {
    try {
      return m_process_group->createProcess(std::forward<Args>(args)...);
    } catch (std::exception &) {
      BOOST_THROW_EXCEPTION(create_process_error()
                            << bunsan::enable_nested_current());
    }
  }

  struct create_directory_error : virtual error {};
  boost::filesystem::path create_directory(
      const boost::filesystem::path &filename,
      const unistd::access::Id &owner_id, mode_t mode = 0700);

  struct setup_error : virtual error {};
  void setup(const ProcessPointer &process,
             const problem::single::process::Settings &settings);

  struct set_test_files_error : virtual error {};
  void set_test_files(const ProcessPointer &process,
                      const problem::single::process::Settings &settings,
                      const test::storage::test &test,
                      const boost::filesystem::path &destination,
                      const unistd::access::Id &owner_id, mode_t mask = 0777);

  struct add_test_files_error : virtual error {};
  void add_test_files(const problem::single::process::Settings &settings,
                      const test::storage::test &test,
                      const boost::filesystem::path &destination,
                      const unistd::access::Id &owner_id, mode_t mask = 0777);

  struct set_redirections_error : virtual error {};
  void set_redirections(
      const ProcessPointer &process,
      const problem::single::process::Settings &settings);

  struct copy_test_file_error : virtual error {};
  void copy_test_file(const test::storage::test &test,
                      const std::string &data_id,
                      const boost::filesystem::path &destination,
                      const unistd::access::Id &owner_id, mode_t mode);

  struct touch_test_file_error : virtual error {};
  void touch_test_file(const boost::filesystem::path &destination,
                       const unistd::access::Id &owner_id, mode_t mode);

  struct read_error : virtual error {};
  std::string read(const boost::filesystem::path &path,
                   const bacs::file::Range &range);

  struct read_first_error : virtual error {};
  std::string read_first(const boost::filesystem::path &path,
                         std::uint64_t size);

  struct read_last_error : virtual error {};
  std::string read_last(const boost::filesystem::path &path,
                        const std::uint64_t size);

  struct send_file_error : virtual error {};
  void send_file(bacs::problem::single::TestResult &result,
                 const std::string &id, const bacs::file::Range &range,
                 const boost::filesystem::path &path);

  struct send_file_if_requested_error : virtual error {};
  void send_file_if_requested(bacs::problem::single::TestResult &result,
                              const problem::single::process::File &file,
                              const boost::filesystem::path &path);

  struct send_test_files_error : virtual error {};
  void send_test_files(bacs::problem::single::TestResult &result);

  struct create_pipe_error : virtual error {};
  Pipe create_pipe() {
    try {
      return m_process_group->createPipe();
    } catch (std::exception &) {
      BOOST_THROW_EXCEPTION(create_pipe_error()
                            << bunsan::enable_nested_current());
    }
  }

  struct add_notifier_error : virtual error {};
  template <typename... Args>
  Pipe::End add_notifier(Args &&... args) {
    try {
      return m_process_group->addNotifier(std::forward<Args>(args)...);
    } catch (std::exception &) {
      BOOST_THROW_EXCEPTION(add_notifier_error()
                            << bunsan::enable_nested_current());
    }
  }

  struct redirect_error : virtual error {};
  void redirect(const ProcessPointer &from, int from_fd,
                const ProcessPointer &to, int to_fd);

  struct synchronized_call_error : virtual error {};
  ProcessGroup::Result synchronized_call();

  struct parse_result_error : virtual error {};
  bool parse_result(const Process::Result &process_result,
                    bacs::process::ExecutionResult &result);

  bool parse_result(const ProcessPointer &process,
                    bacs::process::ExecutionResult &result) {
    return parse_result(process->result(), result);
  }

  struct use_solution_file_error : virtual error {};
  void use_solution_file(const std::string &data_id,
                         const boost::filesystem::path &path);

  bool ok(const problem::single::TestResult &result) const;

  struct run_checker_if_ok_error : virtual error {};
  bool run_checker_if_ok(checker &checker_,
                         problem::single::TestResult &result);

  bool fill_status(problem::single::TestResult &result);

 private:
  const ContainerPointer m_container;
  const boost::filesystem::path m_testing_root;
  const boost::filesystem::path m_container_testing_root;
  bunsan::tempfile m_tmpdir;
  boost::filesystem::path m_current_root;

  ProcessGroupPointer m_process_group;

  std::vector<receive_type> m_receive;
  checker::file_map m_test_files, m_solution_files, m_container_solution_files;
};

}  // namespace single
}  // namespace system
}  // namespace bacs
