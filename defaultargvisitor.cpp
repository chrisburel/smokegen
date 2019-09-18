#include "defaultargvisitor.h"
#include <clang/Basic/SourceLocation.h>

bool DefaultArgVisitor::VisitDeclRefExpr(clang::DeclRefExpr* D) {
    if (clang::EnumConstantDecl* enumConstant = clang::dyn_cast<clang::EnumConstantDecl>(D->getDecl())) {
        containsEnum = true;

        std::string enumStr;
        llvm::raw_string_ostream s(enumStr);
        D->printPretty(s, nullptr, pp());
        s.flush();

        clang::NamedDecl *parent = clang::dyn_cast<clang::NamedDecl>(enumConstant->getDeclContext()->getParent());

        if ( parent ) {
          std::string prefix = parent ->getQualifiedNameAsString() + "::";
          if (enumStr.compare(0, prefix.length(), prefix)) {
              rewriter.InsertText(
                  rewriter.getSourceMgr().getFileLoc(D->getBeginLoc()),
                  prefix,
                  true,
                  true
              );
          }
        }
    }
    return true;
}

std::string DefaultArgVisitor::toString(const clang::Expr* D) const {
    if (!containsEnum) {
        return std::string();
    }

    clang::SourceLocation startLoc = rewriter.getSourceMgr().getFileLoc(D->getBeginLoc());
    clang::SourceLocation endLoc = rewriter.getSourceMgr().getFileLoc(D->getEndLoc());
    clang::SourceRange expandedLoc(startLoc, endLoc);
    return rewriter.getRewrittenText(expandedLoc);
}
