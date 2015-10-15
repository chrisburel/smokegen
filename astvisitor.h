#ifndef SMOKEGEN_ASTVISITOR
#define SMOKEGEN_ASTVISITOR

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>

class SmokegenASTVisitor : public clang::RecursiveASTVisitor<SmokegenASTVisitor> {
public:
    SmokegenASTVisitor(clang::CompilerInstance &ci) : ci(ci) {}

    bool VisitCXXRecordDecl(clang::CXXRecordDecl *D);
    bool VisitEnumDecl(clang::EnumDecl *D);
    bool VisitNamespaceDecl(clang::NamespaceDecl *D);
    bool VisitFunctionDecl(clang::FunctionDecl *D);

private:
    clang::CompilerInstance &ci;
};

#endif