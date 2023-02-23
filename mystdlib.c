#include <unistd.h>

#include "mystdlib.h"
#include "myutil.h"

extern char** environ;

char* get_env(const char* name) {
  int name_len = str_len(name);
  int i = 0;
  while (environ[i] != NULL) {
    if (str_eql_l(name, environ[i], name_len))
      return environ[i] + name_len + 1;
    i++;
  }
  
  return NULL;
}
