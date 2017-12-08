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
#include <GlslangToSpv.h>
#include <json.hpp>
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

        auto offset = program.getUniformBufferOffset(i);
        if (offset >= 0) {
            uniform_blk_json["offset"] = offset;
        }

		const auto& qualifier = type->getQualifier();
		SetQualifier(qualifier, uniform_blk_json);

        auto name = program.getUniformBlockName(i);
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

        auto offset = program.getUniformBufferOffset(i);
        if (offset >= 0) {
            uniform_var_json["offset"] = offset;
        }

        auto index = program.getUniformBlockIndex(i);
        if (index >= 0) {
            uniform_var_json["index"] = index;
            auto block = program.getUniformBlockName(index);
            uniform_var_json["block_name"] = block;
        }

		const auto& qualifier = type->getQualifier();
		SetQualifier(qualifier, uniform_var_json);

		uniform_variables_json[program.getUniformName(i)] = uniform_var_json;
	}
	if (uniform_vars_size > 0)
		config_json["uniform_variables"] = uniform_variables_json;
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

		std::string shader_content(
			(std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>()
		);
		ret.push_back(shader_content);
		file.close();
	}
	return ret;
}

bool ParseShader(glslang::TShader* p_shader, const EShMessages e_messages) {
	if (!p_shader->parse(&DefaultTBuiltInResource, 100, false, e_messages)) {
		std::cout << p_shader->getInfoLog() << std::endl;
		std::cout << p_shader->getInfoDebugLog() << std::endl;
		return false;
	}
	return true;
}

bool InitializeProgram(glslang::TProgram& program, const EShMessages e_messages) {
	if (!program.link(e_messages)) {
        std::cout << program.getInfoLog() << std::endl;
        std::cout << program.getInfoDebugLog() << std::endl;
		return false;
	}
    if (!program.mapIO()) {
        std::cout << program.getInfoLog() << std::endl;
        std::cout << program.getInfoDebugLog() << std::endl;
        return false;
    }
	if (!program.buildReflection()) {
        std::cout << program.getInfoLog() << std::endl;
        std::cout << program.getInfoDebugLog() << std::endl;
		return false;
	}
    program.dumpReflection();
	return true;
}

void ConfigureShader(glslang::TShader* const p_shader, Config& k_config) {
	p_shader->setEnvInput(
        k_config.getSource(),
        k_config.getStage(),
		glslang::EShClientVulkan,
		450
	);
	p_shader->setEnvClient(glslang::EShClientVulkan, 100);
	p_shader->setEnvTarget(glslang::EshTargetSpv, 0x00001000);
	bool result = ParseShader(p_shader, k_config.getMessages());
	assert(result);
}

int main(int argc, char** argv) {
	glslang::TShader* p_shader = nullptr;
	try {
		Config config(argc, argv);
		glslang::InitializeProcess();
		auto stage = config.getStage();
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
        p_shader->setEntryPoint(config.getSourceEntryPoint().c_str());
		ConfigureShader(p_shader, config);

		glslang::TProgram program;

		program.addShader(p_shader);
		bool result = InitializeProgram(program, config.getMessages());
		assert(result);
		
		JSON conf_json;

		WriteAttributesJSON(program, conf_json, config.isVulkanDef() || config.isHLSLDef());
		WriteUniformBlocksJSON(program, conf_json);
		WriteUniformVariablesJSON(program, conf_json);

		std::vector<unsigned int> spirv;
		glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);

		glslang::FinalizeProcess();

		std::ofstream sri_file(config.getShaderInfoFilename());
		std::ofstream sr_file(config.getShaderBinFilename(), std::ios::binary);
		if(!sri_file.is_open() || !sr_file.is_open()) {
			std::cout
				<< "Something is going wrong,"
				<< "file can not be loaded!"
				<< std::endl;
			return 1;
		}
		sr_file.write((char*)spirv.data(), spirv.size() * sizeof(unsigned int));
		sri_file << std::setw(4) << conf_json;
		sri_file.close();
		sr_file.close();
	}
	catch(std::runtime_error& error) {
		std::cout << error.what() << std::endl;
		std::cout << "ShaderRetriever -i <filename> -s <vertex|fragment> -o <Output Filename> -v <true|false>" << std::endl;
	}
	if (p_shader)
		delete p_shader;
	return 0;
}