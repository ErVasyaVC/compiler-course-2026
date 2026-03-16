// RUN: %clang_cc1 -load %llvmshlibdir/eremin_v_lab_1_ClangAST%pluginext -plugin eremin_v_lab_1_resource_checker -fsyntax-only %s 2>&1 | FileCheck %s

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

// CHECK: Warning: resource 'new' not released
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
    // CHECK: Warning: resource 'new[]' not released
    return arr;
}

void testTemplateLeaks() {
    auto* a = createLeakyArray<int>(5);
    auto* b = createLeakyArray<double>(3);
}

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