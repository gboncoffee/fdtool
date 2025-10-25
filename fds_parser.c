#include "fds.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char*
ParserErrorString(ParserError error)
{
    switch (error)
    {
    case NoError:
        return "no error";
    case NoContent:
        return "no content";
    case ExpectedArrowAfterDash:
        return "expected > after -";
    case ExpectedUniverseAndDependenciesDeclaration:
        return "expected universe and dependencies declaration";
    case ExpectedEqualsAfterVariableOnTopLevel:
        return "expected = after variable on top level";
    case ExpectedOpenBracketAfterEqualsOnTopLevel:
        return "expected { after = on top level";
    case UnexpectedToken:
        return "unexpected token";
    case RedeclarationOfUniverse:
        return "redeclaration of the Universe";
    case RedeclarationOfDependencies:
        return "redeclaration of the dependencies";
    }
    return NULL;
}

typedef enum
{
    Variable,
    Arrow,
    OpenBracket,
    CloseBracket,
    Equals,
    Comma,
} TokenKind;

typedef struct
{
    TokenKind kind;
    char      content;
} Token;

typedef struct TokenList
{
    struct TokenList* next;
    Token             token;
} TokenList;

typedef struct VariableList
{
    char                 variable;
    struct VariableList* next;
} VariableList;

const TokenList*
ParseElementOfUniverse(
    const TokenList* const tokens,
    TermList**             ret,
    ParserError*           errorRet
)
{
    if (tokens == NULL)
    {
        *errorRet = UnexpectedToken;
        return NULL;
    }

    switch (tokens->token.kind)
    {
    case Variable:
        if (tokens->next == NULL)
        {
            *errorRet = UnexpectedToken;
            return NULL;
        }
        *ret         = malloc(sizeof(**ret));
        (*ret)->term = tokens->token.content;
        (*ret)->next = NULL;
        return ParseElementOfUniverse(tokens->next, &(*ret)->next, errorRet);
    case Comma:
        return ParseElementOfUniverse(tokens->next, ret, errorRet);
    case CloseBracket:
        *ret = NULL;
        return tokens->next;
    default:
        *errorRet = UnexpectedToken;
        return tokens;
    }
}

const TokenList*
ParseElementOfLvalue(
    const TokenList* const tokens,
    TermList**             ret,
    ParserError* const     errorRet
)
{
    if (tokens == NULL)
    {
        *errorRet = UnexpectedToken;
        return tokens;
    }

    switch (tokens->token.kind)
    {
    case Arrow:
        return tokens;
    case Variable:
        *ret         = malloc(sizeof(**ret));
        (*ret)->term = tokens->token.content;
        (*ret)->next = NULL;
        return ParseElementOfLvalue(tokens->next, &(*ret)->next, errorRet);
    default:
        *errorRet = UnexpectedToken;
        return tokens;
    }
}

/* Pre-declaration for ParseElementOfRvalue. */
const TokenList* ParseElementOfDependency(
    const TokenList* const tokens,
    DependencyList**       ret,
    ParserError* const     errorRet
);

const TokenList*
ParseElementOfRvalue(
    const TokenList* const tokens,
    TermList**             ret,
    ParserError* const     errorRet
)
{
    if (tokens == NULL)
    {
        *errorRet = UnexpectedToken;
        return tokens;
    }

    if (tokens->token.kind != Variable)
        return tokens;

    *ret         = malloc(sizeof(**ret));
    (*ret)->term = tokens->token.content;
    (*ret)->next = NULL;
    return ParseElementOfRvalue(tokens->next, &(*ret)->next, errorRet);
}

const TokenList*
ParseElementOfDependency(
    const TokenList* const tokens,
    DependencyList**       ret,
    ParserError* const     errorRet
)
{
    if (tokens == NULL)
    {
        *errorRet = UnexpectedToken;
        return NULL;
    }

    const TokenList* rem;
    switch (tokens->token.kind)
    {
    case Variable:
        *ret                 = malloc(sizeof(**ret));
        (*ret)->next         = NULL;
        (*ret)->lvalue       = malloc(sizeof(*(*ret)->lvalue));
        (*ret)->lvalue->term = tokens->token.content;
        (*ret)->lvalue->next = NULL;
        rem =
            ParseElementOfLvalue(tokens->next, &(*ret)->lvalue->next, errorRet);
        if (*errorRet != NoError)
            return rem;
        if (rem->token.kind != Arrow)
        {
            *errorRet = UnexpectedToken;
            return rem;
        }
        rem = ParseElementOfRvalue(rem->next, &(*ret)->rvalue, errorRet);
        if (rem == NULL)
        {
            *errorRet = UnexpectedToken;
            return rem;
        }
        switch (rem->token.kind)
        {
        case Comma:
            return ParseElementOfDependency(rem->next, &(*ret)->next, errorRet);
        case CloseBracket:
            return rem->next;
        default:
            *errorRet = UnexpectedToken;
            return rem;
        }
    default:
        *errorRet = UnexpectedToken;
        return tokens;
    }
}

ParserError
Parse(const TokenList* const tokens, Depset* const depsetRet)
{
    if (tokens == NULL)
    {
        if (depsetRet->dependencies == NULL || depsetRet->universe == NULL)
            return ExpectedUniverseAndDependenciesDeclaration;
        return NoError;
    }

    const TokenList* remaining = tokens;

    if (remaining->token.kind != Variable && remaining->token.content != 'U' &&
        remaining->token.content != 'F')
    {
        return ExpectedUniverseAndDependenciesDeclaration;
    }

    int isUniverse = remaining->token.content == 'U';

    remaining = remaining->next;

    if (remaining == NULL || remaining->token.kind != Equals)
        return ExpectedEqualsAfterVariableOnTopLevel;
    remaining = remaining->next;

    if (remaining == NULL || remaining->token.kind != OpenBracket)
        return ExpectedOpenBracketAfterEqualsOnTopLevel;
    remaining = remaining->next;

    /* Descent. */

    ParserError error = NoError;
    if (isUniverse)
    {
        if (depsetRet->universe != NULL)
            return RedeclarationOfUniverse;
        remaining =
            ParseElementOfUniverse(remaining, &depsetRet->universe, &error);
    }
    else
    {
        if (depsetRet->dependencies != NULL)
            return RedeclarationOfDependencies;
        remaining = ParseElementOfDependency(
            remaining, &depsetRet->dependencies, &error
        );
    }

    if (error != NoError)
        return error;

    return Parse(remaining, depsetRet);
}

void
DropTokens(TokenList* const tokens)
{
    if (tokens == NULL)
        return;
    DropTokens(tokens->next);
    free(tokens);
}

TokenList*
Tokenize(
    const char* const  content,
    const long         contentSize,
    ParserError* const errorRet
)
{
    long        size = contentSize;
    const char* text = content;

    /* Loop for the sake of not blowing up the stack. */

    while (size > 0 && (isspace(text[0]) || !isprint(text[0])))
    {
        size -= 1;
        text = &text[1];
    }

    if (size <= 0)
        return NULL;

    const char* nextText = &text[1];
    long        nextSize = size - 1;

    /* Handle -> inline because it's the only token with more than one character
     * anyways. */
    if (text[0] == '-')
    {
        if (nextSize <= 0)
        {
            *errorRet = NoContent;
            return NULL;
        }
        if (nextText[0] != '>')
        {
            *errorRet = ExpectedArrowAfterDash;
            return NULL;
        }
        nextText = &nextText[1];
        nextSize = nextSize - 1;
    }

    TokenList* const new = malloc(sizeof(*new));

    switch (text[0])
    {
    case '-':
        new->token.kind = Arrow;
        break;
    case '{':
        new->token.kind = OpenBracket;
        break;
    case '}':
        new->token.kind = CloseBracket;
        break;
    case '=':
        new->token.kind = Equals;
        break;
    case ',':
        new->token.kind = Comma;
        break;
    default:
        new->token.kind    = Variable;
        new->token.content = text[0];
        break;
    }

    new->next = Tokenize(nextText, nextSize, errorRet);

    return new;
}

ParserError
ParseFds(const char* const content, const long contentSize, Depset* const ret)
{
    /* I wish I could write this in Haskell or Elixir or any other functional
     * language. */

    memset(ret, 0, sizeof(*ret));

    ParserError error  = NoError;
    TokenList*  tokens = Tokenize(content, contentSize, &error);
    if (error != NoError)
    {
        DropTokens(tokens);
        return error;
    }

    error = Parse(tokens, ret);
    DropTokens(tokens);

    return error;
}

void
DropTermList(TermList* const list)
{
    if (list == NULL)
        return;
    DropTermList(list->next);
    free(list);
}

void
DropDependencyList(DependencyList* const list)
{
    if (list == NULL)
        return;
    DropDependencyList(list->next);
    DropTermList(list->lvalue);
    DropTermList(list->rvalue);
    free(list);
}

void
DropDepset(Depset set)
{
    DropTermList(set.universe);
    DropDependencyList(set.dependencies);
}

void
PrintTermList(TermList* list)
{
    if (list == NULL)
        return;

    putchar(list->term);
    PrintTermList(list->next);
}

void
PrintDependencies(DependencyList* list)
{
    if (list == NULL)
        return;

    PrintTermList(list->lvalue);
    printf(" -> ");
    PrintTermList(list->rvalue);
    putchar('\n');

    PrintDependencies(list->next);
}

void
PrintFds(Depset set)
{
    PrintTermList(set.universe);
    putchar('\n');
    PrintDependencies(set.dependencies);
}
