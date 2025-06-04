#include "function_parser.hpp"
#include <iostream>

FunctionParser::FunctionParser(const std::string& headerPath)
    : headerPath_(headerPath), index_(nullptr), unit_(nullptr) {}

FunctionParser::~FunctionParser() {
    if (unit_) clang_disposeTranslationUnit(unit_);
    if (index_) clang_disposeIndex(index_);
}

std::vector<FunctionInfo> FunctionParser::parse() {
    index_ = clang_createIndex(0, 0);
    unit_ = clang_parseTranslationUnit(index_, headerPath_.c_str(),
                                       nullptr, 0, nullptr, 0, CXTranslationUnit_None);
    if (!unit_) {
        std::cerr << "Failed to parse file: " << headerPath_ << "\n";
        return {};
    }

    CXCursor rootCursor = clang_getTranslationUnitCursor(unit_);
    clang_visitChildren(rootCursor, visitor, this);
    return functions_;
}

void FunctionParser::handleFunction(CXCursor cursor) {
    FunctionInfo func;
    func.name = clang_getCString(clang_getCursorSpelling(cursor));

    int numArgs = clang_Cursor_getNumArguments(cursor);
    std::unordered_set<std::string> visitedTypes;

    for (int i = 0; i < numArgs; ++i) {
        CXCursor arg = clang_Cursor_getArgument(cursor, i);
        CXType argType = clang_getCursorType(arg);
        CXString typeName = clang_getTypeSpelling(argType);
        std::string typeStr = clang_getCString(typeName);

        func.arguments.push_back({ typeStr, false });

        // Check canonical type for record structs
        CXType canonicalType = clang_getCanonicalType(argType);
        if (canonicalType.kind == CXType_Record) {
            auto def = extractStructDefinition(canonicalType, visitedTypes);
            if (!def.name.empty()) {
                types_.push_back(def);
            }
        }

        clang_disposeString(typeName);
    }

    if (clang_Cursor_isVariadic(cursor)) {
        func.arguments.push_back({ "...", true });
    }

    functions_.push_back(func);
}

CXChildVisitResult FunctionParser::visitor(CXCursor cursor, CXCursor, CXClientData clientData) {
    if (clang_getCursorKind(cursor) == CXCursor_FunctionDecl) {
        static_cast<FunctionParser*>(clientData)->handleFunction(cursor);
    }
    return CXChildVisit_Recurse;
}

TypeDefinition FunctionParser::extractStructDefinition(CXType type, std::unordered_set<std::string>& visited) {
    CXCursor decl = clang_getTypeDeclaration(type);
    CXString typeName = clang_getCursorSpelling(decl);
    std::string nameStr = clang_getCString(typeName);
    std::cout << "[STRUCT] Extracting type: " << nameStr << "\n";
    clang_disposeString(typeName);

    if (nameStr.empty()) {
        std::cout << "[SKIP] Struct has no name.\n";
        return {};
    }
    
    if (globalTypeCache_.count(nameStr)) {
        std::cout << "[CACHE HIT] Reusing type: " << nameStr << "\n";
        return globalTypeCache_[nameStr];
    }
    

    TypeDefinition def;
    def.name = nameStr;

    struct Context {
        TypeDefinition* defPtr;
        FunctionParser* parser;
        std::unordered_set<std::string>* visitedPtr;
    } context = { &def, this, &visited };

    clang_visitChildren(decl,
        [](CXCursor c, CXCursor, CXClientData clientData) {
            auto* ctx = static_cast<Context*>(clientData);
            if (clang_getCursorKind(c) == CXCursor_FieldDecl) {
                CXType fieldType = clang_getCursorType(c);
                CXString fieldTypeName = clang_getTypeSpelling(fieldType);
                CXString fieldName = clang_getCursorSpelling(c);
                std::string typeStr = clang_getCString(fieldTypeName);
                std::string nameStr = clang_getCString(fieldName);
                std::cout << "  [FIELD] " << typeStr << " " << nameStr << "\n";

                // ctx->defPtr->fields.emplace_back(typeStr, nameStr);

                // if (fieldType.kind == CXType_Record) {
                //     auto nestedDef = ctx->parser->extractStructDefinition(fieldType, *ctx->visitedPtr);
                //     if (!nestedDef.name.empty()) {
                //         ctx->defPtr->nested.push_back(nestedDef);
                //     }
                // }
                if (clang_getCanonicalType(fieldType).kind == CXType_Record) {
                    auto nestedDef = ctx->parser->extractStructDefinition(clang_getCanonicalType(fieldType), *ctx->visitedPtr);
                    if (!nestedDef.name.empty()) {
                        // Inline the nested definition under the current field
                        nestedDef.name = nameStr;  // e.g., name it "e" if declared as "event e"
                        ctx->defPtr->nested.push_back(nestedDef);
                    }
                } else {
                    ctx->defPtr->fields.emplace_back(typeStr, nameStr);
                }
                

                clang_disposeString(fieldTypeName);
                clang_disposeString(fieldName);
            }
            return CXChildVisit_Continue;
        }, &context);

    globalTypeCache_[def.name] = def;
    
    return def;
}



std::vector<TypeDefinition> FunctionParser::getCollectedTypes() const {
    return types_;
}

void printStruct(const TypeDefinition& type, int indent) {
    std::string pad(indent * 2, ' ');
    std::cout << pad << "Struct: " << type.name << "\n";
    for (const auto& field : type.fields) {
        std::cout << pad << "  Field: " << field.first << " " << field.second << "\n";
    }
    for (const auto& nested : type.nested) {
        printStruct(nested, indent + 1);
    }
}