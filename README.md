# WordleSolver

A solver to solve the daily puzzle from Wordle

## Usage

```console
$ g++ -O2 wordle_solver.cpp
$ ./a.out
```

### Note

It will take about several seconds for the first query.

You can replace the wordlist of `5letters_words.txt` and
`5letters_distinct_words.txt`. The former one will be taken as all possible
queries and the latter one will be used as all possible answers. Both wordlist
must contain 5 letters words in lowercase only.

Solving Wordle puzzle without this solver is much more interesting!
