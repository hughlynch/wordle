#ifndef GUESS_H
#define GUESS_H

#include "constants.hpp"

#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

class Guess {
 public:
  Guess(const std::string word)
    : word_(word) {}

  /*
   * Generate internal state by checking against a solution word.
   *
   * infer: whether or not to make inferences after setting tiles.
   */
  void check(const std::string solution, bool infer);
  void check(const std::string solution);

  /**
   * Print internal state of this guess.
   */
  void print_state() const;

  /**
   * Make a set of auxilliary inferences used for pruning a wordset based on the
   * internal state of this guess.
   */
  void infer();

  bool operator==(const Guess& other) const;

  uint64_t id_string() const;

  friend std::ostream& operator<<(std::ostream& os, const Guess& guess);

  // Auxilliary inferences
  std::vector<std::pair<int, char>> correct_placements;
  std::vector<std::pair<int, char>> wrong_placements;
  std::map<char, int> min_letter_counts;
  std::map<char, int> max_letter_counts;

 private:
  static std::map<char, int> count_letters(const std::string);

  // Feedback from checking a guess
  std::string word_;
  char greens_[NUM_LETTERS] = {0, 0, 0, 0, 0};
  char yellows_[NUM_LETTERS] = {0, 0, 0, 0, 0};
  char greys_[NUM_LETTERS] = {0, 0, 0, 0, 0};

  uint64_t id_string_ = 0;
};

/**
 * Public implementations
 */

void Guess::check(const std::string solution) {
  return check(solution, true);
}

void Guess::check(const std::string solution, bool infer_after) {
  auto s_count = count_letters(solution);
  //for (const auto& [k, v] : s_count) {
  //  std::cout << k << ": " << v << std::endl;
  //}

  // Place green tiles
  for (int i = 0; i < NUM_LETTERS; ++i) {
    char g = this->word_[i];
    if (g == solution[i]) {
      this->greens_[i] = g;
      --s_count[g];
    }
  }

  for (int i = 0; i < NUM_LETTERS; ++i) {
    char g = this->word_[i];
    if (this->greens_[i]) {
      continue;
    }

    // Place yellow tile only if max number not reached
    if (s_count[g]) {
      this->yellows_[i] = g;
      --s_count[g];
    } else {
      this->greys_[i] = g;
    }
  }

  this->id_string_ = id_string();
  if (infer_after) {
    infer();
  }
}

void Guess::infer() {
  // Set correct placements from greens
  for (int i = 0; i < NUM_LETTERS; ++i) {
    char c = this->greens_[i];
    if (c != 0) {
      this->correct_placements.push_back(std::pair(i, c));
    }
  }

  // Set wrong placements from yellows + greys
  for (int i = 0; i < NUM_LETTERS; ++i) {
    char y = this->yellows_[i];
    char x = this->greys_[i];
    if (y != 0) {
      this->wrong_placements.push_back(std::pair(i, y));
    }
    if (x != 0) {
      this->wrong_placements.push_back(std::pair(i, x));
    }
  }

  // Minimum letter counts from yellow + greens
  for (int i = 0; i < NUM_LETTERS; ++i) {
    char g = this->greens_[i];
    char y = this->yellows_[i];
    if (g != 0) {
      ++this->min_letter_counts[g];
    }
    if (y != 0) {
      ++this->min_letter_counts[y];
    }
  }

  // Maximum letter counts from greys
  for (int i = 0; i < NUM_LETTERS; ++i) {
    char x = this->greys_[i];
    if (x != 0) {
      this->max_letter_counts[x] = 0;
    }
  }

  auto letter_count = count_letters(this->word_);
  for (auto& [s, s_ct] : this->min_letter_counts) {
    if (s_ct < letter_count[s]) {
      // We guessed more of this letter than were in the word, so we know exact
      this->max_letter_counts[s] = s_ct;
    }
  }
}

bool Guess::operator==(const Guess& other) const {
  return this->id_string() == other.id_string();
}

uint64_t Guess::id_string() const {
  if (this->id_string_) {
    return this->id_string_;
  }

  uint64_t id = 0;
  for (int i = 0; i < NUM_LETTERS; ++i) {
    uint64_t c = this->word_[i] - 'a';
    id |= c << 7*i;

    if (this->greens_[i] != 0) {
      id |= (uint64_t) 0b10 << (7*i + NUM_LETTERS);
    } else if (this->yellows_[i] != 0) {
      id |= (uint64_t) 0b01 << (7*i + NUM_LETTERS);
    }

  }
  return id;
}

/**
 * Private implementations
 */

std::map<char, int> Guess::count_letters(const std::string word) {
  std::map<char, int> counts;
  for (char c : word) {
    ++counts[c];
  }

  return counts;
}

void Guess::print_state() const {
  std::cout << "GREENS:  [";
  for (char c : this->greens_) {
    if (c == 0) {
      std::cout << " _";
    } else {
      std::cout << " " << c;
    }
  }
  std::cout << " ]" << std::endl;

  std::cout << "YELLOWS: [";
  for (char c : this->yellows_) {
    if (c == 0) {
      std::cout << " _";
    } else {
      std::cout << " " << c;
    }
  }
  std::cout << " ]" << std::endl;

  std::cout << "GREYS:   [";
  for (char c : this->greys_) {
    if (c == 0) {
      std::cout << " _";
    } else {
      std::cout << " " << c;
    }
  }
  std::cout << " ]" << std::endl;

  std::cout << "Correct placements: ";
  for (const auto& p : this->correct_placements) {
    std::cout << "(" << p.first << "," << p.second << ") ";
  }
  std::cout << std::endl;

  std::cout << "Wrong placements:  ";
  for (const auto& p : this->wrong_placements) {
    std::cout << "(" << p.first << "," << p.second << ") ";
  }
  std::cout << std::endl;

  std::cout << "Minimum letter counts: ";
  for (const auto& [k, v] : this->min_letter_counts) {
    std::cout << "(" << k << ":" << v << ") ";
  }
  std::cout << std::endl;

  std::cout << "Maximum letter counts: ";
  for (const auto& [k, v] : this->max_letter_counts) {
    std::cout << "(" << k << ":" << v << ") ";
  }
  std::cout << std::endl;
}


std::ostream& operator<<(std::ostream& os, const Guess& guess) {
  for (int i = 0; i < NUM_LETTERS; ++i) {
    os << " ";
    if (guess.greens_[i] != 0) {
      os << GREENC;
    } else if (guess.yellows_[i] != 0) {
      os << YELLOWC;   // YELLOW
    }

    os << guess.word_[i] << ENDC;           // ENDC
  }

  return os;
}


template<> struct std::hash<Guess> {
  std::size_t operator()(const Guess& g) const noexcept {
    return std::hash<std::uint64_t>{}(g.id_string());
  }
};

#endif