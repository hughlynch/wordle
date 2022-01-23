#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "constants.hpp"
#include "guess.hpp"
#include "word.hpp"

#include <boost/dynamic_bitset.hpp>
#include <iostream>
#include <map>
#include <vector>

const uint64_t LSB_MASK = 0x5555555555555555;
const uint64_t MSB_MASK = 0xAAAAAAAAAAAAAAAA;

class Dictionary {
 public:
  Dictionary(const std::vector<Word> words)
    : all_words(words), pruned_(words.size(), false) {
  }

  /**
   * Prune dictionary using the inferences made in guess.
   *
   * Returns a vector matching this.pruned_ after pruning the given guess.
   * Replaces this.pruned_ if inplace.
   */
  std::vector<bool> prune(const Guess& guess) const;

  size_t size() const;

  const std::vector<Word> all_words;

 private:
  /**
   * Returns the final borrow bit of 2-element-wise x - y.
   */
  static uint64_t borrow_2bit(uint64_t x, uint64_t y);

  /**
   * For each word, store an encoded representation of the letter + positions,
   * as well as a 2-bit letter count for each letter and a 'pruned' vector to
   * denote whether a word should be considered pruned from the dataset.
   */
  std::vector<bool> pruned_;
};


/**
 * Public implementations
 */

std::vector<bool> Dictionary::prune(const Guess& guess) const {
  std::vector<bool> post_pruned;
  post_pruned.reserve(this->pruned_.size());

  /**
   * Prune based on correct placements.
   * Anything that does not match all correct placements will be pruned.
   *
   * Assemble a check of all correct placements and XOR with word.
   * For guess share on solve, we have correct letters (0,s), (4,e):
   *                 e                   s
   *      00000000010000000000000000010010
   *  XOR                   (encoded word)
   *  AND 00000001111100000000000000011111
   *
   *  any bits mean there was a mismatch => prune
   */
  uint32_t c_check = 0;
  uint32_t c_mask = 0;
  for (const auto& [pos, l] : guess.correct_placements) {
    c_check |= ((uint32_t) l - 'a') << BITS_PER_LETTER * pos;
    c_mask |= (uint32_t) 0b11111 << BITS_PER_LETTER * pos;
  }

  //std::cout << std::bitset<32>(c_check) << std::endl;
  //std::cout << std::bitset<32>(c_mask) << std::endl;
  //std::cout << std::bitset<32>(c_result) << std::endl;

  /**
   * Prune based on incorrect placements.
   * Anything that matches a placement will be pruned.
   *
   * We perform the same check and mask as for correct placements, but now
   * prune if any blocks (letters) match. We cannot do this in a single
   * bitwise operation and have to check each block individually (AFAICT).
   *
   *                 e                   s
   *      00000000010000000000000000010010
   *  XOR                   (encoded word)
   *  OR  11111110000011111111111111100000
   *
   *  if any block == 00000, there was a match => prune
   */
  uint32_t w_check = 0;
  uint32_t w_mask = 0xFFFFFFFF;
  for (const auto& [pos, l] : guess.wrong_placements) {
    w_check |= ((uint32_t) l - 'a') << BITS_PER_LETTER * pos;
    w_mask ^= (uint32_t) 0b11111 << BITS_PER_LETTER * pos;
  }

  /**
   * Prune based on minimum letter count.
   * Any letter that has less than the minimum letter count will be pruned.
   *
   * We can perform a partial 2-bit subtraction on each separate letter block
   * to get the the sign bit of the subtraction result.
   *
   * We take the borrow bit of a half subtractor from subtracting the first
   * digit of each block, and then compute the borrow bit of a full subtractor
   * for the second digit of each block.
   *
   * We encode the minimum letter checks the same way as the letter counts.
   * (a,2), (s,1) is encoded as:
   *                                                               (count)
   *                                1s                                  2a
   *  -   0000000000000000000000000001000000000000000000000000000000000010
   *
   * If any 1s, then the result of the subtraction is negative => prune
   */
  uint64_t min_cts = 0;
  for (const auto& [l, ct] : guess.min_letter_counts) {
    min_cts |= (uint64_t) ct << BITS_PER_COUNT * (l - 'a');
  }
  //std::cout << std::bitset<64>(min_cts) << std::endl;

  /**
   * Prune based on maximum letter count.
   * Any letter that has more than the maximum letter count will be pruned.
   *
   * We perform the same subtraction method in the opposite order to get the
   * sign bit of:
   * max_letter_ct - (count)
   */
  uint64_t max_cts = 0;
  uint64_t max_mask = 0;
  for (const auto& [l, ct] : guess.max_letter_counts) {
    max_cts |= (uint64_t) ct << BITS_PER_COUNT * (l - 'a');
    max_mask |= (uint64_t) 0b11 << BITS_PER_COUNT * (l - 'a');
  }

  // Prune wordset
  for (size_t i = 0; i < this->pruned_.size(); ++i) {
    if (this->pruned_[i]) {
      // This element is already pruned, skip.
      post_pruned.push_back(true);
      continue;
    }

    //std::string word = this->reference_words[i];
    //std::cout << word << std::endl;

    const Word& w = all_words[i];
    const uint64_t encoded_word = w.encoded_25bit_word();

    // Check correct placements
    uint32_t c_result = (c_check ^ encoded_word) & c_mask;
    if (c_result) {
      post_pruned.push_back(true);
      continue;
    }

    // Check wrong placements
    uint32_t w_result = (w_check ^ encoded_word) | w_mask;
    bool w_prune = false;

    for (int pos = 0; pos < NUM_LETTERS; ++pos) {
      uint8_t block = (w_result >> BITS_PER_LETTER * pos) & 0b11111;
      if (!block) {
        w_prune = true;
        break;
      }
    }
    if (w_prune) {
      post_pruned.push_back(true);
      continue;
    }

    // Check min letter count
    uint64_t letter_cts = all_words[i].encoded_letter_counts();

    uint64_t min_result = borrow_2bit(letter_cts, min_cts);
    if (min_result) {
      post_pruned.push_back(true);
      continue;
    }



    // Check max letter count
    uint64_t max_result = max_mask & borrow_2bit(max_cts, letter_cts);
    if (max_result) {
      post_pruned.push_back(true);
      continue;
    }

    post_pruned.push_back(false);
  }

  return post_pruned;
}

// TODO make this return size of unpruned set
size_t Dictionary::size() const {
  return this->all_words.size();
}

/**
 * Private implementations
 */

uint64_t Dictionary::borrow_2bit(uint64_t x, uint64_t y) {
  // Get LSB borrow bit
  /**
   * This matches any 0/1 x/y pairs in the first digit.
   */
  uint64_t tmp = (~x & y);
  uint64_t b_in = LSB_MASK & tmp;

  /**
   * This matches any 0/1 pairs in the second digit:
   *     (~x & y)
   *   OR
   * any matching digits
   *     ~(x ^ y)
   *   with a borrow bit from the previous digit:
   *     (b_in << 1)
   */
  return MSB_MASK & (tmp | ((b_in << 1) & ~(x ^ y)));
}

#endif