#ifndef WORD_H
#define WORD_H

#include "constants.hpp"

#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

class Word {
 public:
  Word(const std::string& text) 
  : d_(text), 
    encoded_25bit_word_(encode_25bit_word(text)),
    letter_counts_(count_letters(text)), 
    encoded_letter_counts_(encode_letter_counts(letter_counts_)), 
    letter_mask_(mask_letters(letter_counts_)) {}

  const char* letters() const { return d_.letters; }
  char letter(int i) const { return d_.letters[i]; }
  uint32_t encoded_25bit_word() const { return encoded_25bit_word_; }
  uint64_t bits() const { return d_.bits; }
  const std::map<char, int> letter_counts() const { return letter_counts_; }
  uint64_t encoded_letter_counts() const { return encoded_letter_counts_; }
  uint64_t letter_mask() const { return letter_mask_; }

  bool operator==(const Word& other) const;

  friend std::ostream& operator<<(std::ostream& os, const Word& guess);

 private:
  static std::map<char, int> count_letters(const std::string&);
  static uint32_t encode_25bit_word(const std::string&);
  static uint64_t encode_letter_counts(const std::map<char, int>& letter_counts);
  static uint64_t mask_letters(const std::map<char, int>& letter_counts);

  union WordData {
    char letters[8];
    uint64_t bits;

    WordData(const std::string& text) {
      bits = 0;
      strcpy(letters, text.c_str());
    }
  };

  const WordData d_;
  const uint32_t encoded_25bit_word_;
  const std::map<char, int> letter_counts_;
  const uint64_t encoded_letter_counts_;
  const uint64_t letter_mask_;
};

bool Word::operator==(const Word& other) const {
  return this->d_.bits == other.d_.bits;
}

std::map<char, int> Word::count_letters(const std::string& word) {
  std::map<char, int> letter_counts;
  for (char c : word) {
    ++letter_counts[c];
  }
  return letter_counts;
}

uint32_t Word::encode_25bit_word(const std::string& word) {
  /**
   * Encode word as a sequence of five 5-bit numbers
   * --> 25 bits, 7 bits padding
   *
   * adult is encoded as:
   * -------    t    l    u    d    a
   * 00000001001101011101000001100000
   */
  uint32_t encoded_word = 0;
  for (int i = 0; i < NUM_LETTERS; ++i) {
    unsigned char c = word[i] - 'a';
    encoded_word |= (uint32_t) c << BITS_PER_LETTER * i;
  }
  return encoded_word;
}

uint64_t Word::encode_letter_counts(const std::map<char, int>& letter_counts) {
  /**
   * Encode letter counts as 2-bit count per letter
   * --> 52 bits, 12 bits padding
   *
   * aorta is encoded as:
   * ------------            1t  1r    1o                          2a
   * 0000000000000000000000000100010000010000000000000000000000000010
   */
  uint64_t encoded_count = 0;
  for (const auto& [l, ct] : letter_counts) {
    unsigned char pos = l - 'a';
    encoded_count |= (uint64_t) ct << BITS_PER_COUNT * pos;
  }
  return encoded_count;
}

uint64_t Word::mask_letters(const std::map<char, int>& letter_counts) {
  /**
   * Encode letter counts by setting one of the first 26 bits
   * for the first occurrance, one of the second 26 bits for 
   * the second occurrance, and the 52nd bit for any third occurrance.
   */
  uint64_t encoded_mask = 0;
  for (const auto& [l, ct] : letter_counts) {
    unsigned char pos = l - 'a';
    encoded_mask |= 1 << pos;
    if (ct > 1) {
      encoded_mask |= (uint64_t) 1 << (pos + 26);
      if (ct > 2) {
        encoded_mask |= (uint64_t) 1 << (2 * 26);
      }
    }
  }
  return encoded_mask;
}

std::ostream& operator<<(std::ostream& os, const Word& word) {
  os << word.d_.letters << ENDC;
  return os;
}

template<> struct std::hash<Word> {
  std::size_t operator()(const Word& w) const noexcept {
    return std::hash<std::uint64_t>{}(w.bits());
  }
};

#endif