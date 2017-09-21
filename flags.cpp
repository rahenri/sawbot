#include "flags.h"
#include "cmd_args.h"

const bool* Verbose = NewBoolFlag("verbose", false);

const std::string* TeeInput = NewStringFlag("tee-input", "");

const bool* AnalysisMode = NewBoolFlag("analysis", false);

const int* MaxDepth = NewIntFlag("max-depth", 100);

const int* HashSize = NewIntFlag("hash-size", 10000011);

const int* DepthShortening = NewIntFlag("depth-shortening", 0);
const int* ShorteningThreshold = NewIntFlag("shortening-treshold", 0);
