#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "const.h"

typedef struct {
	char* data;
	char* top;
	int height;
	int capacity;
} Stack;

char peek(Stack* s) { return *(s->top); }

int pile(Stack* s, char c) {
	size_t height = s->height;
	if(height == s->capacity - 1)
		return 1;
	else if(height)
		*(++(s->top)) = c;
	else
		*(s->top) = c;

	++(s->height);
	return 0;
}

char pop(Stack* s) {
	char c = *(s->top);
	*(s->top) = 0;
	if(s->top > s->data) {
		--s->top;
		--(s->height);
	} else
		s->height = 0;
	return c;
}

char* postfix_fmt(char* exp) {
	size_t l_exp = strlen(exp);

	Stack stack;
	stack.height = 0;
	stack.capacity = l_exp + 1;
	stack.data = (char*)calloc(stack.capacity, CHARSIZE);
	stack.top = stack.data;

	char* buf = (char*)calloc((l_exp * 2) + 1, CHARSIZE);
	int i = 0;

	int n_atoms = 0;

	// store values of n_atoms prior to entering a group
	int* prev_atoms = (int*)calloc(l_exp + 1, CHARSIZE);
	int j = 0;

	inline void cleanup() {
		free(prev_atoms);
		free(buf);
		free(stack.data);
	}

	for(char* c = exp; *c; ++c) {
		switch(*c) {
			case '(':
				if(n_atoms > 1) {
					buf[i++] = '&';
					--n_atoms;
				}

				prev_atoms[j++] = n_atoms;

				n_atoms = 0;

				pile(&stack, *c);
				break;

			case '|':
				if(!n_atoms)
					return NULL;

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = '&';

				for(char top = peek(&stack); top != 0 && top != '('; top = peek(&stack))
					buf[i++] = pop(&stack);

				pile(&stack, *c);
				break;

			case ')':
				if(j == 0 || n_atoms == 0) {
					fprintf(stderr, "error: mismatched or empty parentheses\n");
					cleanup();
					return NULL;
				}

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = '&';

				for(char top = peek(&stack); top != 0 && top != '('; top = peek(&stack))
					buf[i++] = pop(&stack);

				pop(&stack);

				// remember to decrement the position in prev_atoms
				n_atoms = prev_atoms[--j] + 1;
				break;

			case '*':
				if(!n_atoms)
					return NULL;
				buf[i++] = *c;
				break;

			case '+':
				if(!n_atoms)
					return NULL;
				buf[i++] = *c;
				break;

			case '?':
				if(!n_atoms)
					return NULL;
				buf[i++] = *c;
				break;

			default:
				if(n_atoms > 1) {
					--n_atoms;
					buf[i++] = '&';
				}

				buf[i++] = *c;
				++n_atoms;
				break;
		}
	}

	if(j != 0) {
		fprintf(stderr, "error: mismatched parentheses\n");
		cleanup();
		return NULL;
	}

	for(--n_atoms; n_atoms > 0; --n_atoms)
		buf[i++] = '&';

	while(stack.height > 0)
		buf[i++] = pop(&stack);

	// remember to free post when done
	char* post = (char*)calloc((strlen(buf) + 1), CHARSIZE);
	strcpy(post, buf);

	cleanup();
	return post;
}

int main(int argc, char* argv[]) {
	char* post = postfix_fmt(argv[1]);
	printf("%s\n", post ? post : "");
	free(post);

	return 0;
}
