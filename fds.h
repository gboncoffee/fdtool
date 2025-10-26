#ifndef FDS_H_
#define FDS_H_

typedef enum
{
    NoError,
    NoContent,
    ExpectedArrowAfterDash,
    ExpectedUniverseAndDependenciesDeclaration,
    ExpectedEqualsAfterVariableOnTopLevel,
    ExpectedOpenBracketAfterEqualsOnTopLevel,
    UnexpectedToken,
    RedeclarationOfUniverse,
    RedeclarationOfDependencies,
}

ParserError;

typedef struct TermList
{
    struct TermList* next;
    char             term;
} TermList;

typedef struct DependencyList
{
    TermList*              lvalue;
    TermList*              rvalue;
    struct DependencyList* next;
} DependencyList;

typedef struct
{
    TermList*       universe;
    DependencyList* dependencies;
} Depset;

ParserError
ParseFds(const char* const content, const long contentSize, Depset* ret);
const char* ParserErrorString(ParserError error);

int Closure(const Depset set, const char* const attributes);
int MinCover(const Depset set);
int Keys(const Depset set);
int NormalForm(const Depset set);

void GetClosure(
    const DependencyList* const dependencies,
    const char* const           attributes,
    char*                       ret
);

void DropDepset(Depset set);
void DropDependencyList(DependencyList* const list);
void DropTermList(TermList* const list);

/* Remove before sending for the sake of everything. */
void PrintFds(Depset set);

#endif /* FDS_H_ */
