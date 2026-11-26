// Forced-included (cl /FI) into MSVC TEST translation units only — see the
// suborno_configure_test_target comment in
// cmake/modules/SubornoBuildOptions.cmake for the full rationale.
//
// Why this exists: googletest 1.18's assertion machinery itself performs a
// bool == bool comparison inside
//     struct AssertionResultExpectation {
//         explicit operator bool() const {
//             bool converted(assertion_result);
//             return converted == expected_result;
//         }
//     };
// (include/gtest/gtest-assertion-result.h). When the tested expression folds
// to a compile-time constant (a constexpr function with constant arguments,
// a literal, an enum constant), MSVC's analyzer folds the whole comparison to
// constant-vs-constant and reports C6326 "Potential comparison of a constant
// with another constant" at every EXPECT_*/ASSERT_* expansion line — ~1000
// per build, all attributed to test files, none from this project's logic.
// Microsoft tracks the class as a false positive ("False positive with
// static code analysis and GoogleTest", Developer Community #10978073).
//
// Scope of the suppression: the C6326 diagnostic rule, on test targets,
// MSVC /analyze only. Code analysis itself stays enabled for tests
// (/analyze + /analyze:external-), every other rule stays on, and no
// compiler warning (C4305/C4309/C4584/C4201/...) is touched — those are
// fixed in the source instead.
#pragma warning(disable : 6326)
