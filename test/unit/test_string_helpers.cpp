#include "string_helpers.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <ctime>

// ---------------------------------------------------------------------------
// strip_prefix
// ---------------------------------------------------------------------------

TEST(StripPrefix, RemovesPresentPrefix) {
    EXPECT_EQ(strip_prefix("refs/heads/master", "refs/heads/"), "master");
}

TEST(StripPrefix, LeavesStringUnchangedWhenPrefixAbsent) {
    EXPECT_EQ(strip_prefix("refs/wip/master", "refs/heads/"), "refs/wip/master");
}

TEST(StripPrefix, EmptyPrefix) {
    EXPECT_EQ(strip_prefix("hello", ""), "hello");
}

TEST(StripPrefix, PrefixEqualsString) {
    EXPECT_EQ(strip_prefix("refs/heads/", "refs/heads/"), "");
}

TEST(StripPrefix, EmptyString) {
    EXPECT_EQ(strip_prefix("", "refs/heads/"), "");
}

TEST(StripPrefix, WipRefUnchanged) {
    EXPECT_EQ(strip_prefix("refs/wip/feature", "refs/heads/"), "refs/wip/feature");
}

// ---------------------------------------------------------------------------
// first_line
// ---------------------------------------------------------------------------

TEST(FirstLine, NullReturnsEmpty) {
    EXPECT_EQ(first_line(nullptr), "");
}

TEST(FirstLine, SingleLine) {
    EXPECT_EQ(first_line("hello world"), "hello world");
}

TEST(FirstLine, MultiLine) {
    EXPECT_EQ(first_line("subject\n\nbody"), "subject");
}

TEST(FirstLine, EmptyString) {
    EXPECT_EQ(first_line(""), "");
}

TEST(FirstLine, LeadingNewline) {
    EXPECT_EQ(first_line("\nsecond"), "");
}

TEST(FirstLine, TrailingNewline) {
    EXPECT_EQ(first_line("subject\n"), "subject");
}

// ---------------------------------------------------------------------------
// relative_time
//
// These tests produce a fixed epoch offset from "now" and check that the
// output falls into the right bucket.  We cannot check the exact number
// because the function reads the real wall clock, but the rounding logic
// is deterministic once we control the input epoch.
// ---------------------------------------------------------------------------

namespace {
// Returns an epoch seconds value that is `delta` seconds in the past.
std::int64_t seconds_ago(std::int64_t delta) {
    using namespace std::chrono;
    auto now = system_clock::now();
    return static_cast<std::int64_t>(
        system_clock::to_time_t(now - seconds{delta}));
}
} // namespace

TEST(RelativeTime, JustNow) {
    auto s = relative_time(seconds_ago(5));
    EXPECT_NE(s.find("second"), std::string::npos) << "got: " << s;
}

TEST(RelativeTime, OneMinuteAgo) {
    auto s = relative_time(seconds_ago(91)); // > 90 s → minutes bucket
    EXPECT_NE(s.find("minute"), std::string::npos) << "got: " << s;
}

TEST(RelativeTime, OneHourAgo) {
    auto s = relative_time(seconds_ago(91 * 60)); // > 90 min → hours bucket
    EXPECT_NE(s.find("hour"), std::string::npos) << "got: " << s;
}

TEST(RelativeTime, OneDayAgo) {
    auto s = relative_time(seconds_ago(37 * 3600)); // > 36 h → days bucket
    EXPECT_NE(s.find("day"), std::string::npos) << "got: " << s;
}

TEST(RelativeTime, TwoWeeksAgo) {
    auto s = relative_time(seconds_ago(15 * 24 * 3600)); // > 14 days → weeks
    EXPECT_NE(s.find("week"), std::string::npos) << "got: " << s;
}

TEST(RelativeTime, ThreeMonthsAgo) {
    auto s = relative_time(seconds_ago(90L * 24 * 3600)); // > 8 weeks → months
    EXPECT_NE(s.find("month"), std::string::npos) << "got: " << s;
}

TEST(RelativeTime, TwoYearsAgo) {
    auto s = relative_time(seconds_ago(730L * 24 * 3600)); // > 24 months → years
    EXPECT_NE(s.find("year"), std::string::npos) << "got: " << s;
}

TEST(RelativeTime, FutureTimestampClampsToZero) {
    // A future timestamp should not produce negative output.
    auto s = relative_time(seconds_ago(-3600)); // 1 hour in the future
    EXPECT_NE(s.find("second"), std::string::npos) << "got: " << s;
}

TEST(RelativeTime, SingularSecond) {
    // Exactly 1 second ago → "1 second ago" (singular)
    auto s = relative_time(seconds_ago(1));
    EXPECT_EQ(s, "1 second ago");
}

TEST(RelativeTime, SingularMinute) {
    auto s = relative_time(seconds_ago(91)); // first minute bucket entry
    EXPECT_NE(s.find("minute"), std::string::npos) << "got: " << s;
}
