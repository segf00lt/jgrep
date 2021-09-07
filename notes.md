# Notes

## Regex syntax

### Escape character
`\` Following character is literal

### Repetition operators:
`?` At most once

`+` One or more

`*` Any number of times

`{n}` `n` times, where `n` is an `int >= 0`

`{n,m}` `n` through `m` times, ibid.

### Legal character literals:
All characters are legal except those which overlap with the
special characters, which must be escaped with `\`

### Character ranges:
`a-z` Lowercase alphabeticals

`A-Z` Uppercase alphabeticals

`0-9` Digits

`A-z` Alphabeticals, upper and lowercase

### Metacharacters:
`.` Wildcard, matches any character

### Grouping operators:
`[]` Character class

`()` Capture group


## Compiled regex (Finite Automata)

Regular expressions compile to 'Finite Automata' or finite state machines.
These machines start in some initial state s<sub>0</sub> and, given certain
inputs (which could also be conceptualized as transtion states t<sub>0</sub>
through t<sub>n</sub>) it's state changes to some other state sn within a
finite set, until it eventually an end state which, in the case of regex, would
be the matching state.
