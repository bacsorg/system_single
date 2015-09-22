#pragma once

#include <bacs/problem/single/process/file.pb.h>

#include <boost/filesystem/path.hpp>

#include <sys/types.h>

namespace bacs {
namespace system {
namespace single {
namespace file {

void touch(const boost::filesystem::path &path);
mode_t mode(const problem::single::process::File::Permissions &value);
mode_t mode(const google::protobuf::RepeatedField<int> &permissions);

}  // namespace file
}  // namespace single
}  // namespace system
}  // namespace bacs
