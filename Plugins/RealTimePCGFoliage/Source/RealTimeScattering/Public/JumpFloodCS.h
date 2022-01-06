#pragma once
#include "EngineModule.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

#define LOCTEXT_NAMESPACE "GraphicToolsPlugin"

#define NUM_THREADS_PER_GROUP_DIMENSION 8

/// <summary>
/// Internal class thet holds the parameters and connects the HLSL Shader to the engine
/// </summary>
///

class FJFAInitCS : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FJFAInitCS);

	SHADER_USE_PARAMETER_STRUCT(FJFAInitCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_TEXTURE(Texture2D, InputSeed)
		SHADER_PARAMETER_UAV(RWTexture2D<uint2>, OutputStepRT)
		SHADER_PARAMETER(FIntRect, SimulateRect)
		SHADER_PARAMETER(uint32, Inverse)
	END_SHADER_PARAMETER_STRUCT()

public:
	//Called by the engine to determine which permutations to compile for this shader
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	//Modifies the compilations environment of the shader
	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		//We're using it here to add some preprocessor defines. That way we don't have to change both C++ and HLSL code when we change the value for NUM_THREADS_PER_GROUP_DIMENSION
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}

};

class FJFAStepCS : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FJFAStepCS);

	SHADER_USE_PARAMETER_STRUCT(FJFAStepCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_TEXTURE(Texture2D, InputSeed)
		SHADER_PARAMETER_TEXTURE(Texture2D, InputStepRT)
		SHADER_PARAMETER_UAV(RWTexture2D<uint2>, OutputStepRT)
		SHADER_PARAMETER(FIntRect, SimulateRect)
		SHADER_PARAMETER(uint32, Step)
		SHADER_PARAMETER(uint32, SubstractSeedRadius)
		SHADER_PARAMETER(float, LengthScale)
	END_SHADER_PARAMETER_STRUCT()

public:
	//Called by the engine to determine which permutations to compile for this shader
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	//Modifies the compilations environment of the shader
	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		//We're using it here to add some preprocessor defines. That way we don't have to change both C++ and HLSL code when we change the value for NUM_THREADS_PER_GROUP_DIMENSION
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}

};
class FJFASDFOutputCS : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FJFASDFOutputCS);

	SHADER_USE_PARAMETER_STRUCT(FJFASDFOutputCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_TEXTURE(Texture2D, InputSeed)
		SHADER_PARAMETER_TEXTURE(Texture2D, InputStepRT)
		SHADER_PARAMETER_UAV(RWTexture2D<float>, OutputSDF)
		SHADER_PARAMETER(FIntRect, SimulateRect)
		SHADER_PARAMETER(uint32, SubstractSeedRadius)
		SHADER_PARAMETER(float, LengthScale)
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);		
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}

};

#undef NUM_THREADS_PER_GROUP_DIMENSION

#undef LOCTEXT_NAMESPACE

