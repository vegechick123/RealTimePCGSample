#include "GPUScatteringCS.h"


IMPLEMENT_GLOBAL_SHADER(FGPUScatteringCS, "/CustomShaders/GPUScattering.usf", "MainComputeShader", SF_Compute);