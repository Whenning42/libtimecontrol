#include "src/util/helpers.h"

#include <gtest/gtest.h>

#include <regex>

TEST(GenerateRandomHex, GeneratesExpectedStrings) {
  std::string hex = generate_random_hex(4);
  std::regex pattern("^[0-9a-f]{8}$");
  EXPECT_TRUE(std::regex_match(hex, pattern))
      << "Generated hex '" << hex << "' does not match expected pattern";
}
