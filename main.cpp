#include <iostream>
#include <fstream>
#include <exception>
#include <streambuf>
#include <new>
#include <cstring>
#include <map>
#include <glslang/ShaderLang.h>
#include <glslang/Types.h>
#include <json.hpp>
#include "init_resources.h"
#include "gl2vulkan.h"
#include "config.h"

using JSON = nlohmann::json;

void SetQualifier(const glslang::TQualifier& qualifier, JSON& json) {
	if (qualifier.hasBinding())
		json["binding"] = qualifier.layoutBinding;
	if (qualifier.hasLocation())
		json["location"] = qualifier.layoutLocation;
	if (qualifier.layoutPushConstant)
		json["push_constant"] = "true";
	else
		json["push_constant"] = "false";
}

int main(int argc, char** argv) {
	try {
		Config config(argc, argv);
		glslang::InitializeProcess();
		glslang::TShader* p_shader = nullptr;
		auto stage = config.getStage();
		switch (stage)
		{
		case Config::VERTEX:
			p_shader = new(std::nothrow) glslang::TShader(EShLanguage::EShLangVertex);
			break;
		case Config::FRAGMENT:
			p_shader = new(std::nothrow) glslang::TShader(EShLanguage::EShLangFragment);
			break;
		default:
			throw std::runtime_error("stage is not mapped.");
			break;
		}
		if (p_shader == nullptr) {
			std::cout
				<< "Something is going wrong,"
				<< "memory allocation failed!"
				<< std::endl;
			return 1;
		}
		std::ifstream file(config.getInputFilename());
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

		JSON conf_json;

		JSON attributes_json;
		auto attributes_size = program.getNumLiveAttributes();
		for (int i = 0; i < attributes_size; ++i) {
			JSON attri_json;
			auto attribute_type = program.getAttributeType(i);
			attri_json["type"] = config.isVulkanDef() ? AttributeGL2Vulkan(attribute_type) : attribute_type;
			
			const auto& type = program.getAttributeTType(i);
			attri_json["vector_size"] = type->getVectorSize();

			const auto& qualifier = type->getQualifier();
			SetQualifier(qualifier, attri_json);

			attributes_json[program.getAttributeName(i)] = attri_json;
		}
		conf_json["attributes"] = attributes_json;

		JSON uniform_blocks_json;
		auto uniforms_size = program.getNumLiveUniformBlocks();
		for (int i = 0; i < uniforms_size; ++i) {
			JSON uniform_blk_json;
			uniform_blk_json["block_size"] = program.getUniformBlockSize(i);
			
			const auto& type = program.getUniformBlockTType(i);

			const auto& qualifier = type->getQualifier();
			SetQualifier(qualifier, uniform_blk_json);

			uniform_blocks_json[program.getUniformBlockName(i)] = uniform_blk_json;
		}
		conf_json["uniform_blocks"] = uniform_blocks_json;

		glslang::FinalizeProcess();

		std::ofstream output_file(config.getOutputFilename());
		if(!output_file.is_open()) {
			std::cout
				<< "Something is going wrong,"
				<< "file can not be loaded!"
				<< std::endl;
			return 1;
		}
		output_file << std::setw(4) << conf_json;
	}
	catch(std::runtime_error& error) {
		std::cout << error.what() << std::endl;
		std::cout << "ShaderRetriever -i <filename> -s <vertex|fragment> -o <Output Filename> -v <true|false>" << std::endl;
		return 1;
	}
	return 0;
}