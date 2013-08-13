// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Authors: keith.ray@gmail.com (Keith Ray)
//
// Google Test filepath utilities
//
// This file tests classes and functions used internally by
// Google Test.  They are subject to change without notice.
//
// This file is #included from gtest_unittest.cc, to avoid changing
// build or make-files for some existing Google Test clients. Do not
// #include this file anywhere else!

#include "gtest/internal/gtest-filepath.h"
#include "gtest/gtest.h"

// Indicates that this translation unit is part of Google Test's
// implementation.  It must come before gtest-internal-inl.h is
// included, or there will be a compiler error.  This trick is to
// prevent a user from accidentally including gtest-internal-inl.h in
// his code.
#define GTEST_IMPLEMENTATION_ 1
#include "src/gtest-internal-inl.h"
#undef GTEST_IMPLEMENTATION_

#if GTEST_OS_WINDOWS_MOBILE
# include <windows.h>  // NOLINT
#elif GTEST_OS_WINDOWS
# include <direct.h>  // NOLINT
#endif  // GTEST_OS_WINDOWS_MOBILE

namespace testing {
namespace internal {
namespace {

#if GTEST_OS_WINDOWS_MOBILE
// TODO(wan@google.com): Move these to the POSIX adapter section in
// gtest-port.h.

// Windows CE doesn't have the remove C function.
int remove(const char* path) {
  LPCWSTR wpath = String::AnsiToUtf16(path);
  int ret = DeleteFile(wpath) ? 0 : -1;
  delete [] wpath;
  return ret;
}
// Windows CE doesn't have the _rmdir C function.
int _rmdir(const char* path) {
  FilePath filepath(path);
  LPCWSTR wpath = String::AnsiToUtf16(
      filepath.RemoveTrailingPathSeparator().c_str());
  int ret = RemoveDirectory(wpath) ? 0 : -1;
  delete [] wpath;
  return ret;
}

#else

TEST(GetCurrentDirTest, ReturnsCurrentDir) {
  const FilePath original_dir = FilePath::GetCurrentDir();
  EXPECT_FALSE(original_dir.IsEmpty());

  posix::ChDir(GTEST_PATH_SEP_);
  const FilePath cwd = FilePath::GetCurrentDir();
  posix::ChDir(original_dir.c_str());

# if GTEST_OS_WINDOWS

  // Skips the ":".
  const char* const cwd_without_drive = strchr(cwd.c_str(), ':');
  ASSERT_TRUE(cwd_without_drive != NULL);
  EXPECT_STREQ(GTEST_PATH_SEP_, cwd_without_drive + 1);

# else

  EXPECT_STREQ(GTEST_PATH_SEP_, cwd.c_str());

# endif
}

#endif  // GTEST_OS_WINDOWS_MOBILE

TEST(IsEmptyTest, ReturnsTrueForEmptyPath) {
  EXPECT_TRUE(FilePath("").IsEmpty());
  EXPECT_TRUE(FilePath(NULL).IsEmpty());
}

TEST(IsEmptyTest, ReturnsFalseForNonEmptyPath) {
  EXPECT_FALSE(FilePath("a").IsEmpty());
  EXPECT_FALSE(FilePath(".").IsEmpty());
  EXPECT_FALSE(FilePath("a/b").IsEmpty());
  EXPECT_FALSE(FilePath("a\\b\\").IsEmpty());
}

// RemoveDirectoryName "" -> ""
TEST(RemoveDirectoryNameTest, WhenEmptyName) {
  EXPECT_STREQ("", FilePath("").RemoveDirectoryName().c_str());
}

// RemoveDirectoryName "afile" -> "afile"
TEST(RemoveDirectoryNameTest, ButNoDirectory) {
  EXPECT_STREQ("afile",
      FilePath("afile").RemoveDirectoryName().c_str());
}

// RemoveDirectoryName "/afile" -> "afile"
TEST(RemoveDirectoryNameTest, RootFileShouldGiveFileName) {
  EXPECT_STREQ("afile",
      FilePath(GTEST_PATH_SEP_ "afile").RemoveDirectoryName().c_str());
}

// RemoveDirectoryName "adir/" -> ""
TEST(RemoveDirectoryNameTest, WhereThereIsNoFileName) {
  EXPECT_STREQ("",
      FilePath("adir" GTEST_PATH_SEP_).RemoveDirectoryName().c_str());
}

// RemoveDirectoryName "adir/afile" -> "afile"
TEST(RemoveDirectoryNameTest, ShouldGiveFileName) {
  EXPECT_STREQ("afile",
      FilePath("adir" GTEST_PATH_SEP_ "afile").RemoveDirectoryName().c_str());
}

// RemoveDirectoryName "adir/subdir/afile" -> "afile"
TEST(RemoveDirectoryNameTest, ShouldAlsoGiveFileName) {
  EXPECT_STREQ("afile",
      FilePath("adir" GTEST_PATH_SEP_ "subdir" GTEST_PATH_SEP_ "afile")
      .RemoveDirectoryName().c_str());
}

#if GTEST_HAS_ALT_PATH_SEP_

// Tests that RemoveDirectoryName() works with the alternate separator
// on Windows.

// RemoveDirectoryName("/afile") -> "afile"
TEST(RemoveDirectoryNameTest, RootFileShouldGiveFileNameForAlternateSeparator) {
  EXPECT_STREQ("afile",
               FilePath("/afile").RemoveDirectoryName().c_str());
}

// RemoveDirectoryName("adir/") -> ""
TEST(RemoveDirectoryNameTest, WhereThereIsNoFileNameForAlternateSeparator) {
  EXPECT_STREQ("",
               FilePath("adir/").RemoveDirectoryName().c_str());
}

// RemoveDirectoryName("adir/afile") -> "afile"
TEST(RemoveDirectoryNameTest, ShouldGiveFileNameForAlternateSeparator) {
  EXPECT_STREQ("afile",
               FilePath("adir/afile").RemoveDirectoryName().c_str());
}

// RemoveDirectoryName("adir/subdir/afile") -> "afile"
TEST(RemoveDirectoryNameTest, ShouldAlsoGiveFileNameForAlternateSeparator) {
  EXPECT_STREQ("afile",
               FilePath("adir/subdir/afile").RemoveDirectoryName().c_str());
}

#endif

// RemoveFileName "" -> "./"
TEST(RemoveFileNameTest, EmptyName) {
#if GTEST_OS_WINDOWS_MOBILE
  // On Windows CE, we use the root as the current directory.
  EXPECT_STREQ(GTEST_PATH_SEP_,
      FilePath("").RemoveFileName().c_str());
#else
  EXPECT_STREQ("." GTEST_PATH_SEP_,
      FilePath("").RemoveFileName().c_str());
#endif
}

// RemoveFileName "adir/" -> "adir/"
TEST(RemoveFileNameTest, ButNoFile) {
  EXPECT_STREQ("adir" GTEST_PATH_SEP_,
      FilePath("adir" GTEST_PATH_SEP_).RemoveFileName().c_str());
}

// RemoveFileName "adir/afile" -> "adir/"
TEST(RemoveFileNameTest, GivesDirName) {
  EXPECT_STREQ("adir" GTEST_PATH_SEP_,
      FilePath("adir" GTEST_PATH_SEP_ "afile")
      .RemoveFileName().c_str());
}

// RemoveFileName "adir/subdir/afile" -> "adir/subdir/"
TEST(RemoveFileNameTest, GivesDirAndSubDirName) {
  EXPECT_STREQ("adir" GTEST_PATH_SEP_ "subdir" GTEST_PATH_SEP_,
      FilePath("adir" GTEST_PATH_SEP_ "subdir" GTEST_PATH_SEP_ "afile")
      .RemoveFileName().c_str());
}

// RemoveFileName "/afile" -> "/"
TEST(RemoveFileNameTest, GivesRootDir) {
  EXPECT_STREQ(GTEST_PATH_SEP_,
      FilePath(GTEST_PATH_SEP_ "afile").RemoveFileName().c_str());
}

#if GTEST_HAS_ALT_PATH_SEP_

// Tests that RemoveFileName() works with the alternate separator on
// Windows.

// RemoveFileName("adir/") -> "adir/"
TEST(RemoveFileNameTest, ButNoFileForAlternateSeparator) {
  EXPECT_STREQ("adir" GTEST_PATH_SEP_,
               FilePath("adir/").RemoveFileName().c_str());
}

// RemoveFileName("adir/afile") -> "adir/"
TEST(RemoveFileNameTest, GivesDirNameForAlternateSeparator) {
  EXPECT_STREQ("adir" GTEST_PATH_SEP_,
               FilePath("adir/afile").RemoveFileName().c_str());
}

// RemoveFileName("adir/subdir/afile") -> "adir/subdir/"
TEST(RemoveFileNameTest, GivesDirAndSubDirNameForAlternateSeparator) {
  EXPECT_STREQ("adir" GTEST_PATH_SEP_ "subdir" GTEST_PATH_SEP_,
               FilePath("adir/subdir/afile").RemoveFileName().c_str());
}

// RemoveFileName("/afile") -> "\"
TEST(RemoveFileNameTest, GivesRootDirForAlternateSeparator) {
  EXPECT_STREQ(GTEST_PATH_SEP_,
               FilePath("/afile").RemoveFileName().c_str());
}

#endif

TEST(MakeFileNameTest, GenerateWhenNumberIsZero) {
  FilePath actual = FilePath::MakeFileName(FilePath("foo"), FilePath("bar"),
      0, "xml");
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar.xml", actual.c_str());
}

TEST(MakeFileNameTest, GenerateFileNameNumberGtZero) {
  FilePath actual = FilePath::MakeFileName(FilePath("foo"), FilePath("bar"),
      12, "xml");
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar_12.xml", actual.c_str());
}

TEST(MakeFileNameTest, GenerateFileNameWithSlashNumberIsZero) {
  FilePath actual = FilePath::MakeFileName(FilePath("foo" GTEST_PATH_SEP_),
      FilePath("bar"), 0, "xml");
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar.xml", actual.c_str());
}

TEST(MakeFileNameTest, GenerateFileNameWithSlashNumberGtZero) {
  FilePath actual = FilePath::MakeFileName(FilePath("foo" GTEST_PATH_SEP_),
      FilePath("bar"), 12, "xml");
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar_12.xml", actual.c_str());
}

TEST(MakeFileNameTest, GenerateWhenNumberIsZeroAndDirIsEmpty) {
  FilePath actual = FilePath::MakeFileName(FilePath(""), FilePath("bar"),
      0, "xml");
  EXPECT_STREQ("bar.xml", actual.c_str());
}

TEST(MakeFileNameTest, GenerateWhenNumberIsNotZeroAndDirIsEmpty) {
  FilePath actual = FilePath::MakeFileName(FilePath(""), FilePath("bar"),
      14, "xml");
  EXPECT_STREQ("bar_14.xml", actual.c_str());
}

TEST(ConcatPathsTest, WorksWhenDirDoesNotEndWithPathSep) {
  FilePath actual = FilePath::ConcatPaths(FilePath("foo"),
                                          FilePath("bar.xml"));
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar.xml", actual.c_str());
}

TEST(ConcatPathsTest, WorksWhenPath1EndsWithPathSep) {
  FilePath actual = FilePath::ConcatPaths(FilePath("foo" GTEST_PATH_SEP_),
                                          FilePath("bar.xml"));
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar.xml", actual.c_str());
}

TEST(ConcatPathsTest, Path1BeingEmpty) {
  FilePath actual = FilePath::ConcatPaths(FilePath(""),
                                          FilePath("bar.xml"));
  EXPECT_STREQ("bar.xml", actual.c_str());
}

TEST(ConcatPathsTest, Path2BeingEmpty) {
  FilePath actual = FilePath::ConcatPaths(FilePath("foo"),
                                          FilePath(""));
  EXPECT_STREQ("foo" GTEST_PATH_SEP_, actual.c_str());
}

TEST(ConcatPathsTest, BothPathBeingEmpty) {
  FilePath actual = FilePath::ConcatPaths(FilePath(""),
                                          FilePath(""));
  EXPECT_STREQ("", actual.c_str());
}

TEST(ConcatPathsTest, Path1ContainsPathSep) {
  FilePath actual = FilePath::ConcatPaths(FilePath("foo" GTEST_PATH_SEP_ "bar"),
                                          FilePath("foobar.xml"));
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar" GTEST_PATH_SEP_ "foobar.xml",
               actual.c_str());
}

TEST(ConcatPathsTest, Path2ContainsPathSep) {
  FilePath actual = FilePath::ConcatPaths(
      FilePath("foo" GTEST_PATH_SEP_),
      FilePath("bar" GTEST_PATH_SEP_ "bar.xml"));
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar" GTEST_PATH_SEP_ "bar.xml",
               actual.c_str());
}

TEST(ConcatPathsTest, Path2EndsWithPathSep) {
  FilePath actual = FilePath::ConcatPaths(FilePath("foo"),
                                          FilePath("bar" GTEST_PATH_SEP_));
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar" GTEST_PATH_SEP_, actual.c_str());
}

// RemoveTrailingPathSeparator "" -> ""
TEST(RemoveTrailingPathSeparatorTest, EmptyString) {
  EXPECT_STREQ("",
      FilePath("").RemoveTrailingPathSeparator().c_str());
}

// RemoveTrailingPathSeparator "foo" -> "foo"
TEST(RemoveTrailingPathSeparatorTest, FileNoSlashString) {
  EXPECT_STREQ("foo",
      FilePath("foo").RemoveTrailingPathSeparator().c_str());
}

// RemoveTrailingPathSeparator "foo/" -> "foo"
TEST(RemoveTrailingPathSeparatorTest, ShouldRemoveTrailingSeparator) {
  EXPECT_STREQ(
      "foo",
      FilePath("foo" GTEST_PATH_SEP_).RemoveTrailingPathSeparator().c_str());
#if GTEST_HAS_ALT_PATH_SEP_
  EXPECT_STREQ("foo",
               FilePath("foo/").RemoveTrailingPathSeparator().c_str());
#endif
}

// RemoveTrailingPathSeparator "foo/bar/" -> "foo/bar/"
TEST(RemoveTrailingPathSeparatorTest, ShouldRemoveLastSeparator) {
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar",
               FilePath("foo" GTEST_PATH_SEP_ "bar" GTEST_PATH_SEP_)
               .RemoveTrailingPathSeparator().c_str());
}

// RemoveTrailingPathSeparator "foo/bar" -> "foo/bar"
TEST(RemoveTrailingPathSeparatorTest, ShouldReturnUnmodified) {
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar",
               FilePath("foo" GTEST_PATH_SEP_ "bar")
               .RemoveTrailingPathSeparator().c_str());
}

TEST(DirectoryTest, RootDirectoryExists) {
#if GTEST_OS_WINDOWS  // We are on Windows.
  char current_drive[_MAX_PATH];  // NOLINT
  current_drive[0] = static_cast<char>(_getdrive() + 'A' - 1);
  current_drive[1] = ':';
  current_drive[2] = '\\';
  current_drive[3] = '\0';
  EXPECT_TRUE(FilePath(current_drive).DirectoryExists());
#else
  EXPECT_TRUE(FilePath("/").DirectoryExists());
#endif  // GTEST_OS_WINDOWS
}

#if GTEST_OS_WINDOWS
TEST(DirectoryTest, RootOfWrongDriveDoesNotExists) {
  const int saved_drive_ = _getdrive();
  // Find a drive that doesn't exist. Start with 'Z' to avoid common ones.
  for (char drive = 'Z'; drive >= 'A'; drive--)
    if (_chdrive(drive - 'A' + 1) == -1) {
      char non_drive[_MAX_PATH];  // NOLINT
      non_drive[0] = drive;
      non_drive[1] = ':';
      non_drive[2] = '\\';
      non_drive[3] = '\0';
      EXPECT_FALSE(FilePath(non_drive).DirectoryExists());
      break;
    }
  _chdrive(saved_drive_);
}
#endif  // GTEST_OS_WINDOWS

#if !GTEST_OS_WINDOWS_MOBILE
// Windows CE _does_ consider an empty directory to exist.
TEST(DirectoryTest, EmptyPathDirectoryDoesNotExist) {
  EXPECT_FALSE(FilePath("").DirectoryExists());
}
#endif  // !GTEST_OS_WINDOWS_MOBILE

TEST(DirectoryTest, CurrentDirectoryExists) {
#if GTEST_OS_WINDOWS  // We are on Windows.
# ifndef _WIN32_CE  // Windows CE doesn't have a current directory.

  EXPECT_TRUE(FilePath(".").DirectoryExists());
  EXPECT_TRUE(FilePath(".\\").DirectoryExists());

# endif  // _WIN32_CE
#else
  EXPECT_TRUE(FilePath(".").DirectoryExists());
  EXPECT_TRUE(FilePath("./").DirectoryExists());
#endif  // GTEST_OS_WINDOWS
}

TEST(NormalizeTest, NullStringsEqualEmptyDirectory) {
  EXPECT_STREQ("", FilePath(NULL).c_str());
  EXPECT_STREQ("", FilePath(String(NULL)).c_str());
}

// "foo/bar" == foo//bar" == "foo///bar"
TEST(NormalizeTest, MultipleConsecutiveSepaparatorsInMidstring) {
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar",
               FilePath("foo" GTEST_PATH_SEP_ "bar").c_str());
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar",
               FilePath("foo" GTEST_PATH_SEP_ GTEST_PATH_SEP_ "bar").c_str());
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar",
               FilePath("foo" GTEST_PATH_SEP_ GTEST_PATH_SEP_
                        GTEST_PATH_SEP_ "bar").c_str());
}

// "/bar" == //bar" == "///bar"
TEST(NormalizeTest, MultipleConsecutiveSepaparatorsAtStringStart) {
  EXPECT_STREQ(GTEST_PATH_SEP_ "bar",
    FilePath(GTEST_PATH_SEP_ "bar").c_str());
  EXPECT_STREQ(GTEST_PATH_SEP_ "bar",
    FilePath(GTEST_PATH_SEP_ GTEST_PATH_SEP_ "bar").c_str());
  EXPECT_STREQ(GTEST_PATH_SEP_ "bar",
    FilePath(GTEST_PATH_SEP_ GTEST_PATH_SEP_ GTEST_PATH_SEP_ "bar").c_str());
}

// "foo/" == foo//" == "foo///"
TEST(NormalizeTest, MultipleConsecutiveSepaparatorsAtStringEnd) {
  EXPECT_STREQ("foo" GTEST_PATH_SEP_,
    FilePath("foo" GTEST_PATH_SEP_).c_str());
  EXPECT_STREQ("foo" GTEST_PATH_SEP_,
    FilePath("foo" GTEST_PATH_SEP_ GTEST_PATH_SEP_).c_str());
  EXPECT_STREQ("foo" GTEST_PATH_SEP_,
    FilePath("foo" GTEST_PATH_SEP_ GTEST_PATH_SEP_ GTEST_PATH_SEP_).c_str());
}

#if GTEST_HAS_ALT_PATH_SEP_

// Tests that separators at the end of the string are normalized
// regardless of their combination (e.g. "foo\" =="foo/\" ==
// "foo\\/").
TEST(NormalizeTest, MixAlternateSeparatorAtStringEnd) {
  EXPECT_STREQ("foo" GTEST_PATH_SEP_,
               FilePath("foo/").c_str());
  EXPECT_STREQ("foo" GTEST_PATH_SEP_,
               FilePath("foo" GTEST_PATH_SEP_ "/").c_str());
  EXPECT_STREQ("foo" GTEST_PATH_SEP_,
               FilePath("foo//" GTEST_PATH_SEP_).c_str());
}

#endif

TEST(AssignmentOperatorTest, DefaultAssignedToNonDefault) {
  FilePath default_path;
  FilePath non_default_path("path");
  non_default_path = default_path;
  EXPECT_STREQ("", non_default_path.c_str());
  EXPECT_STREQ("", default_path.c_str());  // RHS var is unchanged.
}

TEST(AssignmentOperatorTest, NonDefaultAssignedToDefault) {
  FilePath non_default_path("path");
  FilePath default_path;
  default_path = non_default_path;
  EXPECT_STREQ("path", default_path.c_str());
  EXPECT_STREQ("path", non_default_path.c_str());  // RHS var is unchanged.
}

TEST(AssignmentOperatorTest, ConstAssignedToNonConst) {
  const FilePath const_default_path("const_path");
  FilePath non_default_path("path");
  non_default_path = const_default_path;
  EXPECT_STREQ("const_path", non_default_path.c_str());
}

class DirectoryCreationTest : public Test {
 protected:
  virtual void SetUp() {
    testdata_path_.Set(FilePath(String::Format("%s%s%s",
        TempDir().c_str(), GetCurrentExecutableName().c_str(),
        "_directory_creation" GTEST_PATH_SEP_ "test" GTEST_PATH_SEP_)));
    testdata_file_.Set(testdata_path_.RemoveTrailingPathSeparator());

    unique_file0_.Set(FilePath::MakeFileName(testdata_path_, FilePath("unique"),
        0, "txt"));
    unique_file1_.Set(FilePath::MakeFileName(testdata_path_, FilePath("unique"),
        1, "txt"));

    remove(testdata_file_.c_str());
    remove(unique_file0_.c_str());
    remove(unique_file1_.c_str());
    posix::RmDir(testdata_path_.c_str());
  }

  virtual void TearDown() {
    remove(testdata_file_.c_str());
    remove(unique_file0_.c_str());
    remove(unique_file1_.c_str());
    posix::RmDir(testdata_path_.c_str());
  }

  String TempDir() const {
#if GTEST_OS_WINDOWS_MOBILE
    return String("\\temp\\");
#elif GTEST_OS_WINDOWS
    const char* temp_dir = posix::GetEnv("TEMP");
    if (temp_dir == NULL || temp_dir[0] == '\0')
      return String("\\temp\\");
    else if (String(temp_dir).EndsWith("\\"))
      return String(temp_dir);
    else
      return String::Format("%s\\", temp_dir);
#else
    return String("/tmp/");
#endif  // GTEST_OS_WINDOWS_MOBILE
  }

  void CreateTextFile(const char* filename) {
    FILE* f = posix::FOpen(filename, "w");
    fprintf(f, "text\n");
    fclose(f);
  }

  // Strings representing a directory and a file, with identical paths
  // except for the trailing separator character that distinquishes
  // a directory named 'test' from a file named 'test'. Example names:
  FilePath testdata_path_;  // "/tmp/directory_creation/test/"
  FilePath testdata_file_;  // "/tmp/directory_creation/test"
  FilePath unique_file0_;  // "/tmp/directory_creation/test/unique.txt"
  FilePath unique_file1_;  // "/tmp/directory_creation/test/unique_1.txt"
};

TEST_F(DirectoryCreationTest, CreateDirectoriesRecursively) {
  EXPECT_FALSE(testdata_path_.DirectoryExists()) << testdata_path_.c_str();
  EXPECT_TRUE(testdata_path_.CreateDirectoriesRecursively());
  EXPECT_TRUE(testdata_path_.DirectoryExists());
}

TEST_F(DirectoryCreationTest, CreateDirectoriesForAlreadyExistingPath) {
  EXPECT_FALSE(testdata_path_.DirectoryExists()) << testdata_path_.c_str();
  EXPECT_TRUE(testdata_path_.CreateDirectoriesRecursively());
  // Call 'create' again... should still succeed.
  EXPECT_TRUE(testdata_path_.CreateDirectoriesRecursively());
}

TEST_F(DirectoryCreationTest, CreateDirectoriesAndUniqueFilename) {
  FilePath file_path(FilePath::GenerateUniqueFileName(testdata_path_,
      FilePath("unique"), "txt"));
  EXPECT_STREQ(unique_file0_.c_str(), file_path.c_str());
  EXPECT_FALSE(file_path.FileOrDirectoryExists());  // file not there

  testdata_path_.CreateDirectoriesRecursively();
  EXPECT_FALSE(file_path.FileOrDirectoryExists());  // file still not there
  CreateTextFile(file_path.c_str());
  EXPECT_TRUE(file_path.FileOrDirectoryExists());

  FilePath file_path2(FilePath::GenerateUniqueFileName(testdata_path_,
      FilePath("unique"), "txt"));
  EXPECT_STREQ(unique_file1_.c_str(), file_path2.c_str());
  EXPECT_FALSE(file_path2.FileOrDirectoryExists());  // file not there
  CreateTextFile(file_path2.c_str());
  EXPECT_TRUE(file_path2.FileOrDirectoryExists());
}

TEST_F(DirectoryCreationTest, CreateDirectoriesFail) {
  // force a failure by putting a file where we will try to create a directory.
  CreateTextFile(testdata_file_.c_str());
  EXPECT_TRUE(testdata_file_.FileOrDirectoryExists());
  EXPECT_FALSE(testdata_file_.DirectoryExists());
  EXPECT_FALSE(testdata_file_.CreateDirectoriesRecursively());
}

TEST(NoDirectoryCreationTest, CreateNoDirectoriesForDefaultXmlFile) {
  const FilePath test_detail_xml("test_detail.xml");
  EXPECT_FALSE(test_detail_xml.CreateDirectoriesRecursively());
}

TEST(FilePathTest, DefaultConstructor) {
  FilePath fp;
  EXPECT_STREQ("", fp.c_str());
}

TEST(FilePathTest, CharAndCopyConstructors) {
  const FilePath fp("spicy");
  EXPECT_STREQ("spicy", fp.c_str());

  const FilePath fp_copy(fp);
  EXPECT_STREQ("spicy", fp_copy.c_str());
}

TEST(FilePathTest, StringConstructor) {
  const FilePath fp(String("cider"));
  EXPECT_STREQ("cider", fp.c_str());
}

TEST(FilePathTest, Set) {
  const FilePath apple("apple");
  FilePath mac("mac");
  mac.Set(apple);  // Implement Set() since overloading operator= is forbidden.
  EXPECT_STREQ("apple", mac.c_str());
  EXPECT_STREQ("apple", apple.c_str());
}

TEST(FilePathTest, ToString) {
  const FilePath file("drink");
  String str(file.ToString());
  EXPECT_STREQ("drink", str.c_str());
}

TEST(FilePathTest, RemoveExtension) {
  EXPECT_STREQ("app", FilePath("app.exe").RemoveExtension("exe").c_str());
  EXPECT_STREQ("APP", FilePath("APP.EXE").RemoveExtension("exe").c_str());
}

TEST(FilePathTest, RemoveExtensionWhenThereIsNoExtension) {
  EXPECT_STREQ("app", FilePath("app").RemoveExtension("exe").c_str());
}

TEST(FilePathTest, IsDirectory) {
  EXPECT_FALSE(FilePath("cola").IsDirectory());
  EXPECT_TRUE(FilePath("koala" GTEST_PATH_SEP_).IsDirectory());
#if GTEST_HAS_ALT_PATH_SEP_
  EXPECT_TRUE(FilePath("koala/").IsDirectory());
#endif
}

TEST(FilePathTest, IsAbsolutePath) {
  EXPECT_FALSE(FilePath("is" GTEST_PATH_SEP_ "relative").IsAbsolutePath());
  EXPECT_FALSE(FilePath("").IsAbsolutePath());
#if GTEST_OS_WINDOWS
  EXPECT_TRUE(FilePath("c:\\" GTEST_PATH_SEP_ "is_not"
                       GTEST_PATH_SEP_ "relative").IsAbsolutePath());
  EXPECT_FALSE(FilePath("c:foo" GTEST_PATH_SEP_ "bar").IsAbsolutePath());
  EXPECT_TRUE(FilePath("c:/" GTEST_PATH_SEP_ "is_not"
                       GTEST_PATH_SEP_ "relative").IsAbsolutePath());
#else
  EXPECT_TRUE(FilePath(GTEST_PATH_SEP_ "is_not" GTEST_PATH_SEP_ "relative")
              .IsAbsolutePath());
#endif  // GTEST_OS_WINDOWS
}

TEST(FilePathTest, IsRootDirectory) {
#if GTEST_OS_WINDOWS
  EXPECT_TRUE(FilePath("a:\\").IsRootDirectory());
  EXPECT_TRUE(FilePath("Z:/").IsRootDirectory());
  EXPECT_TRUE(FilePath("e://").IsRootDirectory());
  EXPECT_FALSE(FilePath("").IsRootDirectory());
  EXPECT_FALSE(FilePath("b:").IsRootDirectory());
  EXPECT_FALSE(FilePath("b:a").IsRootDirectory());
  EXPECT_FALSE(FilePath("8:/").IsRootDirectory());
  EXPECT_FALSE(FilePath("c|/").IsRootDirectory());
#else
  EXPECT_TRUE(FilePath("/").IsRootDirectory());
  EXPECT_TRUE(FilePath("//").IsRootDirectory());
  EXPECT_FALSE(FilePath("").IsRootDirectory());
  EXPECT_FALSE(FilePath("\\").IsRootDirectory());
  EXPECT_FALSE(FilePath("/x").IsRootDirectory());
#endif
}

}  // namespace
}  // namespace internal
}  // namespace testing
