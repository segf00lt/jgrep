#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	char* data;
	int height;
	int capacity;
	char* top;
} Stack;

char peek(Stack* s) { return *(s->top); }

void pile(Stack* s, char c) {
	size_t height = s->height;
	if(height == s->capacity - 1)
		return;
	else if(height)
		*(++(s->top)) = c;
	else
		*(s->top) = c;

	++(s->height);
}

char pop(Stack* s) {
	char c = *(s->top);
	*(s->top) = 0;
	if(s->top > s->data)
		--s->top;
	--(s->height);
	return c;
}

char* postfix_fmt(char* exp) {
	size_t l_exp = strlen(exp);

	Stack stack;
	stack.height = 0;
	stack.capacity = (l_exp * 2) + 1;
	stack.data = (char*)calloc(stack.capacity, sizeof(char));
	stack.top = stack.data;

	char* buf = (char*)calloc((l_exp * 2) + 1, sizeof(char));
	int i = 0;

	int n_atoms = 0;

	// store values of n_atoms prior to entering a group
	int* prev_n_atoms = (int*)calloc(l_exp + 1, sizeof(char));
	int j = 0;

	char* c = NULL;
	for(c = exp; *c; c++) {
		switch(*c) {
			case '(':
				if(n_atoms > 1) {
					buf[i++] = '&';
					--n_atoms;
				}

				prev_n_atoms[j] = n_atoms;
				++j;

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
				if(j == 0 || n_atoms == 0)
					return NULL;

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = '&';

				for(char top = peek(&stack); top != 0 && top != '('; top = peek(&stack))
					buf[i++] = pop(&stack);

				if(!stack.height) {
					fprintf(stderr, "error: mismatched parentheses\n");
					return NULL;
				}

				pop(&stack);

				// remember to decrement the position in prev_n_atoms
				--j;
				n_atoms = prev_n_atoms[j] + 1;
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

	if(j != 0)
		return NULL;
	free(prev_n_atoms);

	for(--n_atoms; n_atoms > 0; --n_atoms)
		buf[i++] = '&';

	while(stack.height > 0) {
		if(*(stack.top) == '(') {
			fprintf(stderr, "error: mismatched parentheses\n");
			return NULL;
		}
		buf[i++] = pop(&stack);
	}

	free(stack.data);
	// remember to free post when done
	char* post = (char*)calloc((strlen(buf) + 1), sizeof(char));
	strcpy(post, buf);

	free(buf);
	return post;
}

int main(int argc, char* argv[]) {
	char* post = postfix_fmt(argv[1]);
	printf("%s\n", post);
	free(post);

	return 0;
}
