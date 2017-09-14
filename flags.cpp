#include "flags.h"
#include "cmd_args.h"

const bool* Verbose = NewBoolFlag("verbose", false);

const std::string* TeeInput = NewStringFlag("tee-input", "");

const bool* AnalysisMode = NewBoolFlag("analysis", false);

const int* MaxDepth = NewIntFlag("max-depth", 1000);
