// RUN: %clang_cc1 -load %llvmshlibdir/resource_checker%pluginext -plugin resource_checker -fsyntax-only %s 2>&1 | FileCheck %s

#include <cstdlib>
#include <cstdio>

// CHECK: Warning: resource 'new' not released at line [[LINE1:[0-9]+]]
void testNewLeak() {
  int *a = new int(10); // CHECK-LABEL: testNewLeak
}

// CHECK: Warning: resource 'new' not released at line [[LINE2:[0-9]+]]
void testArrayNewLeak() {
  int *arr = new int[5];
}

// CHECK: Warning: resource 'malloc' not released at line [[LINE3:[0-9]+]]
void testMallocLeak() {
  int *p = (int*)malloc(sizeof(int) * 4);
}

// CHECK: Warning: resource 'fopen' not released at line [[LINE4:[0-9]+]]
void testFileLeak() {
  FILE *f = fopen("data.txt", "r");
}

void testProperDelete() {
  int *a = new int(5);
  delete a;
}

// CHECK-NOT: resource 'new'
void testCleanMemory() {
  int *p = new int(7);
  delete p;

  int *m = (int*)malloc(10);
  free(m);

  FILE *f = fopen("ok.txt", "r");
  fclose(f);
}

// CHECK: Warning: resource 'new' not released
void testLoopLeak() {
  for (int i = 0; i < 3; ++i) {
    int *p = new int(i);
  }
}

int main() {
  testNewLeak();
  testArrayNewLeak();
  testMallocLeak();
  testFileLeak();
  testProperDelete();
  testCleanMemory();
  testLoopLeak();
  return 0;
}