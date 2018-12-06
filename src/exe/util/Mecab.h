#pragma once

void UninitMecab();
int InitMecab();
wchar_t *MecabParseString(wchar_t *string, int len, wchar_t **error = 0);
