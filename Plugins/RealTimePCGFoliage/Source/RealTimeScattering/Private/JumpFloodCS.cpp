#include "JumpFloodCS.h"


IMPLEMENT_GLOBAL_SHADER(FJFAInitCS, "/CustomShaders/JumpFlood.usf", "InitStepMain", SF_Compute);

IMPLEMENT_GLOBAL_SHADER(FJFAStepCS, "/CustomShaders/JumpFlood.usf", "JFAMain", SF_Compute);

IMPLEMENT_GLOBAL_SHADER(FJFASDFOutputCS, "/CustomShaders/JumpFlood.usf", "SDFOutputMain", SF_Compute);