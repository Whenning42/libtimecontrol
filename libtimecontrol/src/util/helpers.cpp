#include "src/util/helpers.h"

#include <stdio.h>

#include <random>

std::string generate_random_hex(int bytes) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 255);

  std::string hex(bytes * 2, '\0');
  for (int i = 0; i < bytes; i++) {
    snprintf(&hex[i * 2], 3, "%02x", dis(gen));
  }

  return hex;
}
