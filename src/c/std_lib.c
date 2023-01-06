#include "header/std_lib.h"

// Opsional : Tambahkan implementasi kode C
int div (int a, int b) {
  int res = 0;
  while (a>=b) {
    a -= b;
    res++;
  }
  return res;
}

int mod (int a, int n) {
  return a - div(a,n) * n;
}

void memcpy(byte *dest, byte *src, unsigned int n) {
  clear(dest, strlen(dest));
  while (n--) {
    *dest++ = *src++;
  }
}

unsigned int strlen(char *string) {
  int res = 0;
  while (string[res] != 0x0) {
    res++;
  }
  return res;
}

bool strcmp(char *s1, char *s2) {
  int i = 0;
  
  if (strlen(s1) != strlen(s2)) {
    return false;
  }
  for (i = 0; i < strlen(s1); i++) {
    if (s1[i] != s2[i]) {
      return false;
    }
  }
  return true;
}

void strcpy(char *dst, char *src) {
  clear(dst, strlen(dst));
  while ((*dst++ = *src++) != '\0');
}

void clear(byte *ptr, unsigned int n) {
  while (n > 0) {
    *ptr++ = 0;
    n--;
  }
}