#include "dictionary.hpp"
#include "guess.hpp"
#include "word.hpp"

#include <assert.h>

#include <bitset>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
 * Load wordlist from file.
 */
std::vector<Word> load_wordlist(std::string filename) {
  std::ifstream file(filename);

  std::vector<Word> wordlist;
  std::string text;
  while(file >> text) {
    wordlist.emplace_back(text);
  }

  return wordlist;
}

double test_guess(const Dictionary& dict, Word g) {
  std::unordered_map<Guess, int> guess_weights;
  for (const Word& s : dict.all_words) {
    Guess guess(&g, &s);
    guess_weights.emplace(guess, 0);
    guess_weights[guess]++;
  }

  // std::cout << guess_weights.size() << " unique guesses." << std::endl;

  std::vector<int> sizes;
  for (auto& [key, weight] : guess_weights) {
    Guess guess = key;
    guess.infer();
    std::vector<bool> pruned = dict.prune(guess);
    int size = 0;
    for (bool p : pruned) {
      if (!p) {
        ++size;
      }
    }

    sizes.push_back(size * weight);
  }

  double acc = 0;
  for (int size : sizes) {
    acc += size;
  }
  acc /= dict.size();
  return acc;
}

void perf_test(const std::vector<Word>& wordlist) {
  Dictionary dict(wordlist);

  std::vector<std::pair<const Word*, double>> average_sizes;
  int i = 0;

  for (const auto& g : wordlist) {
    if (++i % 100 == 0) std::cout << i << ". " << g;
    double average_size = test_guess(dict, g);
    if (i % 100 == 0) std::cout << ": " << average_size << std::endl;
    average_sizes.emplace_back(&g, average_size);
  }

  std::sort(average_sizes.begin(), average_sizes.end(), [](auto &a, auto &b) {
    return a.second < b.second;
  });
  
  for (const auto& [k,v] : average_sizes) {
    std::cout << *k << "," << v << std::endl;
  }
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "USAGE: ./wordle_bits wordlist" << std::endl;
    return 1;
  }

  std::vector<Word> wordlist = load_wordlist(argv[1]);

  Word rural("rural");
  Word viral("viral");
  Guess check(&rural, &viral);

  std::cout << check << std::endl;
  //check.print_state();
  //std::cout << std::bitset<64>(check.id_string()) << std::endl;

  //std::cout << guess << std::endl;
  //guess.print_state();
  //std::cout << std::bitset<64>(guess.id_string()) << std::endl;

  //std::cout << (guess == check) << std::endl;

  //Dictionary dict(wordlist);

  //std::cout << test_guess(dict, "roate") << std::endl;
  perf_test(wordlist);

  //std::cout << std::bitset<64>(encoded_count) << std::endl;

  return 0;
}