#ifndef FUNCTION_PARSER_HPP
#define FUNCTION_PARSER_HPP

#include <string>
#include <vector>
#include <clang-c/Index.h>
#include <unordered_set>
#include <unordered_map>

struct ArgumentInfo {
    std::string type;
    bool isVariadic;
};

struct TypeDefinition {
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields; // (type, name)
    std::vector<TypeDefinition> nested;
};

struct FunctionInfo {
    std::string name;
    std::vector<ArgumentInfo> arguments;
    std::vector<TypeDefinition> usedTypes;
};

class FunctionParser {
public:
    FunctionParser(const std::string& headerPath);
    ~FunctionParser();

    std::vector<FunctionInfo> parse();
    std::vector<TypeDefinition> getCollectedTypes() const;
    std::vector<FunctionInfo> getFunctions() const;
    int getArgumentCount(const std::string& functionName, int maxVariadic = 0) const;

private:
    static CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData clientData);
    void handleFunction(CXCursor cursor);

    TypeDefinition extractStructDefinition(CXType type, std::unordered_set<std::string>& visited);

    std::string headerPath_;
    std::vector<FunctionInfo> functions_;
    std::vector<TypeDefinition> types_;
    CXIndex index_;
    CXTranslationUnit unit_;
    std::unordered_map<std::string, TypeDefinition> globalTypeCache_;
};

void printStruct(const TypeDefinition& type, int indent = 0);

#endif