/* Tests mkdir(). */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (mkdir ("a"), "mkdir \"a\"");
  ASSERT(0);
  CHECK (create ("a/b", 512), "create \"a/b\"");
  CHECK (chdir ("a"), "chdir \"a\"");
  CHECK (open ("b") > 1, "open \"b\"");
}

