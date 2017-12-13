#pragma once
#include <vector>
#include <json.hpp>
#include <ShaderLang.h>
#include <GlslangToSpv.h>
#include <vulkan/vulkan.h>
#include "gl2vulkan.h"
using JSON = nlohmann::json;

class Config;

class ShaderDescriptor {
public:
    ShaderDescriptor();
    ~ShaderDescriptor() {};

    void processProgram(glslang::TProgram& program, Config& config);
    void writeFile(std::string filename);

private:
    void setQualifier(const glslang::TQualifier& qualifier, JSON& json);
    int getTypeDef(const bool vulkan_def, const int attri_type);
    void writeBasicType(JSON& j, const glslang::TType& type);
    void writeAttributesJSON(const glslang::TProgram& program, JSON& config_json, const bool vulkan_def);
    void writeUniformBlocksJSON(const glslang::TProgram& program, JSON& config_json);
    void writeUniformVariablesJSON(const glslang::TProgram& program, JSON& config_json);

    JSON m_base;
    int m_attributes_count = 0;
    int m_uniform_blocks_count = 0;
    int m_uniform_variables_count = 0;
    JSON m_push_constants;
    std::vector<uint32_t> m_descriptor_pool;
};