#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

// Заменяет frem/srem/urem на: r = a - (a / b) * b
struct FremDecomposePass : public PassInfoMixin<FremDecomposePass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    std::vector<BinaryOperator *> toReplace;

    for (auto &BB : F)
      for (auto &I : BB)
        if (auto *binOp = dyn_cast<BinaryOperator>(&I))
          if (binOp->getOpcode() == Instruction::FRem ||
              binOp->getOpcode() == Instruction::SRem ||
              binOp->getOpcode() == Instruction::URem)
            toReplace.push_back(binOp);

    if (toReplace.empty())
      return PreservedAnalyses::all();

    for (auto *rem : toReplace) {
      IRBuilder<> builder(rem);

      Value *a = rem->getOperand(0);
      Value *b = rem->getOperand(1);
      Value *div = nullptr;
      Value *mul = nullptr;
      Value *sub = nullptr;

      switch (rem->getOpcode()) {
      case Instruction::FRem: {
        // fdiv (fp) -> fptosi (truncate toward zero) -> sitofp -> fmul -> fsub
        // Сохраняем fast-math флаги оригинальной инструкции
        Value *fdiv = builder.CreateFDivFMF(a, b, rem, "frem.div");
        Type *intTy = [&]() -> Type * {
          if (rem->getType()->isVectorTy())
            return VectorType::get(
                builder.getInt64Ty(),
                cast<VectorType>(rem->getType())->getElementCount());
          return builder.getInt64Ty();
        }();
        Value *trunc = builder.CreateFPToSI(fdiv, intTy, "frem.trunc");
        Value *truncFP =
            builder.CreateSIToFP(trunc, rem->getType(), "frem.trunc.fp");
        mul = builder.CreateFMulFMF(truncFP, b, rem, "frem.mul");
        sub = builder.CreateFSubFMF(a, mul, rem, "frem.sub");
        break;
      }
      case Instruction::SRem:
        // r = a - (a / b) * b  (signed integer)
        div = builder.CreateSDiv(a, b, "srem.div");
        mul = builder.CreateMul(div, b, "srem.mul");
        sub = builder.CreateSub(a, mul, "srem.sub");
        break;

      case Instruction::URem:
        // r = a - (a / b) * b  (unsigned integer)
        div = builder.CreateUDiv(a, b, "urem.div");
        mul = builder.CreateMul(div, b, "urem.mul");
        sub = builder.CreateSub(a, mul, "urem.sub");
        break;

      default:
        llvm_unreachable("unexpected opcode");
      }

      rem->replaceAllUsesWith(sub);
      rem->eraseFromParent();
    }

    return PreservedAnalyses::none();
  }

  // Разрешаем запуск на функциях с optnone
  static bool isRequired() { return true; }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "eremin_v_lab_2_frem_decompose",
          LLVM_VERSION_STRING, [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (name == "eremin_v_lab_2_frem_decompose") {
                    FPM.addPass(FremDecomposePass{});
                    return true;
                  }
                  return false;
                });
          }};
}