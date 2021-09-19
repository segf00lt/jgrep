# Notes

## Regex syntax

### Escape character
`\` Following character is literal

### Legal character literals:
All characters are legal except those which overlap with the
special characters, which must be escaped with `\`

### Character ranges:
`a-z` Lowercase alphabetical

`A-Z` Uppercase alphabetical

`0-9` Digits

`A-z` Alphabetical, upper and lowercase

### Metacharacters:
`.` Wildcard, matches any character

`?` At most once

`+` One or more

`*` Any number of times

`{n}` `n` times, where `n` is an `int >= 0`

`{n,m}` `n` through `m` times, ibid.

### Grouping operators:
`[]` Character class

`()` Capture group


## Implementation (Finite Automata)

### Description as FA or NFA

Regular expressions compile to 'Finite Automata' or finite state machines.
These machines start in some initial state s<sub>0</sub> and, given certain
inputs (which could also be conceptualized as transition states t<sub>0</sub>
through t<sub>n</sub>) it's state changes to some other state s<sub>n</sub> within a
finite set, until it eventually an end state which, in the case of regex, would
be the matching state. A Finite Automata can be described as a list of it's
states (this is key to the implementation).

### Description in pseudocode

The regular expression FA will be represented as a linked list of it's
states. Matching an expression in a string roughly consists of walking this list.

Once represented as a linked list, each node, or state, will contain some
literal character to match; while the regex operators will have been used to
indicate how the states will be linked, creating loops of different sorts for
the repetition operators, splits for alternation and direct links for
concatenation.

Therefor, in order to match our expression, we simultaneously loop over an
arbitrary string `s` and walk the list of states, comparing whatever character
of `s` we're at with the character matched by the current state. If once we've
looked at every character in `s`, the state we're at is the finish or matching
state (I.E. the last node), we have successfully matched the expression.

### Intermediate Code

Postfix notation is a way for us to represent our code which will be easier to
parse. Lets take the example of arithmetic operations. The operation `a + b`
in postfix notation (also known as Reverse Polish Notation) would be `a b +` or
`ab+`. The thing to remember is operators operate on at most 2 operands, and apart
from that just read it backwards.

A regular expression `ab|ba` in postfix notation would be written as: `ab&ba&|`.
The `&` is what I'll use to indicate concatenation (Ken Thompson used `.` in
his 1968 paper, but I want to eventually use `.` as wildcard).

Another option is Prefix notation, or "Polish notation". This is the same as
Postfix, except operators precede operands. The regex from the previous
paragraph in prefix notation would be `|&ab&ba`. Postfix may be slightly easier
to evaluate though.

**Tip**: An expression with n characters compiles to a NFA with k states, k
being equal to n minus the number of parenthesis.

#### Operators (in order of precedence)
"\*" any number of "+" one or more "?" at most one
"&" concatenation
"(" begin group
"|" alternation
")" end group

### A note on `void` pointers

GCC will allow you to do `void` pointer arithmetic, but it assumes the data
being pointed to is the size of a `char` (1 byte). This behaviour caused quite
a strange bug during the writing of the function `mk_nfa()`, which would cause
a previous element of the stack to be overwritten with garbage whenever a new
element was added with `pile()`.

Why did this happen? My stack worked fine in `postfix_fmt()`, what was the
difference now? The day after introducing this bug (and failing to fix it) I
discovered the property of `void*` I mentioned before, and realized what my
problem was. In `mk_nfa()`, the previously allocated elements of the stack were
being partially overwritten with data from the new elements, because the
`void*` pointing to the top of the stack was only being incremented by 1 byte,
and not 24 bytes (the size of my `Frag` type). This obviously wouldn't affect
the stack of `char`s from `postfix_fmt()` because the pointer was being
"conveniently" incremented by the correct amount.

So how did I fix it? Simple, add yet another special function just for properly
incrementing or decrementing that pointer and pass it's address where I needed it.

This wasn't perfect though, as it required me to pass yet another parameter to my
`pile()` and `pop()` functions, so instead I added a pointer to the special function
as a member of my stack structure, known as `void(*mvtop)(struct Stack*, int)`, which
is only a mouthful to declare while calling it is as simple as saying
`stack.mvtop(<params>)`. (I also added a pointer to the `assign()` function to the
stack structure, see `stack.c` for more details).

This new solution not only made the code run, but made all the calls to
`peek()`, `pile()` and `pop()` way shorter.

Another way would've been to add a member to the stack for storing the size of
the type the stack holds, allowing one to properly increment or decrement the
pointer. However, because `void` pointer arithmetic is prohibited by the C
standard, and made illegal by all compilers which adhere to it, this wouldn't
be a very portable solution.
