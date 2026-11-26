// End-to-end subprocess tests for the dictionary management CLI tools and

// the portable C-API examples.

//

// These are the only way to cover src/tools/*_main.cpp and examples/*.c:

// they are standalone executables whose logic lives behind main(). Every

// scenario asserts the real exit code and the real stdout/stderr content,

// exercising positive paths, negative paths and error handling of the CLI

// contract documented in dictionary_import_main.cpp.

//

// The binaries under test are handed in as string macros by CMake so the

// tests run against the exact executables built in this build tree (which

// carry the same instrumentation as the tests themselves).

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>

#define SUBORNO_POPEN popen
#define SUBORNO_PCLOSE pclose
#else
#include <process.h>

#define SUBORNO_POPEN _popen
#define SUBORNO_PCLOSE _pclose
#endif

namespace {

std::string quote_path(const char* path) {
    return "\"" + std::string(path) + "\"";
}

const std::string import_tool = quote_path(SUBORNO_IMPORT_TOOL);
const std::string bench_tool = quote_path(SUBORNO_BENCH_TOOL);
const std::string engine_demo = quote_path(SUBORNO_ENGINE_DEMO);
const std::string capi_example = quote_path(SUBORNO_C_API_EXAMPLE);

#ifndef _WIN32
int current_process_id() {
    return ::getpid();
}
#else
int current_process_id() {
    return _getpid();
}
#endif

struct CommandResult {
    int exit_code = -1;
    bool exited_normally = false;
    std::string output;  // stdout + stderr
};

CommandResult run_command(const std::string& command) {
    CommandResult result;
    // Paths passed by the caller are pre-quoted.

#ifdef _WIN32
    // _popen spawns `cmd.exe /c <command>`. With more than one quoted token

    // on the line, cmd.exe applies its legacy parsing and strips the first

    // AND the last quote character, turning `"tool" "arg1" "arg2" 2>&1` into

    // `tool" "arg1" "arg2 2>&1`, which fails with "The system cannot find

    // the path specified." (observed on the real MSVC machine, 2026-11-25:

    // every Tools*Test with arguments failed while the no-argument case

    // passed). Wrapping the whole command in one extra outer quote pair

    // gives cmd.exe exactly one pair to strip and preserves every token and

    // the redirection; /bin/sh needs no such wrapping on POSIX.

    FILE* stream = SUBORNO_POPEN(("cmd /c \"" + command + " 2>&1\"").c_str(), "r");
#else
    FILE* stream = SUBORNO_POPEN((command + " 2>&1").c_str(), "r");
#endif
    if (stream == nullptr) {
        return result;
    }
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), stream) != nullptr) {
        result.output += buffer;
    }
    const int status = SUBORNO_PCLOSE(stream);
#ifndef _WIN32
    result.exited_normally = WIFEXITED(status);
    result.exit_code = result.exited_normally ? WEXITSTATUS(status) : -1;
#else
    result.exited_normally = status >= 0;
    result.exit_code = status;
#endif
    return result;
}

// Unique per-test scratch directory; removed when the guard goes out of scope.

class ScratchDir {
  public:
    ScratchDir() {
        const auto base = std::filesystem::temp_directory_path();
        for (int attempt = 0; attempt < 64; ++attempt) {
            path_ = base
                    / ("suborno_tools_test_" + std::to_string(current_process_id()) + "_"
                       + std::to_string(attempt) + "_"
                       + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
            if (std::filesystem::create_directories(path_)) {
                break;
            }
        }
    }
    ~ScratchDir() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }
    ScratchDir(const ScratchDir&) = delete;
    ScratchDir& operator=(const ScratchDir&) = delete;

    std::filesystem::path path_;
};

std::filesystem::path write_file(const ScratchDir& scratch,
                                 const std::string& name,
                                 const std::string& content) {
    const std::filesystem::path path = scratch.path_ / name;
    std::ofstream stream(path);
    stream << content;
    return path;
}

std::string quoted(const std::filesystem::path& path) {
    return "\"" + path.string() + "\"";
}

std::string db_path(const ScratchDir& scratch) {
    return quoted(scratch.path_ / "dict.sqlite");
}

// Deterministic seed pack used by most scenarios: three valid Bangla words

// (one with full metadata), one comment line, one blank line, one invalid

// (ASCII) word and one invalid-frequency line.

constexpr const char* seed_tsv =
    "# word\tfrequency\tcategory\tregister\n"
    "\n"
    "কলম\t100\tnoun\tstandard\n"
    "বই\t50\n"
    "আমি\n"
    "book\t10\tnoun\tstandard\n"
    "চেয়ার\tnot-a-number\n";

}  // namespace

TEST(ToolsImportTest, UsageWithoutArgumentsExitsTwo) {
    const CommandResult result = run_command(import_tool);
    ASSERT_TRUE(result.exited_normally);
    EXPECT_EQ(result.exit_code, 2);
    EXPECT_NE(result.output.find("Usage: suborno_dictionary_import"), std::string::npos);
    EXPECT_NE(
        result.output.find("Modes: import|export|relations|validate|stats|dedupe|backup|vacuum"),
        std::string::npos);
}

TEST(ToolsImportTest, LegacyTsvArgumentImportsInReplaceMode) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.tsv", seed_tsv);
    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist));
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 0) << result.output;
    EXPECT_NE(result.output.find(
                  "Imported 3 words (mode=replace, origin=seed); rejected 2 invalid lines."),
              std::string::npos)
        << result.output;
    // Invalid lines are echoed on stderr with their content.

    EXPECT_NE(result.output.find("Invalid word rejected at line: book"), std::string::npos);
    EXPECT_NE(result.output.find("Invalid word rejected at line: চেয়ার"), std::string::npos);
}

TEST(ToolsImportTest, LegacyTxtExtensionAlsoImports) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.txt", "কলম\n");
    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist));
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 0) << result.output;
    EXPECT_NE(result.output.find("Imported 1 words"), std::string::npos) << result.output;
}

TEST(ToolsImportTest, ImportModeSupportsMergeAndCustomOrigin) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.tsv", "কলম\t100\nবই\t50\n");
    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " import " + quoted(wordlist)
                    + " --mode=merge --origin=vendor-pack");
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 0) << result.output;
    EXPECT_NE(result.output.find(
                  "Imported 2 words (mode=merge, origin=vendor-pack); rejected 0 invalid lines."),
              std::string::npos)
        << result.output;
}

TEST(ToolsImportTest, ImportRejectsUnknownFlags) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.tsv", "কলম\n");
    const CommandResult result = run_command(import_tool + " " + db_path(scratch) + " import "
                                             + quoted(wordlist) + " --bogus=1");
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 2) << result.output;
    EXPECT_NE(result.output.find("Unknown flag: --bogus=1"), std::string::npos) << result.output;
}

TEST(ToolsImportTest, ImportReportsMissingWordlist) {
    ScratchDir scratch;
    const CommandResult result = run_command(import_tool + " " + db_path(scratch) + " import "
                                             + quoted(scratch.path_ / "missing.tsv"));
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 2) << result.output;
    EXPECT_NE(result.output.find("Cannot open wordlist:"), std::string::npos) << result.output;
}

TEST(ToolsImportTest, ImportAllInvalidLinesExitsTwo) {
    ScratchDir scratch;
    const auto wordlist =
        write_file(scratch, "words.tsv", "book\tpen\n# only comments and junk\n\n");
    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist));
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 2) << result.output;
    EXPECT_NE(result.output.find("Imported 0 words"), std::string::npos) << result.output;
}

TEST(ToolsImportTest, ImportClampsHugeFrequenciesAndRejectsGarbage) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch,
                                     "words.tsv",
                                     "কলম\t99999999999\n"  // > 0x7FFFFFFF -> clamp

                                     "বই\t2147483647\n");  // exact boundary

    const auto exported = scratch.path_ / "out.tsv";
    ASSERT_EQ(run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist)).exit_code,
              0);
    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " export " + quoted(exported));
    ASSERT_EQ(result.exit_code, 0) << result.output;
    std::ifstream check(exported);
    std::string content((std::istreambuf_iterator<char>(check)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("কলম\t2147483647"), std::string::npos) << content;
    EXPECT_NE(content.find("বই\t2147483647"), std::string::npos) << content;
}

TEST(ToolsImportTest, ImportEchoesAtMostTwentyInvalidLines) {
    ScratchDir scratch;
    std::string wordlist;
    for (int index = 0; index < 25; ++index) {
        wordlist += "junk" + std::to_string(index) + "\n";
    }
    const auto path = write_file(scratch, "words.tsv", wordlist);
    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " " + quoted(path));
    ASSERT_EQ(result.exit_code, 2) << result.output;
    std::size_t echoes = 0;
    std::size_t position = 0;
    while ((position = result.output.find("Invalid word rejected at line:", position))
           != std::string::npos) {
        ++echoes;
        ++position;
    }
    // The CLI echoes the first 20 rejections and stays quiet afterwards.

    EXPECT_EQ(echoes, 20U) << result.output;
}

TEST(ToolsExportTest, ExportEmptyDatabaseExitsTwo) {
    ScratchDir scratch;
    const auto exported = scratch.path_ / "out.tsv";
    ASSERT_EQ(run_command(import_tool + " " + db_path(scratch) + " vacuum").exit_code, 0);
    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " export " + quoted(exported));
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 2) << result.output;
    EXPECT_NE(result.output.find("Exported 0 words"), std::string::npos) << result.output;
}

TEST(ToolsExportTest, ExportWritesHeaderAndAllRows) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.tsv", seed_tsv);
    ASSERT_EQ(run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist)).exit_code,
              0);
    const auto exported = scratch.path_ / "out.tsv";
    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " export " + quoted(exported));
    ASSERT_EQ(result.exit_code, 0) << result.output;
    EXPECT_NE(result.output.find("Exported 3 words to"), std::string::npos) << result.output;
    std::ifstream check(exported);
    std::string content((std::istreambuf_iterator<char>(check)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("# word\tfrequency\tcategory\tregister\n"), std::string::npos);
    EXPECT_NE(content.find("কলম\t100\tnoun\tstandard\n"), std::string::npos) << content;
    EXPECT_NE(content.find("বই\t50\t\t\n"), std::string::npos) << content;
    EXPECT_NE(content.find("আমি\t1\t\t\n"), std::string::npos) << content;
}

TEST(ToolsExportTest, ExportRejectsUnwritableOutputPath) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.tsv", "কলম\n");
    ASSERT_EQ(run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist)).exit_code,
              0);
    // A directory cannot be opened as an ofstream -> "Cannot open output".

    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " export " + quoted(scratch.path_));
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 2) << result.output;
    EXPECT_NE(result.output.find("Cannot open output:"), std::string::npos) << result.output;
}

TEST(ToolsRelationsTest, RelationsAddsValidAndRejectsShortAndInvalidLines) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.tsv", "কলম\nবই\n");
    ASSERT_EQ(run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist)).exit_code,
              0);
    const auto relations = write_file(scratch,
                                      "relations.tsv",
                                      "কলম\tsynonym\tলেখনী\n"  // added

                                      "বই\tantonym\tসিনেমা\n"  // added

                                      "only-two-fields\n"  // rejected: < 3 fields

                                      "abc\tsynonym\tকলম\n"  // rejected: invalid word

                                      "# comment\n");
    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " relations " + quoted(relations));
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 0) << result.output;
    EXPECT_NE(result.output.find("Added 2 word relations; rejected 2."), std::string::npos)
        << result.output;
    EXPECT_NE(result.output.find("Rejected relation: abc\tsynonym\tকলম"), std::string::npos)
        << result.output;
}

TEST(ToolsRelationsTest, RelationsReportsMissingFileAndAllRejectedCases) {
    ScratchDir scratch;
    const CommandResult missing = run_command(import_tool + " " + db_path(scratch) + " relations "
                                              + quoted(scratch.path_ / "missing.tsv"));
    ASSERT_EQ(missing.exit_code, 2) << missing.output;
    EXPECT_NE(missing.output.find("Cannot open relations file:"), std::string::npos)
        << missing.output;

    const auto relations = write_file(scratch, "relations.tsv", "abc\tsynonym\tকলম\n");
    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " relations " + quoted(relations));
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 2) << result.output;  // added == 0

    EXPECT_NE(result.output.find("Added 0 word relations; rejected 1."), std::string::npos)
        << result.output;
}

TEST(ToolsValidateTest, ValidateCleanDatabaseExitsZero) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.tsv", "কলম\nবই\nআমি\n");
    ASSERT_EQ(run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist)).exit_code,
              0);
    const CommandResult result = run_command(import_tool + " " + db_path(scratch) + " validate");
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 0) << result.output;
    EXPECT_NE(result.output.find("Validation clean (3 entries checked)."), std::string::npos)
        << result.output;
}

TEST(ToolsStatsTest, StatsPrintsSchemaCountsCategoriesAndTiers) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.tsv", seed_tsv);
    ASSERT_EQ(run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist)).exit_code,
              0);
    const CommandResult result = run_command(import_tool + " " + db_path(scratch) + " stats");
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 0) << result.output;
    EXPECT_NE(result.output.find("total_entries=3"), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("system_entries=3"), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("user_entries=0"), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("category\tnoun\t1"), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("schema_version="), std::string::npos) << result.output;
}

TEST(ToolsBackupVacuumTest, BackupWritesAndVacuumCompacts) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.tsv", "কলম\nবই\n");
    ASSERT_EQ(run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist)).exit_code,
              0);
    const auto backup = scratch.path_ / "backup.sqlite";
    const CommandResult backed =
        run_command(import_tool + " " + db_path(scratch) + " backup " + quoted(backup));
    ASSERT_TRUE(backed.exited_normally) << backed.output;
    EXPECT_EQ(backed.exit_code, 0) << backed.output;
    EXPECT_NE(backed.output.find("Backup written to"), std::string::npos) << backed.output;
    EXPECT_TRUE(std::filesystem::exists(backup)) << "backup file must exist";
    EXPECT_GT(std::filesystem::file_size(backup), 0U) << "backup file must not be empty";

    const CommandResult vacuumed = run_command(import_tool + " " + db_path(scratch) + " vacuum");
    ASSERT_TRUE(vacuumed.exited_normally) << vacuumed.output;
    EXPECT_EQ(vacuumed.exit_code, 0) << vacuumed.output;
    EXPECT_NE(vacuumed.output.find("Vacuum complete."), std::string::npos) << vacuumed.output;
}

TEST(ToolsBackupVacuumTest, BackupReportsUnusableDestination) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.tsv", "কলম\n");
    ASSERT_EQ(run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist)).exit_code,
              0);
    // A directory destination makes sqlite's backup API fail deterministically.

    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " backup " + quoted(scratch.path_));
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 1) << result.output;
    EXPECT_NE(result.output.find("Error:"), std::string::npos) << result.output;
}

TEST(ToolsArgumentParsingTest, UnknownModeAndMissingArgumentsExitTwo) {
    ScratchDir scratch;
    const CommandResult unknown =
        run_command(import_tool + " " + db_path(scratch) + " frobnicate x.tsv");
    EXPECT_EQ(unknown.exit_code, 2) << unknown.output;
    EXPECT_NE(unknown.output.find("Unknown mode: frobnicate"), std::string::npos) << unknown.output;

    const CommandResult no_arg = run_command(import_tool + " " + db_path(scratch) + " export");
    EXPECT_EQ(no_arg.exit_code, 2) << no_arg.output;
    EXPECT_NE(no_arg.output.find("Mode 'export' requires an argument."), std::string::npos)
        << no_arg.output;

    const CommandResult import_no_arg =
        run_command(import_tool + " " + db_path(scratch) + " import");
    EXPECT_EQ(import_no_arg.exit_code, 2) << import_no_arg.output;
}

TEST(ToolsBenchTest, BenchRunsSmallWorkloadAndPrintsTimings) {
    ScratchDir scratch;
    const CommandResult result = run_command(std::string(bench_tool) + " "
                                             + quoted(scratch.path_ / "bench.sqlite") + " 100 10");
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 0) << result.output;
    EXPECT_NE(result.output.find("words=100"), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("exact_lookup_avg_us="), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("prefix_lookup_avg_us="), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("cached_repeat_us="), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("hits=10/10"), std::string::npos) << result.output;
}

TEST(ToolsBenchTest, BenchRejectsMissingDatabaseArgument) {
    const CommandResult result = run_command(bench_tool);
    ASSERT_TRUE(result.exited_normally);
    EXPECT_EQ(result.exit_code, 2);
    EXPECT_NE(result.output.find("Usage: suborno_dictionary_bench"), std::string::npos)
        << result.output;
}

TEST(ToolsImportTest, ImportRejectsWhitespaceOnlyWordLines) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.tsv", "\t\t\n   \n  বই  \t50\nকলম\t100\n");
    // Whitespace-only first fields trim to empty words and are rejected;

    // a space-padded word trims to a valid entry and imports.

    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist));
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 0) << result.output;
    EXPECT_NE(result.output.find("Imported 2 words"), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("rejected 2 invalid lines"), std::string::npos) << result.output;
}

TEST(ToolsArgumentParsingTest, UnknownModeWithoutArgumentExitsTwo) {
    ScratchDir scratch;
    const CommandResult result = run_command(import_tool + " " + db_path(scratch) + " frobnicate");
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 2) << result.output;
    EXPECT_NE(result.output.find("Unknown mode: frobnicate"), std::string::npos) << result.output;
}

TEST(ToolsDedupeTest, BareDedupeRunsTheReportWithoutArguments) {
    ScratchDir scratch;
    const auto wordlist = write_file(scratch, "words.tsv", "কলম\nবই\n");
    ASSERT_EQ(run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist)).exit_code,
              0);
    // dedupe's --apply is optional, so bare `dedupe` must run the report.

    const CommandResult report = run_command(import_tool + " " + db_path(scratch) + " dedupe");
    ASSERT_TRUE(report.exited_normally) << report.output;
    EXPECT_EQ(report.exit_code, 0) << report.output;
    EXPECT_NE(report.output.find("Duplicate groups: 0"), std::string::npos) << report.output;
    EXPECT_NE(report.output.find("(pass --apply to remove the lower-frequency forms)"),
              std::string::npos)
        << report.output;
}

TEST(ToolsBenchTest, BenchRejectsZeroWordCount) {
    ScratchDir scratch;
    // rng() % word_count would divide by zero; the CLI must reject it cleanly.

    const CommandResult result = run_command(std::string(bench_tool) + " "
                                             + quoted(scratch.path_ / "bench.sqlite") + " 0 10");
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 2) << result.output;
    EXPECT_NE(result.output.find("word_count must be greater than zero."), std::string::npos)
        << result.output;
}

TEST(ToolsExamplesTest, EngineDemoPrintsCompositionCandidatesAndCommit) {
    const CommandResult result = run_command(engine_demo);
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 0) << result.output;
    EXPECT_NE(result.output.find("composition: আমি"), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("candidate(s):"), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("committed: আমি"), std::string::npos) << result.output;
}

TEST(ToolsExamplesTest, EngineDemoAcceptsCustomRomanInput) {
    const CommandResult result = run_command(std::string(engine_demo) + " bhai");
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 0) << result.output;
    // The phonetic engine ranks ভৈ as the top candidate for "bhai"; ভাই must

    // still be offered in the candidate list.

    EXPECT_NE(result.output.find("composition: ভৈ"), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("ভাই"), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("committed: ভৈ"), std::string::npos) << result.output;
}

TEST(ToolsExamplesTest, CApiExamplePrintsComposition) {
    const CommandResult result = run_command(capi_example);
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 0) << result.output;
    EXPECT_NE(result.output.find("composition: আমি"), std::string::npos) << result.output;
}
