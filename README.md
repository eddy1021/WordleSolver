# WordleSolver

A solver to solve the daily puzzle from Wordle

## Usage

```console
$ g++ -O2 wordle_solver.cpp
$ ./a.out

# Multi thread
$ g++ -O2 wordle_solver.cpp -pthread -DMULTI_THREAD
$ ./a.out
```

### Note

It will take about several seconds for the first query.

You can replace the wordlist of `5letters_words.txt` and
`5letters_distinct_words.txt`. The former one will be taken as all possible
queries and the latter one will be used as all possible answers. Both wordlist
must contain 5 letters words in lowercase only.

## Distribution of guesses

By using this solver, following is the distribution of number of tries needed
for each answer in `5letters_distinct_words.txt`:

```
1 try: 1
2 tries: 19
3 tries: 1387
4 tries: 7287
5 tries: 4832
6 tries: 144
```

All word can be found in 6 tries.

This distribution could be reproduced by(would take more than an hour):

```console
$ g++ -O2 wordle_solver.cpp -DTEST
$ ./a.out
```

Solving Wordle puzzle without this solver is much more interesting!
