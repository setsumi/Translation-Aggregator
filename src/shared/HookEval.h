#pragma once

struct Registers {
	int edi, esi, ebp, esp, ebx, edx, ecx, eax;
	//int eax, ecx, edx, ebx, esp, ebp, esi, edi;
};

ULONG HookEval(Registers *regs, wchar_t *s, int *error);
int HookEvalSyntaxCheck(wchar_t *s);
