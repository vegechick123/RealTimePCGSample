#include "UAVCleanCS.h"


IMPLEMENT_GLOBAL_SHADER(FUAVCleanCS, "/CustomShaders/UAVClean.usf", "MainComputeShader", SF_Compute);