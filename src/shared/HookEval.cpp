#include <Shared/Shrink.h>
#include "HookEval.h"


ULONG EvalSub(Registers *regs, wchar_t *&s, int *error, int endChar);

// Handles immediates/registers and operators that apply to one argument.
// Note that bracketed expressions as a whole are included in this, though
// the body between the brackets are not included.
ULONG EvalSingle(Registers *regs, wchar_t *&s, int *error) {
	ULONG temp;
	wchar_t *end;
	wchar_t code;
	switch(s[0]) {
		case '!':
		case '~':
			code = s[0];
			s++;
			temp = EvalSingle(regs, s, error);
			if (*error) return -1;
			if (code == '!')
				return !temp;
			if (code == '~')
				return ~temp;
			// ????
			break;
		case '_':
			s++;
			temp = EvalSingle(regs, s, error);
			if (*error) return -1;
			if (IsBadReadPtr((ULONG*)(regs->esp + temp), 4)) {
				*error = 1;
				return -1;
			}
			return *(ULONG*)(regs->esp + temp);
		case '-':
			if (s[1] < '0' || s[1] > '9') {
				s++;
				temp = EvalSingle(regs, s, error);
				if (!*error) temp = -(LONG)temp;
				return temp;
			}
			temp = wcstol(s, &end, 0);
			if (end == s) {
				*error = 1;
				return -1;
			}
			s = end;
			return temp;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			temp = wcstoul(s, &end, 0);
			if (end == s) {
				*error = 1;
				return -1;
			}
			s = end;
			return temp;
		case 'E':
			s++;
			s+=2;
			{
				int reg = *(int*)(s-2);
				switch (reg) {
					case 'X\0A':
						return regs->eax;
					case 'X\0B':
						return regs->ebx;
					case 'X\0C':
						return regs->ecx;
					case 'X\0D':
						return regs->edx;
					case 'P\0S':
						return regs->esp;
					case 'P\0B':
						return regs->ebp;
					case 'I\0S':
						return regs->esi;
					case 'I\0D':
						return regs->edi;
				}
			}
			*error = 1;
			return -1;
		case '(':
			s++;
			return EvalSub(regs, s, error, ')');
		case '[':
			s++;
			return EvalSub(regs, s, error, ']');
		default:
			break;
	};
	*error = 0;
	return -1;
}

// Handles operators that apply to two arguments.
ULONG EvalSub(Registers *regs, wchar_t *&s, int *error, int endChar) {
	ULONG arg1, arg2, argAdd;
	wchar_t op = '+', op2;
	argAdd = EvalSingle(regs, s, error);
	if (*error) return -1;
	arg1 = 0;

	while (s[0] != endChar) {
		if (!s[0]) {
			*error = 1;
			return -1;
		}
		switch (s[0]) {
			case '+':
			case '-':
				if (op == '+')
					arg1 += argAdd;
				else
					arg1 -= argAdd;
				op = s[0];
				s++;
				argAdd = EvalSingle(regs, s, error);
				if (*error) return -1;
				break;
			case '*':
			case '/':
			case '%':
			case '^':
			case '|':
			case '&':
				op2 = s[0];
				s++;
				arg2 = EvalSingle(regs, s, error);
				if (*error) return -1;
				if (op2 == '*')
					argAdd *= arg2;
				else if (op2 == '/')
					argAdd /= arg2;
				else if (op2 == '%')
					argAdd %= arg2;
				else if (op2 == '^')
					argAdd ^= arg2;
				else if (op2 == '|')
					argAdd |= arg2;
				else if (op2 == '&')
					argAdd &= arg2;
				break;
			case '>':
			case '<':
				if (s[1] != s[0]) {
					*error = 1;
					return -1;
				}
				op2 = s[0];
				s+=2;
				arg2 = EvalSingle(regs, s, error);
				if (*error) return -1;
				if (op2 == '>')
					argAdd >>= arg2;
				else
					argAdd <<= arg2;
				break;
			default:
				*error = 0;
				return -1;
		}
	};
	s++;

	if (op == '+')
		arg1 += argAdd;
	else
		arg1 -= argAdd;
	if (endChar == ']') {
		if (IsBadReadPtr((ULONG*)arg1, 4)) {
			*error = 1;
			return -1;
		}
		return *(ULONG*)arg1;
	}
	return arg1;
}

ULONG HookEval(Registers *regs, wchar_t *s, int *error) {
	int e = 0;
	wchar_t *str = s;
	ULONG out = EvalSub(regs, str, &e, 0);
	if (e) *error = e;
	return out;
}




ULONG EvalSyntaxCheckSub(wchar_t *&s, int endChar);
// Handles immediates/registers and operators that apply to one argument.
// Note that bracketed expressions as a whole are included in this, though
// the body between the brackets are not included.
ULONG EvalSyntaxCheckSingle(wchar_t *&s) {
	ULONG temp;
	wchar_t *end;
	switch(s[0]) {
		case '_':
		case '!':
		case '~':
			s++;
			return EvalSyntaxCheckSingle(s);
		case '-':
			if (s[1] < '0' || s[1] > '9') {
				s++;
				return EvalSyntaxCheckSingle(s);
			}
			temp = wcstol(s, &end, 0);
			if (end == s) {
				return 0;
			}
			s = end;
			return 1;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			temp = wcstoul(s, &end, 0);
			if (end == s) {
				return 0;
			}
			s = end;
			return 1;
		case 'E':
			s++;
			s+=2;
			{
				int reg = *(int*)(s-2);
				switch (reg) {
					case 'X\0A':
					case 'X\0B':
					case 'X\0C':
					case 'X\0D':
					case 'P\0S':
					case 'P\0B':
					case 'I\0S':
					case 'I\0D':
						return 1;
					default:
						return 0;
				}
			}
		case '(':
			s++;
			return EvalSyntaxCheckSub(s, ')');
		case '[':
			s++;
			return EvalSyntaxCheckSub(s, ']');
		default:
			return 0;
	};
}

// Handles operators that apply to two arguments.
ULONG EvalSyntaxCheckSub(wchar_t *&s, int endChar) {
	if (!EvalSyntaxCheckSingle(s))
		return 0;

	while (s[0] != endChar) {
		if (!s[0]) {
			return 0;
		}
		switch (s[0]) {
			case '+':
			case '-':
				s++;
				if (!EvalSyntaxCheckSingle(s))
					return 0;
				break;
			case '*':
			case '/':
			case '%':
			case '^':
			case '|':
			case '&':
				s++;
				if (!EvalSyntaxCheckSingle(s))
					return 0;
				break;
			case '>':
			case '<':
				if (s[1] != s[0]) {
					return 0;
				}
				if (!EvalSyntaxCheckSingle(s))
					return 0;
				break;
			default:
				return 0;
		}
	};
	s++;
	return 1;
}


int HookEvalSyntaxCheck(wchar_t *s) {
	if (!s) return 0;
	return EvalSyntaxCheckSub(s, 0);
}
