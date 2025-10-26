#include "fds.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/** 
 * Verifica se um atributo está no conjunto.
 * Basicamente, vai percorrendo a string procurando o caractere
 */
static bool
HasAttribute(const char* set, char attr)
{
    while (*set)
    {
        if (*set == attr)
            return true;
        set++;
    }
    return false;
}

/**
 * Verifica se todos os atributos de lvalue estão no closure.
 * percorre a lista ligada de lvalue e verifica se cada atributo está no closure
 * retorna false assim que encontrar um atributo que não está
 */
static bool
IsSubset(TermList* lvalue, const char* closure)
{
    while (lvalue != NULL)
    {
        if (!HasAttribute(closure, lvalue->term))
            return false;
        lvalue = lvalue->next;
    }
    return true;
}

// Adiciona atributos de rvalue ao closure (se ainda não estiverem)
static bool
AddAttributes(char* closure, TermList* rvalue)
{
    bool changed = false;
    while (rvalue != NULL)
    {
        if (!HasAttribute(closure, rvalue->term))
        {
            size_t len       = strlen(closure);
            closure[len]     = rvalue->term;
            closure[len + 1] = '\0';
            changed          = true;
        }
        rvalue = rvalue->next;
    }
    return changed;
}

// Ordena a string alfabeticamente ex: "DBCA" -> "ABCD"
static void
SortString(char* str)
{
    size_t len = strlen(str);
    for (size_t i = 0; i < len - 1; i++)
    {
        for (size_t j = i + 1; j < len; j++)
        {
            if (str[i] > str[j])
            {
                char temp = str[i];
                str[i]    = str[j];
                str[j]    = temp;
            }
        }
    }
}

void
GetClosure(
    const DependencyList* const dependencies,
    const char* const           attributes,
    char*                       ret
)
{
    ret[27] = '\0';
    strncpy(ret, attributes, 26);

    // Itera até ponto fixo
    bool changed = true;
    while (changed)
    {
        changed = false;

        // Para cada dependência L -> R em F
        const DependencyList* dep = dependencies;
        while (dep != NULL)
        {
            // Se L está contido em closure, adiciona R ao closure
            if (IsSubset(dep->lvalue, ret))
            {
                if (AddAttributes(ret, dep->rvalue))
                    changed = true;
            }
            dep = dep->next;
        }
    }

    SortString(ret);
}

int
Closure(const Depset set, const char* const attributes)
{
    // Inicializa o closure com X
    char closure[27] = {0}; // 26 letras + '\0'
    GetClosure(set.dependencies, attributes, closure);

    printf("%s\n", closure);

    return 0;
}
