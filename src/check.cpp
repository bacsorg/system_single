#include <bacs/system/single/check.hpp>

namespace bacs {
namespace system {
namespace single {
namespace check {

problem::single::JudgeResult::Status equal(std::istream &out,
                                           std::istream &hint) {
  // read all symbols and compare
  char o, c;
  do {
    while (out.get(o) && o == '\r') continue;
    while (hint.get(c) && c == '\r') continue;
    if (out && hint && o != c)
      return problem::single::JudgeResult::WRONG_ANSWER;
  } while (out && hint);
  if (out.eof() && hint.eof()) {
    return problem::single::JudgeResult::OK;
  } else {
    if (out.eof() && hint) {
      hint.putback(c);
      return seek_eof(hint);
    } else if (out && hint.eof()) {
      out.putback(o);
      return seek_eof(out);
    } else {
      return problem::single::JudgeResult::WRONG_ANSWER;
    }
  }
}

problem::single::JudgeResult::Status seek_eof(std::istream &in) {
  char c;
  while (in.get(c)) {
    if (c != '\n' && c != '\r')
      return problem::single::JudgeResult::WRONG_ANSWER;
  }
  return problem::single::JudgeResult::OK;
}

}  // namespace check
}  // namespace single
}  // namespace system
}  // namespace bacs
