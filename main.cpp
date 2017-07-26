#include <iostream>
#include <fstream>
#include <exception>
#include <streambuf>
#include <new>
#include <cstring>
#include <cassert>
#include <map>
#include <vector>
#include <ShaderLang.h>
#include <Types.h>
#include <GlslangToSpv.h>
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
		json["push_constant"] = 1;
}

int GetTypeDef(const bool vulkan_def, const int attri_type) {
	return vulkan_def ? AttributeGL2Vulkan(attri_type) : attri_type;
}

void WriteBasicType(JSON& j, const glslang::TType& type) {
	j["basic_type"] = type.getBasicTypeString().c_str();
}

void WriteAttributesJSON(const glslang::TProgram& program, JSON& config_json, const bool vulkan_def) {
	JSON attributes_json;
	auto attributes_size = program.getNumLiveAttributes();
	for (int i = 0; i < attributes_size; ++i) {
		JSON attri_json;
		auto attribute_type = program.getAttributeType(i);
		attri_json["type"] = GetTypeDef(vulkan_def, attribute_type);

		const auto& type = program.getAttributeTType(i);
		WriteBasicType(attri_json, *type);
		attri_json["vector_size"] = type->getVectorSize();

		const auto& qualifier = type->getQualifier();
		SetQualifier(qualifier, attri_json);

		attributes_json[program.getAttributeName(i)] = attri_json;
	}
	if (attributes_size > 0)
		config_json["attributes"] = attributes_json;
}

void WriteUniformBlocksJSON(const glslang::TProgram& program, JSON& config_json) {
	JSON uniform_blocks_json;
	auto uniform_blks_size = program.getNumLiveUniformBlocks();
	for (int i = 0; i < uniform_blks_size; ++i) {
		JSON uniform_blk_json;
		uniform_blk_json["block_size"] = program.getUniformBlockSize(i);

		const auto& type = program.getUniformBlockTType(i);
		WriteBasicType(uniform_blk_json, *type);

		const auto& qualifier = type->getQualifier();
		SetQualifier(qualifier, uniform_blk_json);

		uniform_blocks_json[program.getUniformBlockName(i)] = uniform_blk_json;
	}
	if (uniform_blks_size > 0)
		config_json["uniform_blocks"] = uniform_blocks_json;
}

void WriteUniformVariablesJSON(const glslang::TProgram& program, JSON& config_json) {
	JSON uniform_variables_json;
	auto uniform_vars_size = program.getNumLiveUniformVariables();
	for (int i = 0; i < uniform_vars_size; ++i) {
		JSON uniform_var_json;

		const auto& type = program.getUniformTType(i);
		uniform_var_json["basic_type"] = type->getBasicTypeString().c_str();
		if (type->getBasicType() == glslang::EbtSampler) {
			auto sampler = type->getSampler();
			JSON sampler_json;
			sampler_json["dim"] = sampler.dim;
			sampler_json["type"] = sampler.type;
			uniform_var_json["sampler"] = sampler_json;
		}

		const auto& qualifier = type->getQualifier();
		SetQualifier(qualifier, uniform_var_json);

		uniform_variables_json[program.getUniformName(i)] = uniform_var_json;
	}
	if (uniform_vars_size > 0)
		config_json["uniform_blocks"] = uniform_variables_json;
}

EShLanguage GetStage(Config::OptionValue stage) {
	switch (stage)
	{
	case Config::VERTEX:
		return EShLanguage::EShLangVertex;
	case Config::FRAGMENT:
		return EShLanguage::EShLangFragment;
	default:
		throw std::runtime_error("stage is not mapped.");
	}
}

void CreateShader(EShLanguage stage, glslang::TShader*& p_shader) {
	p_shader = new(std::nothrow) glslang::TShader(stage);
}

std::vector<std::string> LoadShaderSoruces(std::vector<std::string> file_paths) {
	std::vector<std::string> ret;
	for (auto path : file_paths) {
		std::ifstream file(path);
		if (!file.is_open()) {
			break;
		}

		EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
		TBuiltInResource resources;
		InitializeResources(resources);
		std::string shader_content(
			(std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>()
		);
		ret.push_back(shader_content);
		file.close();
	}
	return ret;
}

bool ParseShader(glslang::TShader* p_shader) {
	EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
	TBuiltInResource resources;
	InitializeResources(resources);

	if (!p_shader->parse(&resources, 100, false, messages)) {
		std::cout << p_shader->getInfoLog() << std::endl;
		std::cout << p_shader->getInfoDebugLog() << std::endl;
		return false;
	}
	return true;
}

bool InitializeProgram(glslang::TProgram& program) {
	EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
	if (!program.link(messages)) {
		return false;
	}
	if (!program.buildReflection()) {
		return false;
	}
	return true;
}

int main(int argc, char** argv) {
	glslang::TShader* p_shader = nullptr;
	try {
		Config config(argc, argv);
		glslang::InitializeProcess();
		auto stage = GetStage(config.getStage());
		CreateShader(stage, p_shader);
		if (p_shader == nullptr) {
			std::cout
				<< "Something is going wrong,"
				<< "memory allocation failed!"
				<< std::endl;
			return 1;
		}

		auto& srcs = LoadShaderSoruces({ config.getInputFilename() });
		std::vector<const char*> c_srcs;
		for (auto& s : srcs) {
			c_srcs.push_back(s.c_str());
		}
		p_shader->setStrings(c_srcs.data(), c_srcs.size());
		assert(ParseShader(p_shader));

		glslang::TProgram program;
		program.addShader(p_shader);
		assert(InitializeProgram(program));
		
		JSON conf_json;

		WriteAttributesJSON(program, conf_json, config.isVulkanDef());
		WriteUniformBlocksJSON(program, conf_json);
		WriteUniformVariablesJSON(program, conf_json);

		std::vector<unsigned int> spirv;
		glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);

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
	}
	if (p_shader)
		delete p_shader;
	return 0;
}