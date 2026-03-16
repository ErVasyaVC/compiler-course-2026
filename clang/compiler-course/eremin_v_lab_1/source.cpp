#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <vector>

namespace {

struct ResourceInfo {
  std::string name;
  std::string type;
  unsigned lineNumber;
  bool released = false;
  const clang::VarDecl *varDecl = nullptr;
};

class ResourceVisitor : public clang::RecursiveASTVisitor<ResourceVisitor> {
public:
  explicit ResourceVisitor(clang::ASTContext *ctx) : context(ctx) {}

  bool VisitVarDecl(clang::VarDecl *var) {
    if (!var->hasInit())
      return true;

    clang::Expr *init = var->getInit()->IgnoreParenCasts();
    if (auto *newExpr = llvm::dyn_cast<clang::CXXNewExpr>(init)) {
      addResource(var, newExpr->isArray() ? "new[]" : "new",
                  newExpr->getBeginLoc());
    } else if (auto *callExpr = llvm::dyn_cast<clang::CallExpr>(init)) {
      handleCallExpr(var, callExpr);
    }
    return true;
  }

  bool VisitBinaryOperator(clang::BinaryOperator *binOp) {
    if (!binOp->isAssignmentOp())
      return true;

    auto *lhs =
        llvm::dyn_cast<clang::DeclRefExpr>(binOp->getLHS()->IgnoreParenCasts());
    if (!lhs)
      return true;

    clang::VarDecl *var = llvm::dyn_cast<clang::VarDecl>(lhs->getDecl());
    if (!var)
      return true;

    clang::Expr *rhs = binOp->getRHS()->IgnoreParenCasts();
    if (auto *newExpr = llvm::dyn_cast<clang::CXXNewExpr>(rhs)) {
      addResource(var, newExpr->isArray() ? "new[]" : "new",
                  newExpr->getBeginLoc());
    } else if (auto *callExpr = llvm::dyn_cast<clang::CallExpr>(rhs)) {
      handleCallExpr(var, callExpr);
    }

    return true;
  }

  bool VisitCXXDeleteExpr(clang::CXXDeleteExpr *del) {
    auto *expr = llvm::dyn_cast<clang::DeclRefExpr>(
        del->getArgument()->IgnoreParenCasts());
    if (!expr)
      return true;

    std::string typeToRelease = del->isArrayForm() ? "new[]" : "new";

    if (auto *varDecl = llvm::dyn_cast<clang::VarDecl>(expr->getDecl())) {
      releaseResource(varDecl, typeToRelease);
    }
    return true;
  }

  bool VisitCallExpr(clang::CallExpr *call) {
    clang::Expr *calleeExpr = call->getCallee()->IgnoreParenCasts();
    std::string funcName;

    if (auto *dre = llvm::dyn_cast<clang::DeclRefExpr>(calleeExpr)) {
      funcName = dre->getDecl()->getNameAsString();
    } else {
      return true;
    }

    if (funcName == "free" || funcName == "fclose") {
      if (call->getNumArgs() > 0) {
        if (auto *argDRE = llvm::dyn_cast<clang::DeclRefExpr>(
                call->getArg(0)->IgnoreParenCasts())) {
          if (auto *varDecl =
                  llvm::dyn_cast<clang::VarDecl>(argDRE->getDecl())) {
            releaseResource(varDecl, funcName == "free" ? "malloc" : "fopen");
          }
        }
      }
    }

    return true;
  }

  void printWarnings() {
    for (auto &r : resources) {
      if (!r.released) {
        llvm::errs() << "Warning: resource '" << r.type
                     << "' not released at line " << r.lineNumber << "\n";
      }
    }
  }

private:
  clang::ASTContext *context;
  std::vector<ResourceInfo> resources;

  void addResource(const clang::VarDecl *var, const std::string &type,
                   clang::SourceLocation loc) {
    ResourceInfo r;
    r.name = var->getNameAsString();
    r.varDecl = var;
    r.type = type;
    r.lineNumber = context->getSourceManager().getSpellingLineNumber(loc);
    resources.push_back(r);
  }

  void handleCallExpr(const clang::VarDecl *var, clang::CallExpr *call) {
    clang::Expr *calleeExpr = call->getCallee()->IgnoreParenCasts();
    std::string funcName;

    if (auto *dre = llvm::dyn_cast<clang::DeclRefExpr>(calleeExpr)) {
      funcName = dre->getDecl()->getNameAsString();
    } else {
      return;
    }

    if (funcName == "malloc" || funcName == "calloc")
      addResource(var, "malloc", call->getBeginLoc());
    if (funcName == "fopen")
      addResource(var, "fopen", call->getBeginLoc());

    if (funcName == "free" || funcName == "fclose") {
      if (call->getNumArgs() > 0) {
        if (auto *argDRE = llvm::dyn_cast<clang::DeclRefExpr>(
                call->getArg(0)->IgnoreParenCasts())) {
          if (auto *varDecl =
                  llvm::dyn_cast<clang::VarDecl>(argDRE->getDecl())) {
            releaseResource(varDecl, funcName == "free" ? "malloc" : "fopen");
          }
        }
      }
    }
  }

  void releaseResource(const clang::VarDecl *var, const std::string &type) {
    for (auto &r : resources) {
      if (r.varDecl == var && r.type == type && !r.released) {
        r.released = true;
        break;
      }
    }
  }
};

class ResourceASTConsumer : public clang::ASTConsumer {
public:
  explicit ResourceASTConsumer(clang::ASTContext *ctx) : visitor(ctx) {}

  void HandleTranslationUnit(clang::ASTContext &ctx) override {
    visitor.TraverseDecl(ctx.getTranslationUnitDecl());
    visitor.printWarnings();
  }

private:
  ResourceVisitor visitor;
};

class ResourcePluginAction : public clang::PluginASTAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &ci, llvm::StringRef) override {
    return std::make_unique<ResourceASTConsumer>(&ci.getASTContext());
  }

  bool ParseArgs(const clang::CompilerInstance &,
                 const std::vector<std::string> &) override {
    return true;
  }
};

} // namespace

static clang::FrontendPluginRegistry::Add<ResourcePluginAction>
    X("eremin_v_lab_1_resource_checker",
      "Detects unreleased resources including new[]/delete[], malloc/free, "
      "fopen/fclose");