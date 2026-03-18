// RUN: %clang_cc1 -load %llvmshlibdir/eremin_v_lab_1_ClangAST%pluginext -plugin eremin_v_lab_1_resource_checker -fsyntax-only %s 2>&1 | FileCheck %s

#include <cstdlib>
#include <cstdio>

// CHECK-LABEL: testNewLeak
// CHECK: Warning: resource 'new' not released
void testNewLeak() {
    int* a = new int(42);
}

// CHECK-LABEL: testNewDelete
// CHECK-NOT: Warning: resource 'new' not released
void testNewDelete() {
    int* a = new int(5);
    delete a;
}

// CHECK-LABEL: testNewArrayLeak
// CHECK: Warning: resource 'new[]' not released
void testNewArrayLeak() {
    int* arr = new int[10];
}

// CHECK-LABEL: testNewArrayDelete
// CHECK-NOT: Warning: resource 'new[]' not released
void testNewArrayDelete() {
    int* arr = new int[3];
    delete[] arr;
}



// CHECK-LABEL: testMallocLeak
// CHECK: Warning: resource 'malloc' not released
void testMallocLeak() {
    int* p = (int*)malloc(100);
}
// CHECK-LABEL: testMallocFree
// CHECK-NOT: Warning: resource 'malloc' not released
void testMallocFree() {
    int* p = (int*)malloc(50);
    free(p);
}


// CHECK-LABEL: testFileLeak
// CHECK: Warning: resource 'fopen' not released
void testFileLeak() {
    FILE* f = fopen("data.txt", "r");
}

// CHECK-LABEL: testFileClose
// CHECK-NOT: Warning: resource 'fopen' not released
void testFileClose() {
    FILE* f = fopen("ok.txt", "r");
    fclose(f);
}

// CHECK-LABEL: testAssignmentLeak
// CHECK: Warning: resource 'new' not released
void testAssignmentLeak() {
    int* ptr;
    ptr = new int(777);
}
// CHECK-LABEL: testAssignmentArrayLeak
// CHECK: Warning: resource 'new[]' not released
void testAssignmentArrayLeak() {
    int* arr;
    arr = new int[5];
}

// CHECK-LABEL: testScopeLeaks
void testScopeLeaks() {
    if (true) {
        // CHECK: Warning: resource 'new' not released
        int* a = new int(100);
    }

    for (int i = 0; i < 2; ++i) {
        // CHECK: Warning: resource 'new' not released
        int* p = new int(i);
    }
}

// CHECK-LABEL: createLeakyArray
// CHECK: Warning: resource 'new[]' not released
template<typename T>
T* createLeakyArray(int n) {
    T* arr = new T[n];
    return arr;
}

// CHECK-LABEL: testTemplateLeaks
void testTemplateLeaks() {
    auto* a = createLeakyArray<int>(5);
    auto* b = createLeakyArray<double>(3);
}


// CHECK-LABEL: testCleanMemory
// CHECK-NOT: resource 'new'
// CHECK-NOT: resource 'new[]'
// CHECK-NOT: resource 'malloc'
// CHECK-NOT: resource 'fopen'
void testCleanMemory() {
    int* a = new int(10);
    delete a;

    int* arr = new int[3];
    delete[] arr;

    int* m = (int*)malloc(10);
    free(m);

    FILE* f = fopen("clean.txt", "r");
    fclose(f);
}

int main() {
    testNewLeak();
    testNewDelete();
    testNewArrayLeak();
    testNewArrayDelete();
    testMallocLeak();
    testMallocFree();
    testFileLeak();
    testFileClose();
    testAssignmentLeak();
    testAssignmentArrayLeak();
    testScopeLeaks();
    testTemplateLeaks();
    testCleanMemory();
}