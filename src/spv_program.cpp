#include <iostream>
#include <fstream>
#include <exception>
#include <streambuf>
#include <new>
#include <cstring>
#include <cassert>
#include <map>
#include <vector>
#include "spv_program.h"
#include "config.h"

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