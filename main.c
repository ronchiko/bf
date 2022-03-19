
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bf.h"

int main(int argc, char *in_argv[]) {
	BF_Argv argv;
	BF_LoadArguments(argc, in_argv, &argv);

	if(!argv.source) printf("No input file!\n"), exit(1);

	BF_Context *ctx = BF_Open(argv.source);

	if (argv.optimizationLevel >= 1)
		BF_OptimizeLevel1(ctx);

	if(argv.flags & BF_FLAG_GENERATE_SUDO) {
		char *sudo = BF_Export(ctx);
		char outName[513];
		strncpy(outName, argv.output ? argv.output : "", 512);

		if (!argv.output) {
			outName[0] = 0;
			strncat(outName, argv.source, 512);
			strncat(outName, ".abf", 512);
		}
		FILE *file = fopen(outName, "w+");
		if(!file) printf("Failed to create file %s\n", outName), exit(1);

		printf("Writing to file %s\n", outName);
		fprintf(file, "%s", sudo);

		fclose(file);
		free(sudo);
	}

	BF_SimulationContext *sim = BF_CreateSimulation(ctx);
	
	printf("Running program %s\n", argv.source);

	uint64_t steps = BF_Run(sim);
	if(sim->error) {
		printf("Got out with an error\n");
	}

	printf("Program finished (in %zu steps)\n", steps);
	BF_FreeSimulation(sim);
	BF_FreeContext(ctx);
	BF_FreeArguments(&argv);
	exit(0);
}