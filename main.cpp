#include <iostream>
#include <fstream>
#include <streambuf>
#include <new>
#include <cstring>
#include <glslang/ShaderLang.h>
#include <glslang/Types.h>

void InitializeResources(TBuiltInResource &resources) {
	resources.maxLights = 32;
	resources.maxClipPlanes = 6;
	resources.maxTextureUnits = 32;
	resources.maxTextureCoords = 32;
	resources.maxVertexAttribs = 64;
	resources.maxVertexUniformComponents = 4096;
	resources.maxVaryingFloats = 64;
	resources.maxVertexTextureImageUnits = 32;
	resources.maxCombinedTextureImageUnits = 80;
	resources.maxTextureImageUnits = 32;
	resources.maxFragmentUniformComponents = 4096;
	resources.maxDrawBuffers = 32;
	resources.maxVertexUniformVectors = 128;
	resources.maxVaryingVectors = 8;
	resources.maxFragmentUniformVectors = 16;
	resources.maxVertexOutputVectors = 16;
	resources.maxFragmentInputVectors = 15;
	resources.minProgramTexelOffset = -8;
	resources.maxProgramTexelOffset = 7;
	resources.maxClipDistances = 8;
	resources.maxComputeWorkGroupCountX = 65535;
	resources.maxComputeWorkGroupCountY = 65535;
	resources.maxComputeWorkGroupCountZ = 65535;
	resources.maxComputeWorkGroupSizeX = 1024;
	resources.maxComputeWorkGroupSizeY = 1024;
	resources.maxComputeWorkGroupSizeZ = 64;
	resources.maxComputeUniformComponents = 1024;
	resources.maxComputeTextureImageUnits = 16;
	resources.maxComputeImageUniforms = 8;
	resources.maxComputeAtomicCounters = 8;
	resources.maxComputeAtomicCounterBuffers = 1;
	resources.maxVaryingComponents = 60;
	resources.maxVertexOutputComponents = 64;
	resources.maxGeometryInputComponents = 64;
	resources.maxGeometryOutputComponents = 128;
	resources.maxFragmentInputComponents = 128;
	resources.maxImageUnits = 8;
	resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
	resources.maxCombinedShaderOutputResources = 8;
	resources.maxImageSamples = 0;
	resources.maxVertexImageUniforms = 0;
	resources.maxTessControlImageUniforms = 0;
	resources.maxTessEvaluationImageUniforms = 0;
	resources.maxGeometryImageUniforms = 0;
	resources.maxFragmentImageUniforms = 8;
	resources.maxCombinedImageUniforms = 8;
	resources.maxGeometryTextureImageUnits = 16;
	resources.maxGeometryOutputVertices = 256;
	resources.maxGeometryTotalOutputComponents = 1024;
	resources.maxGeometryUniformComponents = 1024;
	resources.maxGeometryVaryingComponents = 64;
	resources.maxTessControlInputComponents = 128;
	resources.maxTessControlOutputComponents = 128;
	resources.maxTessControlTextureImageUnits = 16;
	resources.maxTessControlUniformComponents = 1024;
	resources.maxTessControlTotalOutputComponents = 4096;
	resources.maxTessEvaluationInputComponents = 128;
	resources.maxTessEvaluationOutputComponents = 128;
	resources.maxTessEvaluationTextureImageUnits = 16;
	resources.maxTessEvaluationUniformComponents = 1024;
	resources.maxTessPatchComponents = 120;
	resources.maxPatchVertices = 32;
	resources.maxTessGenLevel = 64;
	resources.maxViewports = 16;
	resources.maxVertexAtomicCounters = 0;
	resources.maxTessControlAtomicCounters = 0;
	resources.maxTessEvaluationAtomicCounters = 0;
	resources.maxGeometryAtomicCounters = 0;
	resources.maxFragmentAtomicCounters = 8;
	resources.maxCombinedAtomicCounters = 8;
	resources.maxAtomicCounterBindings = 1;
	resources.maxVertexAtomicCounterBuffers = 0;
	resources.maxTessControlAtomicCounterBuffers = 0;
	resources.maxTessEvaluationAtomicCounterBuffers = 0;
	resources.maxGeometryAtomicCounterBuffers = 0;
	resources.maxFragmentAtomicCounterBuffers = 1;
	resources.maxCombinedAtomicCounterBuffers = 1;
	resources.maxAtomicCounterBufferSize = 16384;
	resources.maxTransformFeedbackBuffers = 4;
	resources.maxTransformFeedbackInterleavedComponents = 64;
	resources.maxCullDistances = 8;
	resources.maxCombinedClipAndCullDistances = 8;
	resources.maxSamples = 4;
	resources.limits.nonInductiveForLoops = true;
	resources.limits.whileLoops = true;
	resources.limits.doWhileLoops = true;
	resources.limits.generalUniformIndexing = true;
	resources.limits.generalAttributeMatrixVectorIndexing = true;
	resources.limits.generalVaryingIndexing = true;
	resources.limits.generalSamplerIndexing = true;
	resources.limits.generalVariableIndexing = true;
	resources.limits.generalConstantMatrixVectorIndexing = true;
}

int main(int argc, char** argv) {
	if(argc == 4) {
		glslang::InitializeProcess();
		glslang::TShader* p_shader = nullptr;
		if (strcmp(argv[2], "vertex") == 0) {
			p_shader = new(std::nothrow) glslang::TShader(EShLanguage::EShLangVertex);
		}
		else if (strcmp(argv[2], "fragment") == 0) {
			p_shader = new(std::nothrow) glslang::TShader(EShLanguage::EShLangFragment);
		}
		if (p_shader == nullptr) {
			std::cout 
				<< "Something is going wrong," 
				<< "memory allocation failed!"
				<< std::endl;
			return 1;
		}
		std::ifstream file(argv[1]);
		if (!file.is_open()) {
			std::cout
				<< "Something is going wrong,"
				<< "file open failed!"
				<< std::endl;
			return 1;
		}
		EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
		TBuiltInResource resources;
		InitializeResources(resources);
		const char* shader_content_c_str[1];
		std::string shader_content(
			(std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>()
		);
		shader_content_c_str[0] = shader_content.c_str();
		p_shader->setStrings(&shader_content_c_str[0], 1);
		
		if (!p_shader->parse(&resources, 100, false, messages)) {
			std::cout << p_shader->getInfoLog() << std::endl;
			std::cout << p_shader->getInfoDebugLog() << std::endl;
			return 1;
		}

		glslang::TProgram program;
		program.addShader(p_shader);
		if (!program.link(messages)) {
			std::cout << p_shader->getInfoLog() << std::endl;
			std::cout << p_shader->getInfoDebugLog() << std::endl;
			return false;
		}
		if (!program.buildReflection()) {
			std::cout
				<< "Something is going wrong,"
				<< "reflection cannot be built!"
				<< std::endl;
			return 1;
		}
		
		auto attributes_size = program.getNumLiveAttributes();
		for (int i = 0; i < attributes_size; ++i) {
			std::cout << program.getAttributeName(i) << std::endl;
			std::cout << program.getAttributeType(i) << std::endl;
			const auto& type = program.getAttributeTType(i);
			std::cout << type->getVectorSize() << std::endl;
		}

		glslang::FinalizeProcess();
	}
	else {
		std::cout << "ShaderRetriever <filename> <vertex|fragment> <Output Filename>" << std::endl;
	}
	return 0;
}