; RUN: opt -load-pass-plugin %llvmshlibdir/eremin_v_lab_2_LLVM_IR%pluginext \
; RUN:     -passes="eremin_v_lab_2_frem_decompose" -S %s | FileCheck %s

; ────────────────────────────────────────────────────────────────────────────
; frem (float) → fdiv + fptosi + sitofp + fmul + fsub
; ────────────────────────────────────────────────────────────────────────────
define float @test_frem_float(float %a, float %b) {
; CHECK-LABEL: @test_frem_float(
; CHECK-NOT:   = frem
; CHECK:       frem.div
; CHECK:       frem.trunc
; CHECK:       frem.trunc.fp
; CHECK:       frem.mul
; CHECK:       frem.sub
; CHECK:       ret float %frem.sub
  %r = frem float %a, %b
  ret float %r
}

; ────────────────────────────────────────────────────────────────────────────
; frem (double)
; ────────────────────────────────────────────────────────────────────────────
define double @test_frem_double(double %a, double %b) {
; CHECK-LABEL: @test_frem_double(
; CHECK-NOT:   = frem
; CHECK:       frem.div
; CHECK:       frem.trunc
; CHECK:       frem.trunc.fp
; CHECK:       frem.mul
; CHECK:       frem.sub
; CHECK:       ret double %frem.sub
  %r = frem double %a, %b
  ret double %r
}

; ────────────────────────────────────────────────────────────────────────────
; frem с fast-math флагами — флаги должны сохраниться
; ────────────────────────────────────────────────────────────────────────────
define float @test_frem_fast(float %a, float %b) {
; CHECK-LABEL: @test_frem_fast(
; CHECK-NOT:   = frem
; CHECK:       fdiv fast
; CHECK:       fmul fast
; CHECK:       fsub fast
  %r = frem fast float %a, %b
  ret float %r
}

; ────────────────────────────────────────────────────────────────────────────
; srem (i32) → sdiv + mul + sub
; ────────────────────────────────────────────────────────────────────────────
define i32 @test_srem_i32(i32 %a, i32 %b) {
; CHECK-LABEL: @test_srem_i32(
; CHECK-NOT:   = srem
; CHECK:       %srem.div = sdiv i32 %a, %b
; CHECK:       %srem.mul = mul i32 %srem.div, %b
; CHECK:       %srem.sub = sub i32 %a, %srem.mul
; CHECK:       ret i32 %srem.sub
  %r = srem i32 %a, %b
  ret i32 %r
}

; ────────────────────────────────────────────────────────────────────────────
; srem (i64)
; ────────────────────────────────────────────────────────────────────────────
define i64 @test_srem_i64(i64 %a, i64 %b) {
; CHECK-LABEL: @test_srem_i64(
; CHECK-NOT:   = srem
; CHECK:       sdiv i64
; CHECK:       mul i64
; CHECK:       sub i64
; CHECK:       ret i64 %srem.sub
  %r = srem i64 %a, %b
  ret i64 %r
}

; ────────────────────────────────────────────────────────────────────────────
; urem (i32) → udiv + mul + sub
; ────────────────────────────────────────────────────────────────────────────
define i32 @test_urem_i32(i32 %a, i32 %b) {
; CHECK-LABEL: @test_urem_i32(
; CHECK-NOT:   = urem
; CHECK:       %urem.div = udiv i32 %a, %b
; CHECK:       %urem.mul = mul i32 %urem.div, %b
; CHECK:       %urem.sub = sub i32 %a, %urem.mul
; CHECK:       ret i32 %urem.sub
  %r = urem i32 %a, %b
  ret i32 %r
}

; ────────────────────────────────────────────────────────────────────────────
; urem (i64)
; ────────────────────────────────────────────────────────────────────────────
define i64 @test_urem_i64(i64 %a, i64 %b) {
; CHECK-LABEL: @test_urem_i64(
; CHECK-NOT:   = urem
; CHECK:       udiv i64
; CHECK:       mul i64
; CHECK:       sub i64
; CHECK:       ret i64 %urem.sub
  %r = urem i64 %a, %b
  ret i64 %r
}

; ────────────────────────────────────────────────────────────────────────────
; Несколько rem в одной функции — все должны быть заменены
; ────────────────────────────────────────────────────────────────────────────
define i32 @test_multiple_rem(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: @test_multiple_rem(
; CHECK-NOT:   = srem
; CHECK:       sdiv i32
; CHECK:       mul i32
; CHECK:       sub i32
; CHECK:       sdiv i32
; CHECK:       mul i32
; CHECK:       sub i32
  %r1 = srem i32 %a, %b
  %r2 = srem i32 %r1, %c
  ret i32 %r2
}

; ────────────────────────────────────────────────────────────────────────────
; Функция без rem — pass ничего не меняет
; ────────────────────────────────────────────────────────────────────────────
define i32 @test_no_rem(i32 %a, i32 %b) {
; CHECK-LABEL: @test_no_rem(
; CHECK:       %r = add i32 %a, %b
; CHECK:       ret i32 %r
  %r = add i32 %a, %b
  ret i32 %r
}

; ────────────────────────────────────────────────────────────────────────────
; Векторный frem (<4 x float>)
; ────────────────────────────────────────────────────────────────────────────
define <4 x float> @test_frem_vec4(<4 x float> %a, <4 x float> %b) {
; CHECK-LABEL: @test_frem_vec4(
; CHECK-NOT:   = frem
; CHECK:       frem.div
; CHECK:       frem.trunc
; CHECK:       frem.trunc.fp
; CHECK:       frem.mul
; CHECK:       frem.sub
  %r = frem <4 x float> %a, %b
  ret <4 x float> %r
}
