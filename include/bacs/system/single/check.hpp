#pragma once

#include <bacs/problem/single/result.pb.h>

namespace bacs {
namespace system {
namespace single {
namespace check {

problem::single::JudgeResult::Status equal(std::istream &out,
                                           std::istream &hint);
problem::single::JudgeResult::Status seek_eof(std::istream &in);

}  // namespace check
}  // namespace single
}  // namespace system
}  // namespace bacs
