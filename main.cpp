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
#include "spv_program.h"
#include "shader_descriptor.h"

int main(int argc, char** argv) {
    std::vector<glslang::TShader*> p_shaders;
    std::vector<glslang::TShader*> p_pc_shaders;
	try {
        if (argc < 2) {
            throw std::exception("input must be specified.");
        }
        
		Config config(argv[1]);
		glslang::InitializeProcess();
        const auto& stages = config.getStages();
        uint32_t stage_count = config.getStages().size();
        p_shaders.resize(stage_count);
        p_pc_shaders.resize(stage_count);
        
        for (uint32_t i = 0; i < stage_count; ++i) {
            auto sh_stage = VKStageFlagToEShStage(stages[i]);
            CreateShader(sh_stage, p_shaders[i]);
            CreateShader(sh_stage, p_pc_shaders[i]);
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

            p_pc_shaders[i]->setStrings(c_srcs.data(), c_srcs.size());
            p_pc_shaders[i]->setEntryPoint(config.shaderEntrys[stages[i]].c_str());
            ConfigureShader(p_pc_shaders[i], sh_stage, config);
        }

		glslang::TProgram program;

        for (auto p_shader : p_shaders) {
            program.addShader(p_shader);
        }

		bool result = InitializeProgram(program, config.getMessages());
		assert(result);
		
        ShaderDescriptor shader_descriptor;
        for (auto p_shader : p_pc_shaders) {
            shader_descriptor.buildPushConstants(p_shader, config);
            if (p_shader)
                delete p_shader;
        }
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