#ifndef RENAME_TRANSFORMS_H
#define RENAME_TRANSFORMS_H

#include "Transforms.h"
#include <pcrecpp.h>
#include <clang/Lex/Preprocessor.h>

class RenameTransform : public Transform {
public:
  RenameTransform() : indentLevel(0) {}
protected:
  // utility functions shared by all rename transforms
  
  bool loadConfig(const std::string& transformName,                        
                  const std::string& renameKeyName,
                  const std::string& ignoreKeyName = "Ignore") {
    auto S = TransformRegistry::get().config[transformName];
    if (!S.IsMap()) {
      llvm::errs() << "Error: Cannot find config entry \"" << transformName
                   << "\" or entry is not a map\n";
      return false;
    }

    auto IG = S[ignoreKeyName];

    if (IG && !IG.IsSequence()) {
      llvm::errs() << "Error: Config key \"" << ignoreKeyName
                   << "\" must be a sequence\n";
      return false;
    }

    for (auto I = IG.begin(), E = IG.end(); I != E; ++I) {
      if (I->IsScalar()) {
        auto P = I->as<std::string>();
        ignoreList.push_back(pcrecpp::RE(P));
        llvm::errs() << "Ignoring: " << P << "\n";
      }
    }

    auto RN = S[renameKeyName];
    if (!RN.IsSequence()) {
      llvm::errs() << "\"" << renameKeyName << "\" is not specified or is"
                   << " not a sequence\n";
      return false;
    }

    for (auto I = RN.begin(), E = RN.end(); I != E; ++I) {
      if (!I->IsMap()) {
        llvm::errs() << "Error: \"" << renameKeyName
                     << "\" contains non-map items\n";
        return false;
      }

      for (auto MI = I->begin(), ME = I->end(); MI != ME; ++MI) {
        auto F = MI->first.as<std::string>();
        auto T = MI->second.as<std::string>();      
        pcrecpp::RE re(F);
        renameList.push_back(REStringPair(re, T));

        llvm::errs() << "renames: " << F << " -> " << T << "\n";
      }
    }
    
    return true;
  }
  
  bool shouldIgnore(clang::SourceLocation L) {
    if (!L.isValid()) {
      return true;
    }

    clang::SourceManager &SM = sema->getSourceManager();
    clang::FullSourceLoc FSL(L, SM);
    const clang::FileEntry *FE = SM.getFileEntryForID(FSL.getFileID());
    if (!FE) {
      return true;
    }

    auto FN = FE->getName();

    for (auto I = ignoreList.begin(), E = ignoreList.end(); I != E; ++I) {
      if (I->FullMatch(FN)) {
        return true;
      }
    }

    return false;
  }
  
  // if we have a NamedDecl and the fully-qualified name matches
  bool nameMatches(const clang::NamedDecl *D, std::string &outNewName,
                   bool checkOnly = false) {
    auto I = nameMap.find(D);
    if (I != nameMap.end()) {
      outNewName = (*I).second;
      return true;
    }
    
    // if we only need a quick look-up of renamed Decl's
    // (e.g. for renamed parent classes, overridden methods etc.)
    if (checkOnly) {
      return false;
    }

    if (!D->getLocation().isValid()) {
      return false;
    }
        
    auto QN = D->getQualifiedNameAsString();
    if (QN.size() == 0) {
      return false;
    }
    
    // special handling for TagDecl
    if (auto T = llvm::dyn_cast<clang::TagDecl>(D)) {
      auto KN = T->getKindName();
      assert(KN && "getKindName() must return a non-NULL value");
      QN.insert(0, KN);
      QN.insert(strlen(KN), " ");
    }
    
    for (auto I = renameList.begin(), E = renameList.end(); I != E; ++I) {
      if (I->first.FullMatch(QN)) {
        std::string newName;
        I->first.Extract(I->second, QN, &newName);
        nameMap[D] = newName;
        outNewName = newName;
        return true;
      }
    }
  
    return false;
  }
  
  // useful when we can't just rely on Decl, e.g. built-in type
  // unmatched names are cached to speed things up
  bool stringMatches(std::string name, std::string &outNewName) {
    auto I = matchedStringMap.find(name);
    if (I != matchedStringMap.end()) {
      outNewName = (*I).second;
      return true;
    }

    auto J = unmatchedStringSet.find(name);
    if (J != unmatchedStringSet.end()) {
      return false;
    }

    for (auto I = renameList.begin(), E = renameList.end(); I != E; ++I) {
      if (I->first.FullMatch(name)) {
        std::string newName;
        I->first.Extract(I->second, name, &newName);
        matchedStringMap[name] = newName;
        outNewName = newName;
        return true;
      }
    }
    
    unmatchedStringSet.insert(name);
    return false;
  }
  
  void renameLocation(clang::SourceLocation L, std::string& N) {
    if (L.isValid()) {
      if (L.isMacroID()) {        
        // TODO: emit error using diagnostics
        llvm::errs() << "Warning: Token is resulted from macro expansion"
          " and is therefore not renamed, at: " << loc(L) << "\n";
      }
      else {
        clang::Preprocessor &P = sema->getPreprocessor();      
        auto LE = P.getLocForEndOfToken(L);
        if (LE.isValid()) {
          // getLocWithOffset returns the location *past* the token, hence -1
          auto E = LE.getLocWithOffset(-1);
          rewriter.ReplaceText(clang::SourceRange(L, E), N);
        }
      }
    }    
  }
    
  const std::string& indent() {    
    return indentString;
  }
  
  void pushIndent() {
    indentLevel++;
    indentString = std::string(indentLevel * 2, ' ');
  }
  
  void popIndent() {
    assert(indentLevel >= 0 && "indentLevel must be >= 0");
    indentLevel--;
    indentString = std::string(indentLevel * 2, ' ');
  }
  
  std::string loc(clang::SourceLocation L) {
    std::string src;
    llvm::raw_string_ostream sst(src);
    L.print(sst, sema->getSourceManager());
    return sst.str();
  }
  
private:
  int indentLevel;
  std::string indentString;

  std::vector<pcrecpp::RE> ignoreList;
  typedef std::pair<pcrecpp::RE, std::string> REStringPair;
  std::vector<REStringPair> renameList;

  std::map<const clang::Decl *, std::string> nameMap;
  std::map<std::string, std::string> matchedStringMap;
  std::set<std::string> unmatchedStringSet;
};

#endif