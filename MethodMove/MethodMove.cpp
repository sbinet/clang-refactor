
// based on https://github.com/loarabia/Clang-tutorial

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <set>

#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Lexer.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Rewriters.h"
#include "clang/Rewrite/Rewriter.h"

using namespace clang;

class MyRecursiveASTVisitor : public RecursiveASTVisitor<MyRecursiveASTVisitor>
{    
public:
    std::set<FileID> rewriteIDSet;
    Rewriter Rewrite;
    CompilerInstance *ci;
    
    bool TraverseDecl(Decl *D)
    {
        if (isa<NamespaceDecl>(D)) {
            NamespaceDecl* N = static_cast<NamespaceDecl*>(D);
            llvm::outs() << "Namespace: " << N->getDeclName().getAsString() << "\n";
        }
        else if (isa<CXXRecordDecl>(D)) {
            CXXRecordDecl* R = static_cast<CXXRecordDecl*>(D);
            llvm::outs() << "CXXRecordDecl: " << R->getDeclName().getAsString() << "\n";            
        }
        else if (isa<CXXMethodDecl>(D)) {
            CXXMethodDecl* M = static_cast<CXXMethodDecl*>(D);
            llvm::outs() << "Method: " << M->getDeclName().getAsString();
            
            SourceRange R = M->getSourceRange();
            
            llvm::outs() << ", body? " << M->hasBody() << ", from : ";
            
            R.getBegin().print(llvm::outs(), ci->getSourceManager());
            
            llvm::outs() << ", to: ";
            R.getEnd().print(llvm::outs(), ci->getSourceManager());
            
            llvm::outs() << "\n";            
            
            if (M->hasBody()) {
                Stmt *S = M->getBody();
                SourceLocation LBL = S->getLocStart();
                SourceLocation RBL = S->getLocEnd();
                
                FullSourceLoc FSL(LBL, ci->getSourceManager());
                const FileEntry* F = ci->getSourceManager().getFileEntryForID(FSL.getFileID());
                llvm::outs() << "in: " << F->getName() << ", ";
                
                std::string fn(F->getName());
                if (fn.rfind(".h") != std::string::npos) {
                    SourceLocation B = LBL;
                    
                    if (M->isInlineSpecified()) {
                        B = R.getBegin();
                        Rewrite.RemoveText(SourceRange(B, RBL));
                    }
                    else {
                        Rewrite.ReplaceText(SourceRange(B, RBL), ";");
                    }
                    
                    rewriteIDSet.insert(FSL.getFileID());
                    
                    llvm::outs() << "compound stmt, from : ";
                        
                    LBL.print(llvm::outs(), ci->getSourceManager());        
                    llvm::outs() << ", to: ";
                    RBL.print(llvm::outs(), ci->getSourceManager());
                    llvm::outs() << "\n";
                }
            }
        }
        
        bool traverseResult = RecursiveASTVisitor<MyRecursiveASTVisitor>::TraverseDecl(D);
        
        return traverseResult;
    }
};


class MyASTConsumer : public ASTConsumer {
public:
    MyASTConsumer(const char *f);
    virtual bool HandleTopLevelDecl(DeclGroupRef d);
    
    MyRecursiveASTVisitor rv;
};


MyASTConsumer::MyASTConsumer(const char *f)
{
    rv.ci = new CompilerInstance();

    // set to C++
    LangOptions& lo = rv.ci->getLangOpts();
    lo.CPlusPlus = 1;
    lo.CPlusPlus0x = 1;

    rv.ci->createDiagnostics(0, NULL);
    
    TargetOptions to;
    to.Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *pti = TargetInfo::CreateTargetInfo(rv.ci->getDiagnostics(), to);
    rv.ci->setTarget(pti);
    
    rv.ci->createFileManager();
    rv.ci->createSourceManager(rv.ci->getFileManager());
    rv.ci->createPreprocessor();
    rv.ci->getPreprocessorOpts().UsePredefines = true;
    
    rv.ci->setASTConsumer(this);
    
    rv.ci->createASTContext();
    
    
    // Initialize rewriter
    rv.Rewrite.setSourceMgr(rv.ci->getSourceManager(), rv.ci->getLangOpts());
    
    const FileEntry *pFile = rv.ci->getFileManager().getFile(f);
    rv.ci->getSourceManager().createMainFileID(pFile);
    rv.ci->getDiagnosticClient().BeginSourceFile(rv.ci->getLangOpts(),
                                                 &rv.ci->getPreprocessor());
    
    // Convert <file>.c to <file_out>.c
    
    

    ParseAST(rv.ci->getPreprocessor(), this, rv.ci->getASTContext());
    rv.ci->getDiagnosticClient().EndSourceFile();

    
    for (std::set<FileID>::iterator i = rv.rewriteIDSet.begin(), e = rv.rewriteIDSet.end(); i != e; ++i) {
        const FileEntry* F = rv.ci->getSourceManager().getFileEntryForID(*i);
        
        std::string outName(F->getName());
        size_t ext = outName.rfind(".");
        if (ext == std::string::npos) {
            ext = outName.length();
        }
        
        outName.insert(ext, "_out");
        
        llvm::errs() << "Output to: " << outName << "\n";
        std::string OutErrorInfo;
        
        llvm::raw_fd_ostream outFile(outName.c_str(), OutErrorInfo, 0);

        
        if (OutErrorInfo.empty())
        {
            // Now output rewritten source code
            const RewriteBuffer *RewriteBuf = rv.Rewrite.getRewriteBufferFor(*i);
            outFile << std::string(RewriteBuf->begin(), RewriteBuf->end());
        }
        else
        {
            llvm::errs() << "Cannot open " << outName << " for writing\n";
        }

        outFile.close();
    }
    
    
    delete rv.ci;
}

bool MyASTConsumer::HandleTopLevelDecl(DeclGroupRef d)
{
    llvm::outs() << __PRETTY_FUNCTION__ << ": ";
    
    if (d.isSingleDecl()) {
        clang::Decl* D = d.getSingleDecl();
        llvm::outs() << "single: " << D->getDeclKindName() << "\n";
    }
    else {
        llvm::outs() << "group" << "\n";
    }
    
    typedef DeclGroupRef::iterator iter;
    
    for (iter b = d.begin(), e = d.end(); b != e; ++b)
    {
        
        rv.TraverseDecl(*b);
    }
    
    return true; // keep going
}


int main(int argc, char *argv[])
{
    struct stat sb;
    
    if (argc != 2) {
        llvm::errs() << "usage: MethodMove <filename>\n";
        return 1;
    }
    
    if (stat(argv[1], &sb) == -1) {
        perror(argv[1]);
        exit(EXIT_FAILURE);
    }
    
    MyASTConsumer *astConsumer = new MyASTConsumer(argv[1]);
    (void)astConsumer;
    
    return 0;
}
