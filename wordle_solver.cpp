// Author: eddy1021
#include <bits/stdc++.h>

// Game config of WORDLE(https://www.powerlanguage.co.uk/wordle/)
constexpr int kLen = 5;
constexpr int kTries = 6;

// Filtered wordlist
constexpr char k5LettersWords[] = "5letters_words.txt";
constexpr char kAnswerCands[] = "5letters_distinct_words.txt";

// Show `kCandidateQuery` in case some are words are not in the wordlist of
// wordle.
constexpr int kCandidateQuery = 3;

// Show all the possible candidates when there are no more than
// `kShowCandidates`.
constexpr int kShowCandidates = 25;

// 3^5
constexpr int kQuinticOfThree = 243;

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
  for (int i = 0; i < kLen; ++i) {
    for (int j = 0; j < kLen; ++j) {
      if (used[j]) {
        continue;
      }
      if (guess[i] == answer[j]) {
        encode += base3[i];
        used[j] = true;
        break;
      }
    }
  }
  return encode;
}

// Finds the current best queries which will result in minimum possible
// largest candidate groups.
// Returns top `num_cand` best queries in case some are not in the wordlist of
// WORDLE.
std::vector<std::pair<int, std::string>> FindBestQuery(
    int num_cand = kCandidateQuery) {
  std::vector<std::pair<int, std::string>> cand_query;
  for (const auto &query : dict) {
    int groups[kQuinticOfThree] = {};
    for (const auto &c : cand) {
      ++groups[Guess(query, c)];
    }
    cand_query.push_back(std::make_pair(
        *std::max_element(groups, groups + kQuinticOfThree), query));
    std::sort(cand_query.begin(), cand_query.end());
    if (static_cast<int>(cand_query.size()) > num_cand) {
      cand_query.resize(num_cand);
    }
  }
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

// Gets the results WORDLE responds.
int GetQueriedResult() {
  int encode = 0;
  while (true) {
    printf(
        "Please enter the result separated by space (0 for not in any spot, 1 "
        "for in wrong spot, 2 for in correct spot). For example, `2 2 0 0 "
        "1`: ");
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

    std::vector<std::pair<int, std::string>> queries = FindBestQuery();
    for (const auto &query : queries) {
      printf("%s (largest groups left = %d)\n", query.second.c_str(),
             query.first);
    }
    fflush(stdout);

    FilterCand();
  }
}

int main() {
  Init();
  Solve();
}
