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
#include "config.h"
#include "shader_descriptor.h"

EShLanguage VKStageFlagToEShStage(VkShaderStageFlagBits stage) {
    switch (stage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT:
        return EShLangVertex;
        break;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return EShLangFragment;
        break;
    default:
        assert(false);
        break;
    }
    return EShLangCount;
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

void ConfigureShader(glslang::TShader* const p_shader, EShLanguage stage, Config& k_config) {
	p_shader->setEnvInput(
        k_config.getSource(),
        stage,
		glslang::EShClientVulkan,
		450
	);
	p_shader->setEnvClient(glslang::EShClientVulkan, 100);
	p_shader->setEnvTarget(glslang::EshTargetSpv, 0x00001000);
	bool result = ParseShader(p_shader, k_config.getMessages());
	assert(result);
}

int main(int argc, char** argv) {
    std::vector<glslang::TShader*> p_shaders;
	try {
        if (argc < 2) {
            throw std::exception("input must be specified.");
        }
        
		Config config(argv[1]);
		glslang::InitializeProcess();
        const auto& stages = config.getStages();
        uint32_t stage_count = config.getStages().size();
        p_shaders.resize(stage_count);
        
        for (uint32_t i = 0; i < stage_count; ++i) {
            auto sh_stage = VKStageFlagToEShStage(stages[i]);
            CreateShader(sh_stage, p_shaders[i]);
            if (p_shaders[i] == nullptr) {
                std::cout
                    << "Something is going wrong,"
                    << "memory allocation failed!"
                    << std::endl;
                return 1;
            }

            auto& srcs = LoadShaderSoruces({ config.shaderFilepaths[stages[i]] });
            std::vector<const char*> c_srcs;
            for (auto& s : srcs) {
                c_srcs.push_back(s.c_str());
            }
            p_shaders[i]->setStrings(c_srcs.data(), c_srcs.size());
            p_shaders[i]->setEntryPoint(config.shaderEntrys[stages[i]].c_str());
            ConfigureShader(p_shaders[i], sh_stage, config);
        }

		glslang::TProgram program;

        for (auto p_shader : p_shaders) {
            program.addShader(p_shader);
        }

		bool result = InitializeProgram(program, config.getMessages());
		assert(result);
		
        ShaderDescriptor shader_descriptor;
        shader_descriptor.processProgram(program, config);

        for (int i = 0; i < stage_count; ++i) {
            std::vector<unsigned int> spirv;
            glslang::GlslangToSpv(*program.getIntermediate(VKStageFlagToEShStage(stages[i])), spirv);
            std::ofstream sr_file(config.getShaderBinFilename(stages[i]), std::ios::binary);
            if (!sr_file.is_open()) {
                std::cout
                    << "Something is going wrong,"
                    << "file can not be loaded!"
                    << std::endl;
                return 1;
            }
            sr_file.write((char*)spirv.data(), spirv.size() * sizeof(unsigned int));
            sr_file.close();
        }

		glslang::FinalizeProcess();
        shader_descriptor.writeFile(config.getShaderDescriptorFilename());
	}

	catch(std::runtime_error& error) {
		std::cout << error.what() << std::endl;
		std::cout << "ShaderRetriever -i <filename> -s <vertex|fragment> -o <Output Filename> -v <true|false>" << std::endl;
	}

    for (auto p_shader : p_shaders) {
        if(p_shader)
            delete p_shader;
    }

	return 0;
}