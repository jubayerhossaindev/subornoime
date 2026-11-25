#include <cstddef>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <string>
#include <system_error>

#include "persistence/atomic_file.hpp"

namespace {

std::string read_file_contents(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string{std::istreambuf_iterator<char>(input), {}};
}

std::size_t count_temporary_files(const std::filesystem::path& directory) {
    std::size_t count = 0;
    std::error_code probe_error;
    for (const auto& entry : std::filesystem::directory_iterator(directory, probe_error)) {
        if (entry.path().filename().string().find(".tmp.") != std::string::npos) {
            ++count;
        }
    }
    return count;
}

}  // namespace

// Covers make_temporary_path (26-38) plus the happy paths of
// write_temporary_file (111-148) and replace_destination (55-72): exact
// binary contents — including an embedded NUL — land at the destination and
// no .tmp.* litter survives in the directory.
TEST(PersistenceCoverage, WriteAtomicallyWritesExactBinaryContentsWithoutTempLitter) {
    const auto directory = std::filesystem::temp_directory_path() / "suborno_cov_atomic_binary_dir";
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
    std::filesystem::create_directories(directory);
    const auto destination = directory / "data.bin";

    const std::string contents = std::string("bin") + '\0' + "ary" + '\x01' + '\xff' + " tail";
    ASSERT_FALSE(suborno::persistence::write_file_atomically(destination, contents));
    EXPECT_EQ(read_file_contents(destination), contents);
    EXPECT_EQ(count_temporary_files(directory), std::size_t{0});

    std::filesystem::remove_all(directory, ignored);
}

// Covers the rename-over-replace path of replace_destination (55-59): an
// existing destination with different-length content is fully replaced and
// no temporary files are left behind.
TEST(PersistenceCoverage, WriteAtomicallyOverwritesDestinationWithDifferentLength) {
    const auto directory =
        std::filesystem::temp_directory_path() / "suborno_cov_atomic_overwrite_dir";
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
    std::filesystem::create_directories(directory);
    const auto destination = directory / "data.txt";

    ASSERT_FALSE(suborno::persistence::write_file_atomically(destination, "first-contents-longer"));
    ASSERT_FALSE(suborno::persistence::write_file_atomically(destination, "second"));
    EXPECT_EQ(read_file_contents(destination), "second");
    EXPECT_EQ(count_temporary_files(directory), std::size_t{0});

    std::filesystem::remove_all(directory, ignored);
}

// Covers the empty-destination guard (199-201) with the exact
// std::errc::invalid_argument error code.
TEST(PersistenceCoverage, WriteAtomicallyRejectsEmptyDestination) {
    const std::error_code error =
        suborno::persistence::write_file_atomically(std::filesystem::path{}, "data");
    EXPECT_EQ(error, std::make_error_code(std::errc::invalid_argument));
}

// Covers the O_CREAT|O_EXCL open failure of write_temporary_file (111-115)
// and the early write_error return of write_file_atomically (204-206): the
// destination directory does not exist, so nothing is created anywhere.
TEST(PersistenceCoverage, WriteAtomicallyReportsErrorWhenDestinationDirectoryIsMissing) {
    const auto missing_directory =
        std::filesystem::temp_directory_path() / "suborno_cov_atomic_missing_dir_xyz";
    std::error_code ignored;
    std::filesystem::remove_all(missing_directory, ignored);
    const auto destination = missing_directory / "data.bin";

    const std::error_code error = suborno::persistence::write_file_atomically(destination, "x");
    EXPECT_NE(error, std::error_code{});
#if !defined(_WIN32)
    EXPECT_EQ(error, std::make_error_code(std::errc::no_such_file_or_directory));
#endif
    EXPECT_FALSE(std::filesystem::exists(destination));
    EXPECT_FALSE(std::filesystem::exists(missing_directory));

    std::filesystem::remove_all(missing_directory, ignored);
}

// Covers the replace failure and temporary cleanup of write_file_atomically
// (208-213): the destination is an existing directory, so the rename fails
// (EISDIR on POSIX), the error is returned, the directory stays untouched
// and the temporary file is removed.
TEST(PersistenceCoverage, WriteAtomicallyFailsAndCleansUpWhenDestinationIsADirectory) {
    const auto parent = std::filesystem::temp_directory_path() / "suborno_cov_atomic_replace_dir";
    std::error_code ignored;
    std::filesystem::remove_all(parent, ignored);
    std::filesystem::create_directories(parent);
    const auto destination = parent / "occupied";
    std::filesystem::create_directories(destination);

    const std::error_code error = suborno::persistence::write_file_atomically(destination, "x");
    EXPECT_NE(error, std::error_code{});
#if !defined(_WIN32)
    EXPECT_EQ(error, std::make_error_code(std::errc::is_a_directory));
#endif
    EXPECT_TRUE(std::filesystem::is_directory(destination));
    std::size_t entry_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(parent)) {
        ++entry_count;
        EXPECT_EQ(entry.path().filename(), "occupied");
    }
    EXPECT_EQ(entry_count, std::size_t{1});
    EXPECT_EQ(count_temporary_files(parent), std::size_t{0});

    std::filesystem::remove_all(parent, ignored);
}

// Covers the directory-fsync skip in replace_destination (62-71): a bare
// relative destination has an empty parent path, so opening the parent
// directory fails and the write still reports success.
TEST(PersistenceCoverage, WriteAtomicallySucceedsWithRelativeDestinationWithoutParent) {
    const std::filesystem::path relative("suborno_cov_atomic_relative.bin");
    const auto absolute = std::filesystem::current_path() / relative;
    std::error_code ignored;
    std::filesystem::remove(absolute, ignored);

    ASSERT_FALSE(suborno::persistence::write_file_atomically(relative, "relative payload"));
    EXPECT_EQ(read_file_contents(absolute), "relative payload");

    std::filesystem::remove(absolute, ignored);
}

// Covers the atomic sequence counter inside make_temporary_path (27, 36):
// two back-to-back writes to one destination must both succeed — the O_EXCL
// temp creation never collides — and leave the second write's contents.
TEST(PersistenceCoverage, SequentialWritesToSameDestinationNeverCollide) {
    const auto directory =
        std::filesystem::temp_directory_path() / "suborno_cov_atomic_sequence_dir";
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
    std::filesystem::create_directories(directory);
    const auto destination = directory / "data.txt";

    ASSERT_FALSE(suborno::persistence::write_file_atomically(destination, "first"));
    ASSERT_FALSE(suborno::persistence::write_file_atomically(destination, "second"));
    EXPECT_EQ(read_file_contents(destination), "second");
    EXPECT_EQ(count_temporary_files(directory), std::size_t{0});

    std::filesystem::remove_all(directory, ignored);
}

// Covers quarantine_file's happy path (153-180): an existing file is
// renamed beside itself under "<name>.corrupt-...", its exact bytes —
// including an embedded NUL and CRLF — are preserved and the original is
// gone.
TEST(PersistenceCoverage, QuarantineRenamesExistingFileAndPreservesExactBytes) {
    const auto directory =
        std::filesystem::temp_directory_path() / "suborno_cov_atomic_quarantine_dir";
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
    std::filesystem::create_directories(directory);
    const auto path = directory / "settings.ini";

    const std::string contents = std::string("corrupt") + '\0' + "but\r\nprecious";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << contents;
    }

    EXPECT_EQ(suborno::persistence::quarantine_file(path), std::error_code{});
    EXPECT_FALSE(std::filesystem::exists(path));

    bool quarantined_found = false;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        const std::string name = entry.path().filename().string();
        if (name.rfind("settings.ini.corrupt-", 0) == 0) {
            EXPECT_FALSE(quarantined_found);
            quarantined_found = true;
            EXPECT_EQ(read_file_contents(entry.path()), contents);
        }
    }
    EXPECT_TRUE(quarantined_found);

    std::filesystem::remove_all(directory, ignored);
}

// Covers the nothing-to-quarantine outcome (157-161): a missing file
// returns a clear error_code, both with an existing parent and with a
// parent directory that does not exist.
TEST(PersistenceCoverage, QuarantineMissingFileSucceedsEvenInMissingDirectory) {
    const auto path =
        std::filesystem::temp_directory_path() / "suborno_cov_atomic_quarantine_missing.bin";
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    EXPECT_EQ(suborno::persistence::quarantine_file(path), std::error_code{});

    const auto nested = path.parent_path() / "suborno_cov_atomic_no_such_dir" / "gone.bin";
    EXPECT_EQ(suborno::persistence::quarantine_file(nested), std::error_code{});
}

// Covers the empty-path guard (154-156) with the exact
// std::errc::invalid_argument error code.
TEST(PersistenceCoverage, QuarantineEmptyPathReportsInvalidArgument) {
    EXPECT_EQ(suborno::persistence::quarantine_file(std::filesystem::path{}),
              std::make_error_code(std::errc::invalid_argument));
}
