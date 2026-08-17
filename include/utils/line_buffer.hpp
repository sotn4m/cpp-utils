#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

namespace utils {

template <std::size_t InitilaCapactiy = 1024>
class line_buffer {
 public:
  explicit line_buffer () { buffer_.reserve (InitilaCapactiy); }

  auto append (std::string_view chunk) -> void { buffer_.append (chunk); }

  [[nodiscard]] auto pop_line () -> std::optional<std::string> {
    const auto it = std::ranges::find (buffer_, '\n');
    if (it == buffer_.end ()) {
      return std::nullopt;
    }

    const auto pos = static_cast<std::size_t> (it - buffer_.begin ());
    auto line = buffer_.substr (0, pos);
    buffer_.erase (0, pos + 1);

    if (!line.empty () && line.back () == '\r') {
      line.pop_back ();
    }
    return line;
  }

  [[nodiscard]] auto empty () const -> bool { return buffer_.empty (); }

 private:
  std::string buffer_ {};
};

}  // namespace utils
