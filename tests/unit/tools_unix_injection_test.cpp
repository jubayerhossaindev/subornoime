// POSIX-only CLI coverage tests that need a SQLite corruption/injection step

// (python3 + stdlib sqlite3). Registered only on Linux/macOS CI, mirroring the

// platform_adapter_test.cpp registration pattern. On Windows the same code

// paths are exercised by the portable tools suite and the dictionary unit

// tests (which hit the identical SQL failure families through the C++ API).

//

// The injection step runs a generated python script FILE (not a -c one-liner)

// so no shell quoting is involved in passing the SQL statements.

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#define SUBORNO_POPEN popen
#define SUBORNO_PCLOSE pclose

namespace {

std::string quote_path(const char* path) {
    return "\"" + std::string(path) + "\"";
}

const std::string import_tool = quote_path(SUBORNO_IMPORT_TOOL);

struct CommandResult {
    int exit_code = -1;
    bool exited_normally = false;
    std::string output;  // stdout + stderr
};

CommandResult run_command(const std::string& command) {
    CommandResult result;
    // Paths passed by the caller are pre-quoted; no extra outer quoting here.

    FILE* stream = SUBORNO_POPEN((command + " 2>&1").c_str(), "r");
    if (stream == nullptr) {
        return result;
    }
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), stream) != nullptr) {
        result.output += buffer;
    }
    const int status = SUBORNO_PCLOSE(stream);
    result.exited_normally = WIFEXITED(status);
    result.exit_code = result.exited_normally ? WEXITSTATUS(status) : -1;
    return result;
}

class ScratchDir {
  public:
    ScratchDir() {
        const auto base = std::filesystem::temp_directory_path();
        for (int attempt = 0; attempt < 64; ++attempt) {
            path_ = base
                    / ("suborno_tools_unix_test_" + std::to_string(::getpid()) + "_"
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

int injection_counter = 0;

std::filesystem::path seed_database(const ScratchDir& scratch, const std::string& tsv) {
    const auto wordlist = write_file(scratch, "words.tsv", tsv);
    const CommandResult imported =
        run_command(import_tool + " " + db_path(scratch) + " " + quoted(wordlist));
    EXPECT_EQ(imported.exit_code, 0) << imported.output;
    return wordlist;
}

// Writes the SQL harness as a python script file and executes it against the

// scratch database. Failing the harness step fails the test loudly: schema

// drift must never pass silently.

void run_python_sql(const ScratchDir& scratch, const std::string& python_body) {
    const std::filesystem::path script =
        write_file(scratch,
                   "inject_" + std::to_string(injection_counter++) + ".py",
                   "import sqlite3, sys\n"
                   "c = sqlite3.connect(sys.argv[1])\n"
                       + python_body + "\nc.commit()\nc.close()\n");
    const CommandResult result =
        run_command("python3 " + quoted(script) + " " + quoted(scratch.path_ / "dict.sqlite"));
    ASSERT_EQ(result.exit_code, 0) << "harness sqlite step failed:\n" << result.output;
}

}  // namespace

TEST(ToolsUnixImportTest, ImportRollsBackAndAbortsWhenStorageFailsMidway) {
    ScratchDir scratch;
    const auto wordlist = seed_database(scratch, "কলম\t100\nবই\t50\n");
    // Recreate the entries table with a CHECK constraint so the SECOND row of

    // the next import fails at insert time. (Dropping the table alone cannot

    // work: the Dictionary constructor heals a missing table on reopen.) The

    // CLI must roll back the whole bulk transaction — leaving no partial

    // import — and the rethrown exception terminates the process.

    run_python_sql(scratch,
                   "c.execute('DROP TABLE dictionary_entries')\n"
                   "c.execute(\"CREATE TABLE dictionary_entries ("
                   " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   " word TEXT NOT NULL UNIQUE,"
                   " normalized TEXT NOT NULL,"
                   " frequency INTEGER NOT NULL DEFAULT 1 CHECK (frequency < 100),"
                   " source TEXT NOT NULL DEFAULT 'system',"
                   " user_frequency INTEGER NOT NULL DEFAULT 0,"
                   " user_pinned INTEGER NOT NULL DEFAULT 0,"
                   " category TEXT NOT NULL DEFAULT '',"
                   " register TEXT NOT NULL DEFAULT '',"
                   " origin TEXT NOT NULL DEFAULT '',"
                   " created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)\")\n"
                   "c.execute('CREATE INDEX IF NOT EXISTS idx_dictionary_normalized"
                   " ON dictionary_entries(normalized)')");
    const auto second_list =
        write_file(scratch, "second.tsv", "কলম\t50\nবই\t200\n");  // বই violates the CHECK

    const CommandResult result = run_command(import_tool + " " + db_path(scratch) + " import "
                                             + quoted(second_list) + " --mode=merge");
    EXPECT_FALSE(result.exited_normally && result.exit_code == 0) << result.output;
    EXPECT_EQ(result.output.find("Imported"), std::string::npos) << result.output;
}

TEST(ToolsUnixValidateTest, ValidateDetectsInvalidWordsAndExitsOne) {
    ScratchDir scratch;
    seed_database(scratch, "কলম\n");
    // Inject a row the CLI itself can never produce: an invalid word surface.

    run_python_sql(scratch,
                   "c.execute(\"INSERT INTO dictionary_entries(word, normalized, frequency, source,"
                   " category, register, user_frequency)"
                   " VALUES('book','book',1,'system','','',0)\")");
    const CommandResult result = run_command(import_tool + " " + db_path(scratch) + " validate");
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 1) << result.output;
    EXPECT_NE(result.output.find("INVALID-WORD\tbook"), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("Validation problems: 1"), std::string::npos) << result.output;
}

TEST(ToolsUnixExportTest, ExportSkipsTransientLearnedRowsAndKeepsUsedOnes) {
    ScratchDir scratch;
    seed_database(scratch, "কলম\n");
    // A user row with user_frequency == 0 is a transient learned entry that

    // must be excluded from the exported word pack; a used row stays in.

    run_python_sql(scratch,
                   "c.execute(\"INSERT INTO dictionary_entries(word, normalized, frequency, source,"
                   " category, register, user_frequency)"
                   " VALUES('ছায়া','ছায়া',1,'user','', '', 0)\")\n"
                   "c.execute(\"INSERT INTO dictionary_entries(word, normalized, frequency, source,"
                   " category, register, user_frequency)"
                   " VALUES('রঙ','রঙ',1,'user','', '', 3)\")");
    const auto exported = scratch.path_ / "out.tsv";
    const CommandResult result =
        run_command(import_tool + " " + db_path(scratch) + " export " + quoted(exported));
    ASSERT_EQ(result.exit_code, 0) << result.output;
    std::ifstream check(exported);
    std::string content((std::istreambuf_iterator<char>(check)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content.find("ছায়া"), std::string::npos) << content;
    EXPECT_NE(content.find("রঙ"), std::string::npos) << content;
    EXPECT_NE(result.output.find("Exported 2 words"), std::string::npos) << result.output;
}

TEST(ToolsUnixValidateTest, ValidateDetectsInvalidNormalizedAndDuplicateGroups) {
    ScratchDir scratch;
    seed_database(scratch, "কলম\n");
    // A valid word carrying an over-long normalized key (INVALID-NORMALIZED)

    // plus two rows sharing one normalized key (DUPLICATE-NORMALIZED group).

    run_python_sql(scratch,
                   "c.execute(\"INSERT INTO dictionary_entries(word, normalized, frequency,"
                   " source, user_frequency)"
                   " VALUES('বই','" + std::string(120, 'x') + "',1,'system',0)\")\n"
                   "c.execute(\"INSERT INTO dictionary_entries(word, normalized, frequency,"
                   " source, user_frequency)"
                   " VALUES('কাম','dup',1,'system',0)\")\n"
                   "c.execute(\"INSERT INTO dictionary_entries(word, normalized, frequency,"
                   " source, user_frequency)"
                   " VALUES('কোম','dup',1,'system',0)\")");
    const CommandResult result = run_command(import_tool + " " + db_path(scratch) + " validate");
    ASSERT_TRUE(result.exited_normally) << result.output;
    EXPECT_EQ(result.exit_code, 1) << result.output;
    EXPECT_NE(result.output.find("INVALID-NORMALIZED\tবই"), std::string::npos) << result.output;
    EXPECT_NE(result.output.find("DUPLICATE-NORMALIZED\tdup\tকাম\tকোম"), std::string::npos)
        << result.output;
    EXPECT_NE(result.output.find("Validation problems: 2"), std::string::npos) << result.output;
}

TEST(ToolsUnixDedupeTest, DedupeReportsGroupsAndAppliesRemovalsKeepingUserRows) {
    ScratchDir scratch;
    seed_database(scratch, "কলম\n");
    // Build a duplicate-normalized group of three rows with distinct sources;

    // the middle one carries a user frequency that dedupe --apply must keep.

    run_python_sql(scratch,
                   "c.execute(\"INSERT INTO dictionary_entries(word, normalized, frequency, source,"
                   " category, register, user_frequency)"
                   " VALUES('কম','কলম',900,'system','','',0)\")\n"
                   "c.execute(\"INSERT INTO dictionary_entries(word, normalized, frequency, source,"
                   " category, register, user_frequency)"
                   " VALUES('কাম','কলম',100,'user','', '', 7)\")\n"
                   "c.execute(\"INSERT INTO dictionary_entries(word, normalized, frequency, source,"
                   " category, register, user_frequency)"
                   " VALUES('কোম','কলম',100,'system','', '', 0)\")");
    const CommandResult report = run_command(import_tool + " " + db_path(scratch) + " dedupe");
    ASSERT_TRUE(report.exited_normally) << report.output;
    EXPECT_EQ(report.exit_code, 0) << report.output;
    EXPECT_NE(report.output.find("DUPLICATE\tকলম"), std::string::npos) << report.output;
    EXPECT_NE(report.output.find("(pass --apply to remove the lower-frequency forms)"),
              std::string::npos)
        << report.output;

    const CommandResult applied =
        run_command(import_tool + " " + db_path(scratch) + " dedupe --apply");
    ASSERT_TRUE(applied.exited_normally) << applied.output;
    EXPECT_EQ(applied.exit_code, 0) << applied.output;
    EXPECT_NE(applied.output.find("KEEP-USER"), std::string::npos) << applied.output;
    EXPECT_NE(applied.output.find("removed 2"), std::string::npos) << applied.output;
}
