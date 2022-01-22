#include "dictionary.hpp"
#include "guess.hpp"

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
std::vector<std::string> load_wordlist(std::string filename) {
  std::ifstream file(filename);

  std::vector<std::string> wordlist;
  std::string word;
  while(file >> word) {
    wordlist.emplace_back(word);
  }

  return wordlist;
}

double test_guess(const Dictionary& dict, std::string g) {
  //Guess check("rural");
  //check.check("viral");
  //std::cout << check << std::endl;

  std::unordered_map<Guess, std::pair<Guess, int>> guess_weights;
  for (std::string s : dict.reference_words) {
    Guess guess(g);
    guess.check(s, false);
    guess_weights.emplace(guess, std::pair<Guess, int>(guess, 0));
    std::pair<Guess, int>& val = guess_weights.at(guess);
    val.second++;
    guess_weights.insert(std::pair<Guess, std::pair<Guess, int>>(guess, val));

    //if (guess == check) {
    //  std::cout << guess << ": " << s << std::endl;
    //}
  }

  //std::cout << guess_weights.size() << " unique guesses." << std::endl;

  std::vector<int> sizes;
  for (auto [key, v] : guess_weights) {
    Guess guess = v.first;
    int weight = v.second;
    guess.infer();
    std::vector<bool> pruned = dict.prune(guess);
    int size = 0;
    for (bool p : pruned) {
      if (!p) {
        ++size;
      }
    }

    //assert(size == weight);

    sizes.push_back(size * weight);
  }

  double acc = 0;
  for (int size : sizes) {
    acc += size;
  }
  acc /= dict.reference_words.size();
  return acc;
}

void perf_test(std::vector<std::string> wordlist) {
  Dictionary dict(wordlist);

  std::vector<std::pair<std::string, double>> average_sizes;
  int i = 0;

  for (const auto& g : wordlist) {
    if (++i % 100 == 0) std::cout << i << ". " << g;
    double average_size = test_guess(dict, g);
    if (i % 100 == 0) std::cout << ": " << average_size << std::endl;
    average_sizes.emplace_back(g, average_size);
  }

  std::sort(average_sizes.begin(), average_sizes.end(), [](auto &a, auto &b) {
    return a.second < b.second;
  });
  
  for (const auto& [k,v] : average_sizes) {
    std::cout << k << "," << v << std::endl;
  }
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "USAGE: ./wordle_bits wordlist" << std::endl;
    return 1;
  }

  std::vector<std::string> wordlist = load_wordlist(argv[1]);

  Guess check("rural");
  check.check("viral");

  //Guess guess("rural");
  //guess.check("spray");

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