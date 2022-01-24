// Author: eddy1021
#include <bits/stdc++.h>

// Game config of Wordle(https://www.powerlanguage.co.uk/wordle/)
constexpr int kLen = 5;
constexpr int kTries = 6;

// Filtered wordlist
constexpr char k5LettersWords[] = "5letters_words.txt";

// Show `kCandidateQuery` in case some words are not in the wordlist of Wordle.
constexpr int kCandidateQuery = 3;

// Show all the possible candidates when there are no more than
// `kShowCandidates`.
constexpr int kShowCandidates = 25;

// 3^5
constexpr int kQuinticOfThree = 243;

constexpr int kThreads = 4;

constexpr bool kHardMode = true;

std::vector<std::string> dict;
std::vector<std::string> cand;

int base3[kLen];

#ifdef TEST
std::string test_answer;
#endif  // TEST

template <typename... Args>
void Printf(const char *format, Args... args) {
#ifndef TEST
  printf(format, args...);
#endif
}

// Sets up the list of words for candidate queries and answers.
void SetUp(const char word_file[], std::vector<std::string> *vec) {
  FILE *in = fopen(word_file, "r");

  char buf[256];
  while (fscanf(in, "%s", buf) == 1) {
    assert(strlen(buf) == kLen);

    std::string str;
    for (int i = 0; i < kLen; ++i) {
      str += buf[i];
    }

    vec->push_back(str);
  }
}

void Prepare() {
  SetUp(k5LettersWords, &dict);
  base3[0] = 1;
  for (int i = 1; i < kLen; ++i) {
    base3[i] = base3[i - 1] * 3;
  }

  srand(time(0));
  random_shuffle(dict.begin(), dict.end());
}

void Init() { SetUp(k5LettersWords, &cand); }

// Returns the encoded results if we guess `guess` and the answer is `answer`.
int Guess(const std::string &guess, const std::string &answer) {
  int encode = 0;
  bool used[kLen] = {};
  for (int i = 0; i < kLen; ++i) {
    if (guess[i] == answer[i]) {
      encode += 2 * base3[i];
      used[i] = true;
    }
  }
  int has = 0;
  for (int i = 0; i < kLen; ++i) {
    if (!used[i]) {
      has |= 1 << (answer[i] - 'a');
    }
  }
  for (int i = 0; i < kLen; ++i) {
    if (guess[i] == answer[i]) {
      continue;
    }
    if ((has >> (guess[i] - 'a')) & 1) {
      has &= ~(1 << (guess[i] - 'a'));
      encode += base3[i];
    }
  }
  return encode;
}

// contains the information of a query and the corresponding result.
struct Hint {
  std::string query;
  int result;
};

// Returns true if `query` is a valid query in hard mode given the `hints`.
bool Valid(const std::vector<Hint> &hints, const std::string &query) {
  for (const auto &hint : hints) {
    int has = 0, matched = 0;
    int code = hint.result;
    for (int i = 0; i < kLen; ++i) {
      if (code % 3 == 2) {
        if (query[i] != hint.query[i]) {
          return false;
        } else {
          matched |= (1 << i);
        }
      } else if (code % 3 == 1) {
        has |= (1 << (hint.query[i] - 'a'));
      }
      code /= 3;
    }
    for (int i = 0; i < kLen; ++i) {
      if ((matched >> i) & 1) {
        continue;
      }
      if ((has >> (query[i] - 'a')) & 1) {
        has &= ~(1 << (query[i] - 'a'));
      }
    }
    if (has) {
      return false;
    }
  }
  return true;
}

// Finds the current best queries which will result in minimum possible
// largest candidate groups.
// Returns top `num_cand` best queries in case some are not in the wordlist of
// Wordle.
std::vector<std::pair<int, std::string>> FindBestQuery(
    const std::vector<Hint> &hints, int num_cand = kCandidateQuery) {
  auto find_best = [&](int st, int ed,
                       std::vector<std::pair<int, std::string>> &cand_query) {
    for (int i = st; i < ed; ++i) {
      const auto &query = dict[i];
      if (kHardMode and not Valid(hints, query)) {
        continue;
      }

      bool cut_early = false;

      int groups[kQuinticOfThree] = {};
      for (const auto &c : cand) {
        int code = Guess(query, c);
        ++groups[code];

        if (static_cast<int>(cand_query.size()) == num_cand and
            groups[code] > cand_query.back().first) {
          cut_early = true;
          break;
        }
      }
      if (cut_early) {
        continue;
      }
      cand_query.push_back(std::make_pair(
          *std::max_element(groups, groups + kQuinticOfThree), query));
      std::sort(cand_query.begin(), cand_query.end());
      if (static_cast<int>(cand_query.size()) > num_cand) {
        cand_query.resize(num_cand);
      }
    }
  };
  std::vector<std::pair<int, std::string>> cand_query;
#ifdef MULTI_THREAD
  std::vector<std::pair<int, std::string>> cands[kThreads];
  std::vector<std::thread> threads;
  for (int i = 0, st = 0; i < kThreads; ++i) {
    int ed = dict.size() * (i + 1) / kThreads;
    threads.emplace_back(find_best, st, ed, std::ref(cands[i]));
    st = ed;
  }
  for (int i = 0; i < kThreads; ++i) {
    threads[i].join();
    for (const auto &c : cands[i]) {
      cand_query.push_back(c);
    }
  }
  std::sort(cand_query.begin(), cand_query.end());
  if (static_cast<int>(cand_query.size()) > num_cand) {
    cand_query.resize(num_cand);
  }
#else
  find_best(0, dict.size(), cand_query);
#endif
  return cand_query;
}

// Gets the actual query player choose.
std::string GetActualQuery() {
  std::string query;
  while (true) {
    Printf("Please enter the chosen query: ");
    fflush(stdout);
    std::getline(std::cin, query);
    if (query.length() != kLen) {
      Printf("The query must be a word with length of 5!\n");
      continue;
    }
    bool failed = false;
    for (int i = 0; i < kLen; ++i) {
      if (!isalpha(query[i])) {
        Printf("The query must contain English letter only!\n");
        failed = true;
        break;
      }
      if (isupper(query[i])) {
        query[i] = query[i] - 'A' + 'a';
      }
    }
    if (failed) {
      continue;
    }
    break;
  }
  return query;
}

// Gets the results Wordle responds.
int GetQueriedResult() {
  int encode = 0;
  while (true) {
    // print out the message to prompt the user input the response from Wordle.
    auto print_message = []() {
      static bool first = true;
      Printf("Please enter the result separated by space");
      // print out the detailed message only for the first time.
      if (first) {
        Printf(
            " (0 for not in any spot, 1 for in wrong spot, 2 for in correct "
            "spot). For example, `2 2 0 0 1`");
        first = false;
      }
      Printf(": ");
    };
    print_message();
    fflush(stdout);

    std::string line;
    std::getline(std::cin, line);
    std::stringstream ss(line);

    std::vector<int> nums;
    int val;
    while (ss >> val) {
      nums.push_back(val);
    }

    if (nums.size() != kLen) {
      Printf("Expected %d [0,1,2] values\n", kLen);
      continue;
    }
    bool failed = false;
    for (int i = 0; i < kLen; ++i) {
      if (nums[i] < 0 or nums[i] > 2) {
        Printf("Values must be 0, 1, or 2\n");
        failed = true;
        break;
      }
    }
    if (failed) {
      continue;
    }
    encode = 0;
    for (int i = 0; i < kLen; ++i) {
      encode += base3[i] * nums[i];
    }
    break;
  }
  return encode;
}

// Filters out the possible candidates left.
void FilterCand(std::string queried, int code) {
  std::vector<std::string> left_cand;
  for (const auto &c : cand) {
    int res = Guess(queried, c);
    if (res == code) {
      left_cand.push_back(c);
    }
  }
  cand.swap(left_cand);
}

int Solve(int max_tries = kTries) {
  std::vector<Hint> hints;

  for (int turn = 0; turn < max_tries; ++turn) {
    Printf("Guess #%d (%d candidates left):\n", turn,
           static_cast<int>(cand.size()));
    if (cand.empty()) {
      Printf("No possible word in the list matched!!!\n");
      return -2;
    }
    if (cand.size() <= kShowCandidates) {
      Printf("Candidates: ");
      for (const auto &c : cand) {
        Printf("%s ", c.c_str());
      }
      Printf("\n");
    }
    if (cand.size() == 1u) {
      Printf("The only possible answer is: %s\n", cand[0].c_str());
      return turn + 1;
    }

    auto s = std::chrono::system_clock::now();
    std::vector<std::pair<int, std::string>> queries = FindBestQuery(hints);
    auto t = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = t - s;
    Printf("(spent %.3f seconds)\n", elapsed_seconds.count());

    for (const auto &query : queries) {
      Printf("%s (largest groups left = %d)\n", query.second.c_str(),
             query.first);
    }
    fflush(stdout);

#ifdef TEST
    std::string queried = queries[0].second;
    int code = Guess(queried, test_answer);
#else
    std::string queried = GetActualQuery();
    int code = GetQueriedResult();
#endif
    hints.push_back({queried, code});

    if (code == kQuinticOfThree - 1) {
      return turn + 1;
    }
    FilterCand(queried, code);
  }
  Printf("Failed to guess the correct word within %d tries...\n", kTries);
  return -1;
}

int main() {
  Prepare();
#ifdef TEST
  std::vector<std::string> all_answers;
  SetUp(k5LettersWords, &all_answers);
  std::random_shuffle(all_answers.begin(), all_answers.end());

  constexpr int kTest = 100;
  int tries[kTries + 1] = {};
  for (int i = 0; i < kTest; ++i) {
    test_answer = all_answers[i];
    printf("Trying #%d (answer = %s): ", i, test_answer.c_str());
    Init();
    int ret = Solve();
    if (ret == -1) {
      ++tries[0];
      printf("Failed to find out in %d tries\n", kTries);
    } else if (ret == -2) {
      assert(false);
    } else {
      assert(1 <= ret and ret <= kTries);
      ++tries[ret];
      printf("%d tries\n", ret);
    }
  }
  for (int i = 1; i <= kTries; ++i) {
    printf("Tries %d: %d\n", i, tries[i]);
  }
  printf("Failed to found in %d tries: %d\n", kTries, tries[0]);
#else
  Init();
  Solve();
#endif
}
