
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bf.h"

void HandleOutputArgument(BF_Argv *argv, char *arg) {
	if(!argv || !arg) return;
	
	argv->output = strdup(arg);
}

void HandleGenerateArgument(BF_Argv *argv, char *_) {
	argv->flags |= BF_FLAG_GENERATE_SUDO;
}

void HandleDebugArgument(BF_Argv *argv, char *_) {
	argv->flags |= BF_FLAG_DEBUG;
}

void HandleOptimizationArgument(BF_Argv *argv, char *arg) {
	if(!argv || !arg) return;

	switch (arg[0])
	{
	case '0': argv->optimizationLevel = BF_OPT_NONE; break;
	case '1': argv->optimizationLevel = BF_OPT_MIN; break;
	case '2': argv->optimizationLevel = BF_OPT_MAX; break;
	}
};

static struct {
	const char *prefix;
	void (*handler)(BF_Argv *argv, char *arg); 
	bool isFlags;
} gOptionals[] = {
	{ "--out", &HandleOutputArgument, 			false },
	{ "--opt", &HandleOptimizationArgument,		false },

	{ "--generate", &HandleGenerateArgument,	true },
	
	{ NULL, NULL, false }
};

static bool LoadOptional(int *j, char **args, BF_Argv *argv) {
	for(int i = 0; gOptionals[i].prefix; ++i) {
		if (strcmp(gOptionals[i].prefix, args[*j]) == 0) {
			if(gOptionals[i].isFlags) {
				(*gOptionals[i].handler)(argv, NULL);
				return true;
			}

			++*j;	
			(*gOptionals[i].handler)(argv, args[*j]);
	
			return true;
		}
	}	
	
	return false;
}

void BF_LoadArguments(int argc, char *argv[], BF_Argv *args) {
	memset(args, 0, sizeof(BF_Argv));

	char *source = NULL;
	for(int i = 1; i < argc; ++i) {
		if(LoadOptional(&i, argv, args)) continue;

		if(!args->source)
			args->source = strdup(argv[i]);
	}
}

void BF_FreeArguments(BF_Argv *argv) {
	free(argv->output);
	free(argv->source);
}