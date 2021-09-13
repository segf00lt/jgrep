#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "const.h"
#include "stack.h"

void assign_char(void* dest, void* src) {
	*((char*)dest) = *((char*)src);
}

char* postfix_fmt(char* exp) {
	size_t l_exp = strlen(exp);

	Stack stack = nStack(l_exp + 1, CHARSIZE);

	char* buf = (char*)calloc((l_exp * 2) + 1, CHARSIZE);
	int i = 0;

	int n_atoms = 0;

	// store values of n_atoms prior to entering a group
	int* prev_atoms = (int*)calloc(l_exp + 1, CHARSIZE);
	int j = 0;

	inline void cleanup() {
		free(prev_atoms);
		free(buf);
		dStack(&stack);
	}

	for(char* c = exp; *c; ++c) {
		char tmp = 0;
		void* p = (void*)(&tmp);

		switch(*c) {
			case '(':
				if(n_atoms > 1) {
					buf[i++] = '&';
					--n_atoms;
				}

				prev_atoms[j++] = n_atoms;

				n_atoms = 0;

				pile(&stack, (void*)c, assign_char);
				break;

			case '|':
				if(!n_atoms)
					return NULL;

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = '&';

				for(peek(&stack, p, assign_char);
					tmp != 0 && tmp != '(';
					peek(&stack, p, assign_char))
				{
					pop(&stack, (void*)(buf + (i++)), assign_char);
				}
				tmp = 0;

				pile(&stack, (void*)c, assign_char);
				break;

			case ')':
				if(j == 0 || n_atoms == 0) {
					fprintf(stderr, "error: mismatched or empty parentheses\n");
					cleanup();
					return NULL;
				}

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = '&';

				for(peek(&stack, p, assign_char);
					tmp != 0 && tmp != '(';
					peek(&stack, p, assign_char))
				{
					pop(&stack, (void*)(buf + (i++)), assign_char);
				}
				tmp = 0;

				pop(&stack, NULL, assign_char);

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
		// remeber to set tmp to 0
		tmp = 0;
	}

	if(j != 0) {
		fprintf(stderr, "error: mismatched parentheses\n");
		cleanup();
		return NULL;
	}

	for(--n_atoms; n_atoms > 0; --n_atoms)
		buf[i++] = '&';

	while(stack.size > 0)
		pop(&stack, (void*)(buf + (i++)), assign_char);

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
