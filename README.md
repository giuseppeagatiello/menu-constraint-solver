# Menu Constraint Solver

This is my C project for the API course at Politecnico di Milano. The idea is simple: given a list of dishes and the preferences of the people attending a company dinner, can we choose a menu that satisfies everyone?

Each person lists alternatives they would accept, such as including a dish or leaving one out. At least one preference must hold for each person. In logical terms, the preferences on a single line are joined by OR, and the lines are joined by AND.

## A small example

The first line lists the available dishes. Each following line contains one person's preferences. A minus sign means that the dish should be left out.

```text
pasta soup
pasta soup
-pasta
```

The first person accepts pasta or soup. The second wants pasta left out. Choosing soup and leaving out pasta satisfies both, so the program prints:

```text
OK
```

The output reports whether a solution exists; it does not list the chosen dishes. If the preferences conflict, the program prints `KO`, then tries again after removing people from the end of the input. Lines such as `-1` and `-2` show how many people have been removed so far.

## Build and run

You'll need a C compiler with C11 support, such as GCC or Clang. Open a terminal in the `menu-constraint-solver` folder and run:

```sh
mkdir -p build
cc -std=c11 -O2 -Wall -Wextra src/main.c -o build/menu_solver
./build/menu_solver < tests/compatible_constraints.txt
```

The `<` passes the contents of the `.txt` file to the program as input. To try another case, replace the filename:

```sh
./build/menu_solver < tests/remove_last_employee.txt
```

You can also write your own `.txt` file using the format above.

## Test files

Each case has two files: `name.txt` contains the input, and `name.expected.txt` contains the expected output. Pass the input file to the program; use the expected file to check the result.

| Input file | What it checks |
|---|---|
| `tests/compatible_constraints.txt` | Preferences that can all be satisfied. |
| `tests/remove_last_employee.txt` | Conflicting preferences resolved by removing the last person. |
| `tests/branching_unsatisfiable.txt` | A conflict that requires branching to detect. |
| `tests/remove_three_employees.txt` | Several removals in a row; currently exposes a known bug. |

To compare an output with the expected result:

```sh
./build/menu_solver < tests/compatible_constraints.txt > build/compatible_constraints.actual.txt
diff -u tests/compatible_constraints.expected.txt build/compatible_constraints.actual.txt
```

If `diff` prints nothing, the files match. These are small checks added to this repository, not the official course tests. Three currently pass; `remove_three_employees.txt` fails because the removal loop stops too early.

## How it works

The solver keeps track of dishes as selected, excluded or undecided. If a person's preferences leave only one possible choice, it makes that choice immediately. Otherwise, it picks an undecided dish, tries one value and backtracks if it reaches a conflict.

The main implementation choices are:

- A hash table with linear probing to turn dish names into numeric IDs.
- Dynamic arrays for dishes, people, preferences and occurrences.
- An occurrence index and a circular queue to revisit affected preferences during propagation.
- Branching from an unresolved constraint with the fewest undecided preferences.
- An assignment trail to undo changes when a branch fails.

## Files

```text
src/main.c                  Solver source code
tests/<name>.txt            Test inputs
tests/<name>.expected.txt   Expected outputs
docs/verification.md        Test results and the known removal-loop bug
```
