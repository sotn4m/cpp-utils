#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>

namespace utils {

// RAII guard: creates a directory on construction and recursively deletes it
// on destruction. Throws if the path already exists. Sets mode 0700 on Unix.
// Not copyable; movable. Moved-from objects own nothing.
class scoped_temp_dir {
 public:
  explicit scoped_temp_dir (std::filesystem::path path);
  scoped_temp_dir (std::filesystem::path base, std::string_view name);
  ~scoped_temp_dir () noexcept;
  scoped_temp_dir (const scoped_temp_dir& other) = delete;
  scoped_temp_dir& operator= (const scoped_temp_dir& other) = delete;
  scoped_temp_dir (scoped_temp_dir&& other) noexcept;
  scoped_temp_dir& operator= (scoped_temp_dir&& other) noexcept;

  [[nodiscard]] const std::filesystem::path& path () const noexcept;

 private:
  static void create_at (const std::filesystem::path& path);
  static void remove_at (const std::filesystem::path& path) noexcept;

  std::filesystem::path path_;
};

inline void scoped_temp_dir::create_at (const std::filesystem::path& path) {
  if (path.empty ()) {
    throw std::filesystem::filesystem_error {
        "scoped_temp_dir: empty path",
        path,
        std::make_error_code (std::errc::invalid_argument),
    };
  }

  if (std::filesystem::exists (path)) {
    throw std::filesystem::filesystem_error {
        "scoped_temp_dir: path already exists",
        path,
        std::make_error_code (std::errc::file_exists),
    };
  }

  std::filesystem::create_directories (path);

#ifndef _WIN32
  std::filesystem::permissions (path, std::filesystem::perms::owner_all,
                                std::filesystem::perm_options::replace);
#endif
}

inline void scoped_temp_dir::remove_at (
    const std::filesystem::path& path) noexcept {
  if (path.empty ()) {
    return;
  }

  try {
    if (std::filesystem::exists (path)) {
      std::filesystem::remove_all (path);
    }
  } catch (...) {
  }
}

inline scoped_temp_dir::scoped_temp_dir (std::filesystem::path path)
    : path_ {std::move (path)} {
  create_at (path_);
}

inline scoped_temp_dir::scoped_temp_dir (std::filesystem::path base,
                                         std::string_view name)
    : path_ {std::move (base) / name} {
  create_at (path_);
}

inline scoped_temp_dir::~scoped_temp_dir () noexcept {
  remove_at (path_);
}

inline scoped_temp_dir::scoped_temp_dir (scoped_temp_dir&& other) noexcept
    : path_ {std::move (other.path_)} {
  other.path_.clear ();
}

inline scoped_temp_dir& scoped_temp_dir::operator= (
    scoped_temp_dir&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  remove_at (path_);
  path_ = std::move (other.path_);
  other.path_.clear ();
  return *this;
}

inline const std::filesystem::path& scoped_temp_dir::path () const noexcept {
  return path_;
}

}  // namespace utils
