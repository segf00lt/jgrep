#!/usr/bin/env python3

from sys import argv
from enum import Enum

class OPCODE(Enum):
    CHAR = 1
    WILD = 2
    JUMP = 3
    BACK = 4
    ALT = 5
    CLASS = 6
    BLINE = 130
    ELINE = 140
    MATCH = 11

def parse(exp) -> str:
    l_exp = len(exp)
    stack = []
    post = ""
    n_atoms = 0
    prev_atoms = []
    baseop = '&'

    i = 0
    while(i in range(l_exp)):
        c = exp[i]

        if c == '(':
            if baseop != '&':
                print("error: group operator within class")
                quit()

            if n_atoms > 1:
                post += baseop
                n_atoms -= 1

            prev_atoms.append(n_atoms)
            n_atoms = 0
            stack.append(c)
            i += 1

        elif c == '|':
            if n_atoms == 0:
                quit()

            n_atoms -= 1
            while(n_atoms > 0):
                post += baseop
                n_atoms -= 1

            last = len(stack) - 1
            while(last > -1 and stack[last] != '('):
                post += stack.pop()
                last -= 1

            stack.append(c)
            i += 1

        elif c == ')':
            if baseop != '&':
                print("error: group operator within class")
                quit()

            if len(prev_atoms) == 0 or n_atoms == 0:
                print("error: mismatched or empty parentheses")
                quit()

            last = len(stack) - 1

            n_atoms -= 1
            while(n_atoms > 0):
                post += baseop
                n_atoms -= 1

            while(last > -1 and stack[last] != '('):
                post += stack.pop()
                last -= 1

            if last < 0:
                print("error: mismatched parentheses")
                quit()

            stack.pop()
            n_atoms = prev_atoms.pop() + 1
            i += 1

        elif c == '[':
            if baseop != '&':
                print("error: mismatched square brackets")
                quit()

            if n_atoms > 1:
                n_atoms -= 1
                post += baseop

            post += c
            prev_atoms.append(n_atoms)
            n_atoms = 0
            baseop = ''
            i += 1

        elif c == '-':
            if baseop == '&':
                if n_atoms > 1:
                    n_atoms -= 1
                    post += baseop
                post += c
                n_atoms += 1
                i += 1
                continue

            elif n_atoms == 0:
                print("error: invalid range")
                quit()

            pc = exp[i - 1]
            nc = exp[i + 1]
            if ord(nc) <= ord(pc):
                print("error: invalid range")
                quit()

            cr = [f"\\{chr(c)}" if c >= 91 and c <= 93 else chr(c) for c in range(ord(pc) + 1, ord(nc))]
            post += "".join(cr)
            i += 1

        elif c == ']':
            if baseop == '&' or n_atoms == 0:
                print("error: mismatched and or empty square brackets")
                quit()
            post += c
            n_atoms = prev_atoms.pop() + 1
            baseop = '&'
            i += 1

        elif c == '*' or c == '+' or c == '?':
            if n_atoms == 0:
                print("error: lone repetition operator")
                quit()
            post += c
            i += 1

        elif c == '^':
            if n_atoms > 1:
                n_atoms -= 1
                post += baseop

            post += c
            i += 1

        elif c == '$':
            if n_atoms > 1:
                n_atoms -= 1
                post += baseop

            post += c
            i += 1

        elif c == '\\':
            if n_atoms > 1:
                n_atoms -= 1
                post += baseop

            if (i + 1) == l_exp:
                quit()

            nc = exp[i + 1]
            if nc in ".(|)[-]*+?&^$\\":
                post += c
                i += 1
                n_atoms += 1

            post += nc
            i += 1
            continue

        else:
            if n_atoms > 1:
                n_atoms -= 1
                post += baseop

            post += c
            n_atoms += 1
            i += 1

    if baseop != '&':
        print("error: mismatched square brackets")
        quit()

    n_atoms -= 1
    while(n_atoms > 0):
        post += baseop
        n_atoms -= 1

    last = len(stack) - 1
    while(last > -1):
        if stack[last] == '(':
            print("error: mismatched parentheses")
            quit()
        post += stack.pop()
        last -= 1

    return post

def assemble(post) -> str:
    if post == None:
        print("error: assemble: empty input string")
        quit()

    prog = ""
    stack = []
    l_post = len(post)

    i = 0
    while(i in range(l_post)):
        c = post[i]

        if c == '\\':
            i += 1
            stack.append(f"{chr(OPCODE.CHAR.value)}{post[i]}")
            i += 1
            continue

        elif c == '^':
            stack.append(f"{chr(OPCODE.BLINE.value)}")
            i += 1
            continue

        elif c == '$':
            stack.append(f"{chr(OPCODE.ELINE.value)}")
            i += 1
            continue

        elif c == '|':
            f2 = stack.pop()
            f1 = stack.pop()
            f1 += f"{chr(OPCODE.JUMP.value)}{chr(len(f2) + 1)}"
            
            nf = f"{chr(OPCODE.ALT.value)}{chr(2)}{chr(len(f1) + 1)}" + f1 + f2

            stack.append(nf)
            i += 1

        elif c == '?':
            f = stack.pop()
            nf = f"{chr(OPCODE.ALT.value)}{chr(2)}{chr(len(f) + 1)}" + f

            stack.append(nf)
            i += 1

        elif c == '+':
            f = stack.pop()
            nf = f + f"{chr(OPCODE.ALT.value)}{chr(2)}{chr(3)}" + f"{chr(OPCODE.BACK.value)}{chr(len(f) + 4)}"

            stack.append(nf)
            i += 1

        elif c == '*':
            f = stack.pop()
            f += f"{chr(OPCODE.BACK.value)}{chr(1 + 3 + len(f))}"
            nf = f"{chr(OPCODE.ALT.value)}{chr(2)}{chr(len(f) + 1)}" + f

            stack.append(nf)
            i += 1

        elif c == '&':
            f2 = stack.pop()
            f1 = stack.pop()
            stack.append(f1 + f2)
            i += 1

        elif c == '[':
            s = ""
            i += 1
            while(post[i] != ']'):
                if post[i] == '\\':
                    i += 1
                s += post[i]
                i += 1

            stack.append(f"{chr(OPCODE.CLASS.value)}{chr(len(s))}" + s)
            i += 1
            continue

        elif c == '.':
            stack.append(f"{chr(OPCODE.WILD.value)}")
            i += 1

        else:
            stack.append(f"{chr(OPCODE.CHAR.value)}{c}")
            i += 1

    prog = "".join(stack) + f"{chr(OPCODE.MATCH.value)}"

    return prog

def match(re, s, sub=False) -> bool:
    if sub:
        subre = assemble(".*")
        subre = subre[:6]
        re = subre + re[:len(re) - 1] + subre + chr(OPCODE.MATCH.value)
    clist = []
    nlist = []
    pc = 0
    sp = 0
    s += '\0'

    clist.append(pc)
    while(sp < len(s)):
        i = 0
        while(i < len(clist)):
            pc = clist[i]
            op = ord(re[pc])

            if op == OPCODE.CHAR.value:
                pc += 1
                if re[pc] == s[sp]:
                    nlist.append(pc + 1)
                i += 1
                continue

            elif op == OPCODE.WILD.value:
                nlist.append(pc + 1)
                i += 1
                continue

            elif op == OPCODE.JUMP.value:
                pc += 1
                clist.append(pc + ord(re[pc]))
                i += 1
                continue

            elif op == OPCODE.BACK.value:
                pc += 1
                clist.append(pc - ord(re[pc]))
                i += 1
                continue

            elif op == OPCODE.ALT.value:
                pc += 1
                clist.append(pc + ord(re[pc]))
                pc += 1
                clist.append(pc + ord(re[pc]))
                i += 1
                continue

            elif op == OPCODE.CLASS.value:
                pc += 1
                end = ord(re[pc]) + pc
                pc += 1

                found = False
                while(pc <= end):
                    if re[pc] == s[sp]:
                        found = True
                    pc += 1

                if found:
                    nlist.append(pc)

                i += 1
                continue

            elif op == OPCODE.BLINE.value:
                if sp == 0:
                    clist.append(pc + 1)
                i += 1
                continue

            elif op == OPCODE.ELINE.value:
                if sp == (len(s) - 1):
                    clist.append(pc + 1)
                i += 1
                continue

            elif op == OPCODE.MATCH.value:
                if sp == (len(s) - 1):
                    return True
                i += 1
                continue

        clist = nlist
        nlist = []
        sp += 1

    return False


if __name__ == '__main__':
    post = parse(argv[1])
    print(post)
    re = assemble(post)
    print(match(re, argv[2], True))
