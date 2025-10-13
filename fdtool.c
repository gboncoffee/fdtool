#include "fds.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
Usage(void)
{
    printf(
        "Subcommands: \n"
        "  closure    --fds <file> --X <attributes>\n"
        "  mincover   --fds <file>\n"
        "  keys       --fds <file>\n"
        "  normalform --fds <file>\n"
    );

    return 1;
}

const char*
GetArg(int argc, char* const argv[], const char* const arg)
{
    for (int i = 1; i < argc - 1; i += 1)
    {
        if (argv[i][0] == '\0' || argv[i][0] != '-' || argv[i][1] == '\0')
            continue;

        int idx = 1;
        if (argv[i][1] == '-')
            idx += 1;

        if (strcmp(&argv[i][idx], arg) == 0)
            return argv[i + 1];
    }

    return NULL;
}

const char*
GetNonArg(int argc, char* const argv[])
{
    int i = 1;
    while (i < argc)
    {
        /* This is magic: if we find a -, we skip the next argument. So just
         * comparing if it's not a - works. */
        if (argv[i][0] == '\0' || argv[i][0] != '-')
            return argv[i];

        i += 2;
    }

    return NULL;
}

const char*
SubcommandOrDie(int argc, char* argv[])
{
    const char* const subcommand = GetNonArg(argc, argv);
    if (subcommand == NULL)
        exit(Usage());
    return subcommand;
}

Depset
FdsFileOrDie(int argc, char* argv[])
{
    const char* const file = GetArg(argc, argv, "fds");
    if (file == NULL)
        exit(Usage());

    FILE* fp = fopen(file, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Couldn't read file %s.\n", file);
        exit(1);
    }

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "File %s is not seekable.\n", file);
        exit(1);
    }

    /* No way these can fail after a successful fseek. Right? */
    long size = ftell(fp);
    rewind(fp);

    char* const buffer = malloc(size * sizeof(*buffer));
    fread(buffer, size, sizeof(*buffer), fp);

    Depset            set;
    const ParserError error = ParseFds(buffer, size, &set);
    if (error != NoError)
    {
        fprintf(stderr, "Error parsing file: %s.\n", ParserErrorString(error));
        exit(1);
    }

    PrintFds(set);

    return set;
}

int
main(int argc, char* argv[])
{
    const char* const subcommand = SubcommandOrDie(argc, argv);
    const char* const file       = GetArg(argc, argv, "fds");
    if (file == NULL)
        return Usage();

    const Depset set = FdsFileOrDie(argc, argv);

    if (strcmp(subcommand, "closure") == 0)
    {
        const char* const attributes = GetArg(argc, argv, "X");
        if (attributes != NULL)
            return Closure(set, attributes);
        return Usage();
    }

    if (strcmp(subcommand, "mincover") == 0)
        return MinCover(set);
    if (strcmp(subcommand, "keys") == 0)
        return Keys(set);
    if (strcmp(subcommand, "normalform") == 0)
        return NormalForm(set);

    return Usage();
}
