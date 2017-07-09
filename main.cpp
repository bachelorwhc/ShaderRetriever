#include <iostream>
#include <fstream>
#include <streambuf>
#include <new>
#include <cstring>
#include <glslang/ShaderLang.h>
#include <glslang/Types.h>
#include "init_resources.h"

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
			return 1;
		}
		if (!program.buildReflection()) {
			std::cout
				<< "Something is going wrong,"
				<< "reflection cannot be built!"
				<< std::endl;
			return 1;
		}

		std::cout << "Attributes:" << std::endl;
		auto attributes_size = program.getNumLiveAttributes();
		for (int i = 0; i < attributes_size; ++i) {
			std::cout << program.getAttributeName(i) << std::endl;
			std::cout << program.getAttributeType(i) << std::endl;
			const auto& type = program.getAttributeTType(i);
			std::cout << type->getVectorSize() << std::endl;
			const auto& qualifier = type->getQualifier();
			if (qualifier.hasBinding())
				std::cout << "binding: " << qualifier.layoutBinding << std::endl;
			if (qualifier.hasLocation())
				std::cout << "location: " << qualifier.layoutLocation << std::endl;
			if (qualifier.layoutPushConstant)
				std::cout << "PushConstant" << std::endl;
			std::cout << std::endl;
		}

		std::cout << std::endl << "UniformBlocks:" << std::endl;
		auto uniforms_size = program.getNumLiveUniformBlocks();
		for (int i = 0; i < uniforms_size; ++i) {
			std::cout << program.getUniformBlockName(i) << std::endl;
			std::cout << "block size:" << program.getUniformBlockSize(i) << std::endl;
			const auto& type = program.getUniformBlockTType(i);
			const auto& qualifier = type->getQualifier();
			if(qualifier.hasBinding())
				std::cout << "binding: " << qualifier.layoutBinding << std::endl;
			if (qualifier.hasLocation())
				std::cout << "location: " << qualifier.layoutLocation << std::endl;
			if (qualifier.layoutPushConstant)
				std::cout << "PushConstant" << std::endl;
			std::cout << std::endl;
		}

		glslang::FinalizeProcess();
	}
	else {
		std::cout << "ShaderRetriever <filename> <vertex|fragment> <Output Filename>" << std::endl;
	}
	return 0;
}