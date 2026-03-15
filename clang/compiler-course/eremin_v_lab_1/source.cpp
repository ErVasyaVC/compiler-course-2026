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
  std::string resourceType;
  unsigned lineNumber;
  bool released = false;
};

class ResourceVisitor : public clang::RecursiveASTVisitor<ResourceVisitor> {
public:
  explicit ResourceVisitor(clang::ASTContext *ctx) : context(ctx) {}

  bool VisitCXXNewExpr(clang::CXXNewExpr *expr) {
    auto &sm = context->getSourceManager();

    ResourceInfo r;
    r.resourceType = "new";
    r.lineNumber = sm.getSpellingLineNumber(expr->getBeginLoc());

    resources.push_back(r);
    return true;
  }

  bool VisitCXXDeleteExpr(clang::CXXDeleteExpr *expr) {

    for (auto &r : resources) {
      if (r.resourceType == "new" && !r.released) {
        r.released = true;
        break;
      }
    }

    return true;
  }

  bool VisitCallExpr(clang::CallExpr *call) {

    auto *callee = call->getDirectCallee();
    if (!callee)
      return true;

    std::string funcName = callee->getNameAsString();
    auto &sm = context->getSourceManager();

    if (funcName == "malloc") {
      ResourceInfo r;
      r.resourceType = "malloc";
      r.lineNumber = sm.getSpellingLineNumber(call->getBeginLoc());
      resources.push_back(r);
    }

    if (funcName == "fopen") {
      ResourceInfo r;
      r.resourceType = "fopen";
      r.lineNumber = sm.getSpellingLineNumber(call->getBeginLoc());
      resources.push_back(r);
    }

    if (funcName == "free") {
      for (auto &r : resources) {
        if (r.resourceType == "malloc" && !r.released) {
          r.released = true;
          break;
        }
      }
    }

    if (funcName == "fclose") {
      for (auto &r : resources) {
        if (r.resourceType == "fopen" && !r.released) {
          r.released = true;
          break;
        }
      }
    }

    return true;
  }

  void printWarnings() {
    for (auto &r : resources) {
      if (!r.released) {
        llvm::errs() << "Warning: resource '" << r.resourceType
                     << "' not released at line " << r.lineNumber << "\n";
      }
    }
  }

private:
  clang::ASTContext *context;
  std::vector<ResourceInfo> resources;
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
    Y("resource_checker", "Detects unreleased resources");