#include <utils/line_buffer.hpp>

#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::Eq;
using testing::Optional;

namespace {

TEST (LineBufferTest, EmptyBuffer) {
  utils::line_buffer buffer {};

  auto line = buffer.pop_line ();
  EXPECT_THAT (line, Eq (std::nullopt));
}

TEST (LineBufferTest, Success) {
  utils::line_buffer buffer {};
  std::string line1 {"Ala ma kota"};
  buffer.append (line1 + '\n');
  auto line = buffer.pop_line ();
  EXPECT_THAT (line, Optional (Eq (line1)));
}

TEST (LineBufferTest, MultipleLines) {
  utils::line_buffer buffer {};
  buffer.append ("first\nsecond\nthird\n");

  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("first")));
  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("second")));
  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("third")));
  EXPECT_THAT (buffer.pop_line (), Eq (std::nullopt));
}

TEST (LineBufferTest, IncrementalAppend) {
  utils::line_buffer buffer {};
  buffer.append ("hel");
  buffer.append ("lo wo");
  buffer.append ("rld\n");

  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("hello world")));
}

TEST (LineBufferTest, IncompleteLineReturnsNullopt) {
  utils::line_buffer buffer {};
  buffer.append ("no newline yet");

  EXPECT_THAT (buffer.pop_line (), Eq (std::nullopt));
  EXPECT_FALSE (buffer.empty ());
}

TEST (LineBufferTest, IncompleteLineThenCompleted) {
  utils::line_buffer buffer {};
  buffer.append ("partial");
  EXPECT_THAT (buffer.pop_line (), Eq (std::nullopt));

  buffer.append (" line\n");
  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("partial line")));
}

TEST (LineBufferTest, CrlfStripping) {
  utils::line_buffer buffer {};
  buffer.append ("windows line\r\n");

  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("windows line")));
}

TEST (LineBufferTest, CrlfMultipleLines) {
  utils::line_buffer buffer {};
  buffer.append ("first\r\nsecond\r\n");

  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("first")));
  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("second")));
}

TEST (LineBufferTest, EmptyLine) {
  utils::line_buffer buffer {};
  buffer.append ("\n");

  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("")));
}

TEST (LineBufferTest, ConsecutiveEmptyLines) {
  utils::line_buffer buffer {};
  buffer.append ("\n\n\n");

  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("")));
  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("")));
  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("")));
  EXPECT_THAT (buffer.pop_line (), Eq (std::nullopt));
}

TEST (LineBufferTest, EmptyAfterAllLinesPopped) {
  utils::line_buffer buffer {};
  buffer.append ("line\n");
  std::ignore = buffer.pop_line ();

  EXPECT_TRUE (buffer.empty ());
}

TEST (LineBufferTest, NotEmptyWithRemainder) {
  utils::line_buffer buffer {};
  buffer.append ("line\nremainder");
  std::ignore = buffer.pop_line ();

  EXPECT_FALSE (buffer.empty ());
}

TEST (LineBufferTest, MixedLineEndings) {
  utils::line_buffer buffer {};
  buffer.append ("unix\nwindows\r\nunix2\n");

  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("unix")));
  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("windows")));
  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("unix2")));
}

TEST (LineBufferTest, LargeChunk) {
  utils::line_buffer buffer {};
  std::string long_line (4096, 'x');
  buffer.append (long_line + "\n");

  EXPECT_THAT (buffer.pop_line (), Optional (Eq (long_line)));
}

TEST (LineBufferTest, CustomInitialCapacity) {
  utils::line_buffer<64> buffer {};
  buffer.append ("short\n");

  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("short")));
}

TEST (LineBufferTest, CustomInitialCapacityExceeded) {
  utils::line_buffer<64> buffer {};
  std::string long_line (124, 'x');
  buffer.append (long_line + '\n');

  EXPECT_THAT (buffer.pop_line (), Optional (Eq (long_line)));
}

TEST (LineBufferTest, BareCarriageReturnPreserved) {
  utils::line_buffer buffer {};
  buffer.append ("has\rmiddle cr\n");

  EXPECT_THAT (buffer.pop_line (), Optional (Eq ("has\rmiddle cr")));
}

}  // namespace
