#ifndef GUESS_H
#define GUESS_H

#include "constants.hpp"
#include "word.hpp"

#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

class Guess {
 public:
  Guess(const Word* guess, const Word* solution)
    : guess_(guess), solution_(solution) {
      compare();
    }

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
  void compare();

  const Word* guess_;
  const Word* solution_;
  char greens_[NUM_LETTERS] = {0, 0, 0, 0, 0};
  char yellows_[NUM_LETTERS] = {0, 0, 0, 0, 0};
  char greys_[NUM_LETTERS] = {0, 0, 0, 0, 0};

  uint64_t id_string_ = 0;
};

/**
 * Public implementations
 */
void Guess::compare() {
  auto s_count = solution_->letter_counts();

  // Place green tiles
  for (int i = 0; i < NUM_LETTERS; ++i) {
    char g = guess_->letter(i);
    if (g == solution_->letter(i)) {
      greens_[i] = g;
      --s_count[g];
    }
  }

  for (int i = 0; i < NUM_LETTERS; ++i) {
    char g = guess_->letter(i);
    if (greens_[i]) {
      continue;
    }

    // Place yellow tile only if max number not reached
    if (s_count[g]) {
      yellows_[i] = g;
      --s_count[g];
    } else {
      greys_[i] = g;
    }
  }

  id_string_ = id_string();
}

void Guess::infer() {
  // Set correct placements from greens
  for (int i = 0; i < NUM_LETTERS; ++i) {
    char c = greens_[i];
    if (c != 0) {
      correct_placements.emplace_back(i, c);
    }
  }

  // Set wrong placements from yellows + greys
  for (int i = 0; i < NUM_LETTERS; ++i) {
    char y = yellows_[i];
    char x = greys_[i];
    if (y != 0) {
      wrong_placements.emplace_back(i, y);
    }
    if (x != 0) {
      wrong_placements.emplace_back(i, x);
    }
  }

  // Minimum letter counts from yellow + greens
  for (int i = 0; i < NUM_LETTERS; ++i) {
    char g = greens_[i];
    char y = yellows_[i];
    if (g != 0) {
      ++min_letter_counts[g];
    }
    if (y != 0) {
      ++min_letter_counts[y];
    }
  }

  // Maximum letter counts from greys
  for (int i = 0; i < NUM_LETTERS; ++i) {
    char x = greys_[i];
    if (x != 0) {
      max_letter_counts[x] = 0;
    }
  }

  auto& letter_count = guess_->letter_counts();
  for (auto& [s, s_ct] : min_letter_counts) {
    if (s_ct < letter_count.at(s)) {
      // We guessed more of this letter than were in the word, so we know exact
      max_letter_counts[s] = s_ct;
    }
  }
}

bool Guess::operator==(const Guess& other) const {
  return id_string() == other.id_string();
}

uint64_t Guess::id_string() const {
  if (id_string_) {
    return id_string_;
  }

  uint64_t id = 0;
  for (int i = 0; i < NUM_LETTERS; ++i) {
    uint64_t c = guess_->letter(i) - 'a';
    id |= c << 7*i;

    if (greens_[i] != 0) {
      id |= (uint64_t) 0b10 << (7*i + NUM_LETTERS);
    } else if (yellows_[i] != 0) {
      id |= (uint64_t) 0b01 << (7*i + NUM_LETTERS);
    }
  }
  return id;
}

/**
 * Private implementations
 */
void Guess::print_state() const {
  std::cout << "Guessed: " << guess_->letters() << " vs " << solution_->letters() << std::endl;
  std::cout << "GREENS:  [";
  for (char c : greens_) {
    if (c == 0) {
      std::cout << " _";
    } else {
      std::cout << " " << c;
    }
  }
  std::cout << " ]" << std::endl;

  std::cout << "YELLOWS: [";
  for (char c : yellows_) {
    if (c == 0) {
      std::cout << " _";
    } else {
      std::cout << " " << c;
    }
  }
  std::cout << " ]" << std::endl;

  std::cout << "GREYS:   [";
  for (char c : greys_) {
    if (c == 0) {
      std::cout << " _";
    } else {
      std::cout << " " << c;
    }
  }
  std::cout << " ]" << std::endl;

  std::cout << "Correct placements: ";
  for (const auto& p : correct_placements) {
    std::cout << "(" << p.first << "," << p.second << ") ";
  }
  std::cout << std::endl;

  std::cout << "Wrong placements:  ";
  for (const auto& p : wrong_placements) {
    std::cout << "(" << p.first << "," << p.second << ") ";
  }
  std::cout << std::endl;

  std::cout << "Minimum letter counts: ";
  for (const auto& [k, v] : min_letter_counts) {
    std::cout << "(" << k << ":" << v << ") ";
  }
  std::cout << std::endl;

  std::cout << "Maximum letter counts: ";
  for (const auto& [k, v] : max_letter_counts) {
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

    os << guess.guess_->letter(i) << ENDC;           // ENDC
  }

  return os;
}


template<> struct std::hash<Guess> {
  std::size_t operator()(const Guess& g) const noexcept {
    return std::hash<std::uint64_t>{}(g.id_string());
  }
};

#endif