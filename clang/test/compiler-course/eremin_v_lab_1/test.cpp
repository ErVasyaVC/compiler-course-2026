// RUN: %clang_cc1 -load %llvmshlibdir/eremin_v_lab_1_ClangAST%pluginext -plugin eremin_v_lab_1_resource_checker -fsyntax-only %s 2>&1 | FileCheck %s
/ RUN: %clang_cc1 -load %llvmshlibdir/eremin_v_lab_1_ClangAST%pluginext -plugin eremin_v_lab_1_resource_checker -fsyntax-only %s 2>&1 | FileCheck %s

typedef unsigned long size_t;
extern "C" void* malloc(size_t);
extern "C" void free(void*);

struct FILE;
extern "C" FILE* fopen(const char*, const char*);
extern "C" int fclose(FILE*);



// CHECK-LABEL: testNewLeak
void testNewLeak() {
    // CHECK-DAG: Warning: resource 'new' not released at {{.*}}:[[@LINE+1]]
    int* a = new int(42);
}

// CHECK-LABEL: testNewDelete
// CHECK-NOT: Warning: resource 'new' not released
void testNewDelete() {
    int* a = new int(5);
    delete a;
}

// ===================== NEW[] =====================

// CHECK-LABEL: testNewArrayLeak
void testNewArrayLeak() {
    // CHECK-DAG: Warning: resource 'new[]' not released at {{.*}}:[[@LINE+1]]
    int* arr = new int[10];
}

// CHECK-LABEL: testNewArrayDelete
// CHECK-NOT: Warning: resource 'new[]' not released
void testNewArrayDelete() {
    int* arr = new int[3];
    delete[] arr;
}

extern "C" void* malloc(unsigned long);
extern "C" void free(void*);
struct FILE;
extern "C" FILE* fopen(const char*, const char*);
extern "C" int fclose(FILE*);

// CHECK: Warning: resource 'new' not released
void testNewLeak() {
    int* a = new int(42);
}

void testNewDelete() {
    int* a = new int(5);
    delete a;
}

// CHECK: Warning: resource 'new[]' not released
void testNewArrayLeak() {
    int* arr = new int[10];
}

void testNewArrayDelete() {
    int* arr = new int[3];
    delete[] arr;
}



// CHECK: Warning: resource 'malloc' not released
void testMallocLeak() {
    int* p = (int*)malloc(100);
}
void testMallocFree() {
    int* p = (int*)malloc(50);
    free(p);
}


// CHECK: Warning: resource 'fopen' not released
void testFileLeak() {
    FILE* f = fopen("data.txt", "r");
}

void testFileClose() {
    FILE* f = fopen("ok.txt", "r");
    fclose(f);
}

// CHECK: Warning: resource 'new' not released#include <cstdlib>
#include <cstdio>

void testAssignmentLeak() {
    int* ptr;
    ptr = new int(777);
}
// CHECK: Warning: resource 'new[]' not released
void testAssignmentArrayLeak() {
    int* arr;
    arr = new int[5];
}

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

template<typename T>
T* createLeakyArray(int n) {
    T* arr = new T[n];
    return arr;
}

void testTemplateLeaks() {
    auto* a = createLeakyArray<int>(5);
    auto* b = createLeakyArray<double>(3);
}


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
}*}}:[[@LINE+1]]
    int* p = (int*)malloc(100);
}

// CHECK-LABEL: testMallocFree
// CHECK-NOT: Warning: resource 'malloc' not released
void testMallocFree() {
    int* p = (int*)malloc(50);
    free(p);
}

// ===================== FILE =====================

// CHECK-LABEL: testFileLeak
void testFileLeak() {
    // CHECK-DAG: Warning: resource 'fopen' not released at {{.*}}:[[@LINE+1]]
    FILE* f = fopen("data.txt", "r");
}

// CHECK-LABEL: testFileClose
// CHECK-NOT: Warning: resource 'fopen' not released
void testFileClose() {
    FILE* f = fopen("ok.txt", "r");
    fclose(f);
}

// ===================== ASSIGNMENT =====================

// CHECK-LABEL: testAssignmentLeak
void testAssignmentLeak() {
    int* ptr;
    // CHECK-DAG: Warning: resource 'new' not released at {{.*}}:[[@LINE+1]]
    ptr = new int(777);
}

// CHECK-LABEL: testAssignmentArrayLeak
void testAssignmentArrayLeak() {
    int* arr;
    // CHECK-DAG: Warning: resource 'new[]' not released at {{.*}}:[[@LINE+1]]
    arr = new int[5];
}

// ===================== SCOPES =====================

// CHECK-LABEL: testScopeLeaks
void testScopeLeaks() {
    if (true) {
        // CHECK-DAG: Warning: resource 'new' not released at {{.*}}:[[@LINE+1]]
        int* a = new int(100);
    }

    for (int i = 0; i < 2; ++i) {
        // CHECK-DAG: Warning: resource 'new' not released at {{.*}}:[[@LINE+1]]
        int* p = new int(i);
    }
}

// ===================== TEMPLATE =====================

// CHECK-LABEL: createLeakyArray
template<typename T>
T* createLeakyArray(int n) {
    // CHECK-DAG: Warning: resource 'new[]' not released at {{.*}}:[[@LINE+1]]
    T* arr = new T[n];
    return arr;
}

// CHECK-LABEL: testTemplateLeaks
void testTemplateLeaks() {
    auto* a = createLeakyArray<int>(5);
    auto* b = createLeakyArray<double>(3);
}

// ===================== CLEAN =====================

// CHECK-LABEL: testCleanMemory
// CHECK-NOT: Warning: resource 'new'
// CHECK-NOT: Warning: resource 'new[]'
// CHECK-NOT: Warning: resource 'malloc'
// CHECK-NOT: Warning: resource 'fopen'
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

// ===================== MAIN =====================

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