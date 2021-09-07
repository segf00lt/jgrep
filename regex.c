#include <stdio.h>

/*
 * Escape character:
 * 	\
 *
 * Repetition operators:
 * 	? At most once
 * 	+ One or more
 * 	* Any number of times
 * 	{n} n times, where n is an int >= 0
 * 	{n, m} n through m times, ibid.
 *
 * Legal character literals:
 * 	All characters are legal except those which overlap with the
 * 	special characters, which must be escaped with '\'
 *
 * Character ranges:
 * 	a-z Lowercase alphabeticals
 * 	A-Z Uppercase alphabeticals
 * 	0-9 Digits
 * 	A-z Alphabeticals, upper and lowercase
 *
 * Metacharacters:
 * 	. Wildcard, matches any character
 *
 * Grouping operators:
 * 	[] Character class
 * 	() Capture group
 */

int main(int argc, char* argv[]) {

	return 0;
}
