#include <bacs/system/single/file.hpp>

#include <bunsan/filesystem/fstream.hpp>

namespace bacs {
namespace system {
namespace single {
namespace file {

void touch(const boost::filesystem::path &path) {
  bunsan::filesystem::ofstream fout(path);
  fout.close();
}

mode_t mode(const problem::single::process::File::Permissions &value) {
  switch (value) {
    case problem::single::process::File::READ:
      return 0444;
    case problem::single::process::File::WRITE:
      return 0222;
    case problem::single::process::File::EXECUTE:
      return 0111;
    default:
      BOOST_ASSERT(false);
      return 0;
  }
}

mode_t mode(const google::protobuf::RepeatedField<int> &permissions) {
  mode_t m = 0;
  for (const int p : permissions) {
    m |= mode(static_cast<problem::single::process::File::Permissions>(p));
  }
  return m;
}

}  // namespace file
}  // namespace single
}  // namespace system
}  // namespace bacs
