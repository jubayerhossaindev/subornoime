#include "persistence/atomic_file.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <string>
#include <system_error>

TEST(AtomicFileTest, ReplacesExistingFileWithCompleteContents) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_atomic_file_test.txt";
    std::error_code ignored;
    std::filesystem::remove(path, ignored);

    ASSERT_FALSE(suborno::persistence::write_file_atomically(path, "old"));
    ASSERT_FALSE(suborno::persistence::write_file_atomically(path, "new contents"));

    std::ifstream input(path, std::ios::binary);
    ASSERT_TRUE(input.is_open());
    const std::string contents{std::istreambuf_iterator<char>(input), {}};
    EXPECT_EQ(contents, "new contents");

    input.close();
    std::filesystem::remove(path, ignored);
}

TEST(AtomicFileTest, RejectsAnEmptyDestination) {
    const std::error_code error =
        suborno::persistence::write_file_atomically(std::filesystem::path{}, "data");
    EXPECT_EQ(error, std::make_error_code(std::errc::invalid_argument));
}

TEST(AtomicFileTest, FailedReplacementLeavesDestinationIntact) {
    // A destination occupied by a directory makes the final replacement fail
    // on every platform. The contract: report the error, leave the directory
    // untouched, and clean up the temporary file — no corruption, no litter.
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "suborno_atomic_replace_dir_test";
    std::filesystem::remove_all(directory);
    const std::filesystem::path destination = directory / "data";
    std::filesystem::create_directories(destination);

    const std::error_code error = suborno::persistence::write_file_atomically(destination, "x");
    EXPECT_NE(error, std::error_code{});
    EXPECT_TRUE(std::filesystem::is_directory(destination));
    // The failed replacement must leave the destination untouched and clean
    // up the temporary file: the only entry left in the parent is the
    // destination directory itself.
    std::size_t sibling_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        ++sibling_count;
        EXPECT_EQ(entry.path().filename(), "data");
        EXPECT_TRUE(entry.is_directory());
    }
    EXPECT_EQ(sibling_count, 1U);
    std::filesystem::remove_all(directory);
}

TEST(AtomicFileTest, QuarantineRenamesExistingFileBesideItself) {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "suborno_atomic_quarantine_test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    const std::filesystem::path path = directory / "settings.ini";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "corrupt-but-precious";
    }

    EXPECT_EQ(suborno::persistence::quarantine_file(path), std::error_code{});
    EXPECT_FALSE(std::filesystem::exists(path));

    bool quarantined_found = false;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().filename().string().rfind("settings.ini.corrupt-", 0) == 0) {
            EXPECT_FALSE(quarantined_found);
            quarantined_found = true;
            std::ifstream input(entry.path(), std::ios::binary);
            const std::string contents{std::istreambuf_iterator<char>(input), {}};
            EXPECT_EQ(contents, "corrupt-but-precious");
        }
    }
    EXPECT_TRUE(quarantined_found);
    std::filesystem::remove_all(directory);
}

TEST(AtomicFileTest, QuarantineMissingFileSucceeds) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_atomic_quarantine_missing.bin";
    std::filesystem::remove(path);
    EXPECT_EQ(suborno::persistence::quarantine_file(path), std::error_code{});
}

TEST(AtomicFileTest, QuarantineEmptyPathIsInvalid) {
    EXPECT_EQ(suborno::persistence::quarantine_file(std::filesystem::path{}),
              std::make_error_code(std::errc::invalid_argument));
}
