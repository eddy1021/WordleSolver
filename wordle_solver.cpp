// Author: eddy1021
#include <bits/stdc++.h>

// Game config of Wordle(https://www.powerlanguage.co.uk/wordle/)
constexpr int kLen = 5;
constexpr int kTries = 6;

// Filtered wordlist
constexpr char k5LettersWords[] = "5letters_words.txt";
constexpr char kAnswerCands[] = "5letters_distinct_words.txt";

// Show `kCandidateQuery` in case some words are not in the wordlist of Wordle.
constexpr int kCandidateQuery = 3;

// Show all the possible candidates when there are no more than
// `kShowCandidates`.
constexpr int kShowCandidates = 25;

// 3^5
constexpr int kQuinticOfThree = 243;

constexpr int kThreads = 4;

std::vector<std::string> dict;
std::vector<std::string> cand;

int base3[kLen];

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

void Init() {
  SetUp(k5LettersWords, &dict);
  SetUp(kAnswerCands, &cand);
  base3[0] = 1;
  for (int i = 1; i < kLen; ++i) {
    base3[i] = base3[i - 1] * 3;
  }

  srand(time(0));
  random_shuffle(dict.begin(), dict.end());
}

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

// Finds the current best queries which will result in minimum possible
// largest candidate groups.
// Returns top `num_cand` best queries in case some are not in the wordlist of
// Wordle.
std::vector<std::pair<int, std::string>> FindBestQuery(
    int num_cand = kCandidateQuery) {
  auto find_best = [&](int st, int ed,
                       std::vector<std::pair<int, std::string>> &cand_query) {
    for (int i = st; i < ed; ++i) {
      const auto &query = dict[i];
      bool cut_early = false;

      int groups[kQuinticOfThree] = {};
      for (const auto &c : cand) {
        int code = Guess(query, c);
        ++groups[code];

        if (cand_query.size() == num_cand and
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
    printf("Please enter the chosen query: ");
    fflush(stdout);
    std::getline(std::cin, query);
    if (query.length() != kLen) {
      printf("The query must be a word with length of 5!\n");
      continue;
    }
    bool failed = false;
    for (int i = 0; i < kLen; ++i) {
      if (!isalpha(query[i])) {
        printf("The query must contain English letter only!\n");
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
      printf("Please enter the result separated by space");
      // print out the detailed message only for the first time.
      if (first) {
        printf(
            " (0 for not in any spot, 1 for in wrong spot, 2 for in correct "
            "spot). For example, `2 2 0 0 1`");
        first = false;
      }
      printf(": ");
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
      printf("Expected %d [0,1,2] values\n", kLen);
      continue;
    }
    bool failed = false;
    for (int i = 0; i < kLen; ++i) {
      if (nums[i] < 0 or nums[i] > 2) {
        printf("Values must be 0, 1, or 2\n");
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
void FilterCand() {
  std::string queried = GetActualQuery();
  int code = GetQueriedResult();
  std::vector<std::string> left_cand;
  for (const auto &c : cand) {
    int res = Guess(queried, c);
    if (res == code) {
      left_cand.push_back(c);
    }
  }
  cand.swap(left_cand);
}

void Solve() {
  for (int turn = 0; turn < kTries; ++turn) {
    printf("Guess #%d (%d candidates left):\n", turn,
           static_cast<int>(cand.size()));
    if (cand.empty()) {
      printf("No possible word in the list matched!!!\n");
      exit(0);
    }
    if (cand.size() <= kShowCandidates) {
      printf("Candidates: ");
      for (const auto &c : cand) {
        printf("%s ", c.c_str());
      }
      printf("\n");
    }
    if (cand.size() == 1u) {
      printf("The only possible answer is: %s\n", cand[0].c_str());
      exit(0);
    }

    auto s = std::chrono::system_clock::now();
    std::vector<std::pair<int, std::string>> queries = FindBestQuery();
    auto t = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = t - s;
    printf("(spent %.3f seconds)\n", elapsed_seconds.count());

    for (const auto &query : queries) {
      printf("%s (largest groups left = %d)\n", query.second.c_str(),
             query.first);
    }
    fflush(stdout);

    FilterCand();
  }
  printf("Failed to guess the correct word within %d tries...\n", kTries);
}

int main() {
  Init();
  Solve();
}
