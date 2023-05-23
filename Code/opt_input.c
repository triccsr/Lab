#include <stdio.h>
#include "def.h"

void input_to_IRList(const char fileName[]) {
  freopen(fileName, "r", stdin);
  fclose(stdin);
}