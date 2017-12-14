#pragma once
#include <ShaderLang.h>
#include <GlslangToSpv.h>
#include <vulkan/vulkan.h>

class Config;

EShLanguage VKStageFlagToEShStage(VkShaderStageFlagBits stage);

void CreateShader(EShLanguage stage, glslang::TShader*& p_shader);

std::vector<std::string> LoadShaderSoruces(std::vector<std::string> file_paths);

bool ParseShader(glslang::TShader* p_shader, const EShMessages e_messages);

bool InitializeProgram(glslang::TProgram& program, const EShMessages e_messages);

void ConfigureShader(glslang::TShader* const p_shader, EShLanguage stage, Config& k_config);