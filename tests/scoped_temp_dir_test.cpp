#include <utils/scoped_temp_dir.hpp>

#include <atomic>
#include <fstream>
#include <stdexcept>

#include <gtest/gtest.h>

namespace {

std::filesystem::path unique_path (std::string_view suffix) {
  static std::atomic<int> counter {0};
  return std::filesystem::temp_directory_path () /
         ("cpp_utils_scoped_temp_dir_" + std::to_string (counter++)) / suffix;
}

TEST (ScopedTempDirTest, CreatesDirectory) {
  const auto dir_path = unique_path ("creates");

  {
    utils::scoped_temp_dir dir {dir_path};
    EXPECT_TRUE (std::filesystem::exists (dir.path ()));
    EXPECT_TRUE (std::filesystem::is_directory (dir.path ()));
    EXPECT_EQ (dir.path (), dir_path);
  }

  EXPECT_FALSE (std::filesystem::exists (dir_path));
}

TEST (ScopedTempDirTest, BaseAndNameConstructor) {
  const auto base = unique_path ("base");
  const auto name = std::string_view {"job-42"};
  const auto expected = base / name;

  {
    utils::scoped_temp_dir dir {base, name};
    EXPECT_EQ (dir.path (), expected);
    EXPECT_TRUE (std::filesystem::is_directory (dir.path ()));
  }

  EXPECT_FALSE (std::filesystem::exists (expected));
}

TEST (ScopedTempDirTest, DestructorRemovesTree) {
  const auto dir_path = unique_path ("tree");
  const auto nested_file = dir_path / "nested" / "file.txt";

  {
    utils::scoped_temp_dir dir {dir_path};
    std::filesystem::create_directories (nested_file.parent_path ());
    std::ofstream {nested_file} << "data";
    ASSERT_TRUE (std::filesystem::exists (nested_file));
  }

  EXPECT_FALSE (std::filesystem::exists (dir_path));
}

TEST (ScopedTempDirTest, DuplicatePathThrows) {
  const auto dir_path = unique_path ("duplicate");
  utils::scoped_temp_dir first {dir_path};

  EXPECT_THROW (
      { utils::scoped_temp_dir second {dir_path}; },
      std::filesystem::filesystem_error);
}

TEST (ScopedTempDirTest, MoveConstructTransfersOwnership) {
  const auto dir_path = unique_path ("move_construct");
  const auto nested_file = dir_path / "keep.txt";

  utils::scoped_temp_dir first {dir_path};
  std::ofstream {nested_file} << "keep";
  const auto moved_path = first.path ();

  utils::scoped_temp_dir second {std::move (first)};
  EXPECT_EQ (second.path (), moved_path);
  EXPECT_TRUE (std::filesystem::exists (nested_file));

  // second's destructor removes the tree; first's destructor is a no-op.
}

TEST (ScopedTempDirTest, MoveAssignDeletesPreviousAndTransfers) {
  const auto first_path = unique_path ("move_assign_first");
  const auto second_path = unique_path ("move_assign_second");
  const auto second_file = second_path / "second.txt";

  utils::scoped_temp_dir first {first_path};
  utils::scoped_temp_dir second {second_path};
  std::ofstream {second_file} << "second";

  first = std::move (second);

  EXPECT_FALSE (std::filesystem::exists (first_path));
  EXPECT_EQ (first.path (), second_path);
  EXPECT_TRUE (std::filesystem::exists (second_file));
}

TEST (ScopedTempDirTest, ExceptionSafetyStillRemovesDirectory) {
  const auto dir_path = unique_path ("exception");

  try {
    utils::scoped_temp_dir dir {dir_path};
    throw std::runtime_error {"abort scope"};
  } catch (const std::runtime_error&) {
  }

  EXPECT_FALSE (std::filesystem::exists (dir_path));
}

#ifndef _WIN32
TEST (ScopedTempDirTest, PermissionsAreOwnerOnly) {
  const auto dir_path = unique_path ("permissions");

  utils::scoped_temp_dir dir {dir_path};
  const auto perms = std::filesystem::status (dir.path ()).permissions ();

  EXPECT_EQ (perms, std::filesystem::perms::owner_all);
}
#endif

}  // namespace
