#include <bacs/system/single/tester_util.hpp>

#include <bacs/file.hpp>
#include <bacs/system/file.hpp>
#include <bacs/system/process.hpp>
#include <bacs/system/single/file.hpp>

#include <boost/assert.hpp>
#include <boost/filesystem/operations.hpp>

namespace bacs {
namespace system {
namespace single {

tester_util_mkdir_hook::tester_util_mkdir_hook(
    const yandex::contest::invoker::ContainerPointer &container,
    const boost::filesystem::path &testing_root) {
  const boost::filesystem::path path =
      container->filesystem().keepInRoot(testing_root);
  // TODO permissions
  boost::filesystem::create_directories(path);
}

tester_util::tester_util(const ContainerPointer &container,
                         const boost::filesystem::path &testing_root)
    : tester_util_mkdir_hook(container, testing_root),
      m_container(container),
      m_testing_root(testing_root),
      m_container_testing_root(
          m_container->filesystem().keepInRoot(m_testing_root)) {
  reset();
}

void tester_util::reset() {
  try {
    m_tmpdir =
        bunsan::tempfile::directory_in_directory(m_container_testing_root);
    m_current_root = m_container->filesystem().containerPath(m_tmpdir.path());
    m_container->filesystem().setOwnerId(m_current_root, {0, 0});
    m_container->filesystem().setMode(m_current_root, 0500);

    m_process_group = m_container->createProcessGroup();
    m_receive.clear();
    m_test_files.clear();
    m_solution_files.clear();
    m_container_solution_files.clear();
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(reset_error() << bunsan::enable_nested_current());
  }
}

boost::filesystem::path tester_util::create_directory(
    const boost::filesystem::path &filename, const unistd::access::Id &owner_id,
    const mode_t mode) {
  try {
    const boost::filesystem::path path = m_current_root / filename;
    boost::filesystem::create_directory(
        m_container->filesystem().keepInRoot(path));
    m_container->filesystem().setOwnerId(path, owner_id);
    m_container->filesystem().setMode(path, mode);
    return path;
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(create_directory_error()
                          << bunsan::enable_nested_current());
  }
}

void tester_util::setup(const ProcessPointer &process,
                        const problem::single::process::Settings &settings) {
  try {
    process::setup(m_process_group, process, settings.resource_limits());
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(setup_error() << bunsan::enable_nested_current());
  }
}

void tester_util::set_test_files(
    const ProcessPointer &process,
    const problem::single::process::Settings &settings,
    const test::storage::test &test, const boost::filesystem::path &destination,
    const unistd::access::Id &owner_id, const mode_t mask) {
  try {
    add_test_files(settings, test, destination, owner_id, mask);
    set_redirections(process, settings);
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(set_test_files_error()
                          << bunsan::enable_nested_current());
  }
}

void tester_util::add_test_file(const problem::single::process::File &file,
                                const test::storage::test &test,
                                const boost::filesystem::path &destination,
                                const unistd::access::Id &owner_id,
                                const mode_t mask) {
  try {
    BOOST_ASSERT(destination.is_absolute());
    if (m_solution_files.find(file.id()) != m_solution_files.end()) {
      BOOST_THROW_EXCEPTION(error() << error::message("Duplicate file ids."));
    }
    // note: strip to filename
    const boost::filesystem::path location = m_solution_files[file.id()] =
        destination / (file.has_path()
                           ? bacs::file::path_cast(file.path()).filename()
                           : boost::filesystem::unique_path());
    m_container_solution_files[file.id()] =
        m_container->filesystem().keepInRoot(location);
    const mode_t mode = file::mode(file.permission()) & mask;
    if (file.has_init()) {
      copy_test_file(test, file.init(), location, owner_id, mode);
    } else {
      touch_test_file(location, owner_id, mode);
    }
    if (file.has_receive())
      m_receive.push_back({file.id(), location, file.receive()});
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(add_test_file_error()
                          << bunsan::enable_nested_current());
  }
}

void tester_util::set_test(const test::storage::test &test) {
  try {
    for (const std::string &data_id : test.data_set()) {
      m_test_files[data_id] = test.location(data_id);
    }
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(set_test_error() << bunsan::enable_nested_current());
  }
}

void tester_util::add_test_files(
    const problem::single::process::Settings &settings,
    const test::storage::test &test, const boost::filesystem::path &destination,
    const unistd::access::Id &owner_id, const mode_t mask) {
  try {
    set_test(test);
    for (const problem::single::process::File &file : settings.file()) {
      add_test_file(file, test, destination, owner_id, mask);
    }
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(add_test_files_error()
                          << bunsan::enable_nested_current());
  }
}

void tester_util::set_redirections(
    const ProcessPointer &process,
    const problem::single::process::Settings &settings) {
  try {
    for (const auto &redirection : settings.execution().redirection()) {
      const auto iter = m_solution_files.find(redirection.file_id());
      if (iter == m_solution_files.end())
        BOOST_THROW_EXCEPTION(error() << error::message("Invalid file id."));
      const boost::filesystem::path path = iter->second;
      switch (redirection.stream()) {
        case problem::single::process::Execution::Redirection::STDIN:
          process->setStream(0, File(path, AccessMode::READ_ONLY));
          break;
        case problem::single::process::Execution::Redirection::STDOUT:
          process->setStream(1, File(path, AccessMode::WRITE_ONLY));
          break;
        case problem::single::process::Execution::Redirection::STDERR:
          process->setStream(2, File(path, AccessMode::WRITE_ONLY));
          break;
      }
    }
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(set_redirections_error()
                          << bunsan::enable_nested_current());
  }
}

void tester_util::copy_test_file(const test::storage::test &test,
                                 const std::string &data_id,
                                 const boost::filesystem::path &destination,
                                 const unistd::access::Id &owner_id,
                                 const mode_t mode) {
  try {
    test.copy(data_id, m_container->filesystem().keepInRoot(destination));
    m_container->filesystem().setOwnerId(destination, owner_id);
    m_container->filesystem().setMode(destination, mode);
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(copy_test_file_error()
                          << bunsan::enable_nested_current());
  }
}

void tester_util::touch_test_file(const boost::filesystem::path &destination,
                                  const unistd::access::Id &owner_id,
                                  const mode_t mode) {
  try {
    file::touch(m_container->filesystem().keepInRoot(destination));
    m_container->filesystem().setOwnerId(destination, owner_id);
    m_container->filesystem().setMode(destination, mode);
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(touch_test_file_error()
                          << bunsan::enable_nested_current());
  }
}

std::string tester_util::read(const boost::filesystem::path &path,
                              const bacs::file::Range &range) {
  try {
    return bacs::system::file::read(m_container->filesystem().keepInRoot(path),
                                    range);
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(read_error() << bunsan::enable_nested_current());
  }
}

std::string tester_util::read_first(const boost::filesystem::path &path,
                                    const std::uint64_t size) {
  try {
    return bacs::system::file::read_first(
        m_container->filesystem().keepInRoot(path), size);
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(read_first_error()
                          << bunsan::enable_nested_current());
  }
}

std::string tester_util::read_last(const boost::filesystem::path &path,
                                   const std::uint64_t size) {
  try {
    return bacs::system::file::read_last(
        m_container->filesystem().keepInRoot(path), size);
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(read_last_error() << bunsan::enable_nested_current());
  }
}

void tester_util::send_file(bacs::problem::single::TestResult &result,
                            const std::string &id,
                            const bacs::file::Range &range,
                            const boost::filesystem::path &path) {
  try {
    problem::single::FileResult &file = *result.add_file();
    file.set_id(id);
    *file.mutable_data() = bacs::system::file::read(
        m_container->filesystem().keepInRoot(path), range);
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(send_file_error() << bunsan::enable_nested_current());
  }
}

void tester_util::send_file_if_requested(
    bacs::problem::single::TestResult &result,
    const problem::single::process::File &file,
    const boost::filesystem::path &path) {
  try {
    if (!file.has_receive()) return;
    send_file(result, file.id(), file.receive(), path);
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(send_file_if_requested_error()
                          << bunsan::enable_nested_current());
  }
}

void tester_util::send_test_files(bacs::problem::single::TestResult &result) {
  try {
    for (const receive_type &r : m_receive)
      send_file(result, r.id, r.range, r.path);
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(send_test_files_error()
                          << bunsan::enable_nested_current());
  }
}

void tester_util::redirect(const ProcessPointer &from, const int from_fd,
                           const ProcessPointer &to, const int to_fd) {
  try {
    const Pipe pipe = create_pipe();
    from->setStream(from_fd, pipe.writeEnd());
    to->setStream(to_fd, pipe.readEnd());
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(redirect_error() << bunsan::enable_nested_current());
  }
}

tester_util::ProcessGroup::Result tester_util::synchronized_call() {
  try {
    return m_process_group->synchronizedCall();
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(synchronized_call_error()
                          << bunsan::enable_nested_current());
  }
}

bool tester_util::parse_result(const Process::Result &process_result,
                               bacs::process::ExecutionResult &result) {
  try {
    return process::parse_result(m_process_group->result(), process_result,
                                 result);
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(parse_result_error()
                          << bunsan::enable_nested_current());
  }
}

void tester_util::use_solution_file(const std::string &data_id,
                                    const boost::filesystem::path &path) {
  try {
    m_solution_files[data_id] = path;
    m_container_solution_files[data_id] =
        m_container->filesystem().keepInRoot(path);
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(use_solution_file_error()
                          << bunsan::enable_nested_current());
  }
}

bool tester_util::ok(const problem::single::TestResult &result) const {
  return result.execution().status() == bacs::process::ExecutionResult::OK &&
         result.judge().status() == problem::single::JudgeResult::OK;
}

bool tester_util::run_checker_if_ok(checker &checker_,
                                    problem::single::TestResult &result) {
  try {
    if (!ok(result)) return false;
    return checker_.check(m_test_files, m_container_solution_files,
                          *result.mutable_judge());
  } catch (std::exception &) {
    BOOST_THROW_EXCEPTION(run_checker_if_ok_error()
                          << bunsan::enable_nested_current());
  }
}

bool tester_util::fill_status(problem::single::TestResult &result) {
  result.set_status(ok(result) ? problem::single::TestResult::OK
                               : problem::single::TestResult::FAILED);
  return result.status() == problem::single::TestResult::OK;
}

}  // namespace single
}  // namespace system
}  // namespace bacs
