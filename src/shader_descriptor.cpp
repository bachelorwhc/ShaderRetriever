#include <iostream>
#include <fstream>
#include <exception>
#include "shader_descriptor.h"
#include "config.h"

ShaderDescriptor::ShaderDescriptor() : 
    m_descriptor_pool(VkDescriptorType::VK_DESCRIPTOR_TYPE_RANGE_SIZE) {

}

void ShaderDescriptor::processProgram(glslang::TProgram& program, Config& config) {
    JSON variables;
    writeAttributesJSON(program, variables, config.isVulkanDef() || config.isHLSLDef());
    writeUniformBlocksJSON(program, variables);
    writeUniformVariablesJSON(program, variables);
    variables["push_constant"] = m_push_constants;
    m_base["variables"] = variables;
    
    JSON brief;
    brief["attributes_count"] = m_attributes_count;
    brief["uniform_blocks_count"] = m_uniform_blocks_count;
    brief["uniform_variables_count"] = m_uniform_variables_count;
    m_base["brief"] = brief;
    m_base["descriptor_pool"] = m_descriptor_pool;

    const auto& stages = config.getStages();
    uint32_t stage_count = config.getStages().size();

    JSON spv_desc;
    for (auto s : stages) {
        spv_desc[config.getShaderBinFilename(s)] = s;
    }
    m_base["spvs"] = spv_desc;
}

void ShaderDescriptor::writeFile(std::string filename) {
    std::ofstream sd_file(filename);
    if (!sd_file.is_open()) {
        throw std::exception("Something is going wrong, file can not be loaded!");
    }
    sd_file << std::setw(4) << m_base;
    sd_file.close();
}

void ShaderDescriptor::setQualifier(const glslang::TQualifier& qualifier, JSON& json) {
    if (qualifier.hasBinding())
        json["binding"] = qualifier.layoutBinding;
    if (qualifier.hasLocation())
        json["location"] = qualifier.layoutLocation;
    if (qualifier.hasSet())
        json["set"] = qualifier.layoutSet;
}

int ShaderDescriptor::getTypeDef(const bool vulkan_def, const int attri_type) {
    return vulkan_def ? AttributeGL2Vulkan(attri_type) : attri_type;
}

void ShaderDescriptor::writeBasicType(JSON& j, const glslang::TType& type) {
    std::string basic_type(type.getBasicTypeString().c_str());
    std::size_t found = basic_type.find("sampler");
    if (found != std::string::npos) {
        const auto sampler = type.getSampler();
        if (sampler.combined) {
            ++m_descriptor_pool[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER];
        }
        else {
            ++m_descriptor_pool[VK_DESCRIPTOR_TYPE_SAMPLER];
        }
    }
    else if(basic_type == "block") {
        ++m_descriptor_pool[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER];
    }
    j["basic_type"] = type.getBasicTypeString().c_str();
}

void ShaderDescriptor::writeAttributesJSON(const glslang::TProgram& program, JSON& config_json, const bool vulkan_def) {
    JSON attributes_json;
    auto attributes_size = program.getNumLiveAttributes();
    for (int i = 0; i < attributes_size; ++i) {
        JSON attri_json;
        auto attribute_type = program.getAttributeType(i);
        attri_json["type"] = getTypeDef(vulkan_def, attribute_type);

        const auto& type = program.getAttributeTType(i);
        writeBasicType(attri_json, *type);
        attri_json["vector_size"] = type->getVectorSize();

        const auto& qualifier = type->getQualifier();
        setQualifier(qualifier, attri_json);

        if (qualifier.layoutPushConstant == false) {
            attributes_json[program.getAttributeName(i)] = attri_json;
        }
        else {
            m_push_constants[program.getAttributeName(i)] = attri_json;
        }
    }
    if (attributes_size > 0)
        config_json["attributes"] = attributes_json;
    m_attributes_count += attributes_size;
}

void ShaderDescriptor::writeUniformBlocksJSON(const glslang::TProgram& program, JSON& config_json) {
    JSON uniform_blocks_json;
    auto uniform_blks_size = program.getNumLiveUniformBlocks();
    for (int i = 0; i < uniform_blks_size; ++i) {
        JSON uniform_blk_json;
        uniform_blk_json["block_size"] = program.getUniformBlockSize(i);

        const auto& type = program.getUniformBlockTType(i);
        writeBasicType(uniform_blk_json, *type);

        const auto& qualifier = type->getQualifier();
        setQualifier(qualifier, uniform_blk_json);

        auto offset = program.getUniformBufferOffset(i);
        if (offset >= 0 && qualifier.layoutPushConstant == false) {
            uniform_blk_json["offset"] = offset;
        }

        auto name = program.getUniformBlockName(i);
        if (qualifier.layoutPushConstant == false) {
            uniform_blocks_json[program.getUniformBlockName(i)] = uniform_blk_json;
        }
        else {
            m_push_constants[program.getUniformBlockName(i)] = uniform_blk_json;
        }
    }
    if (uniform_blks_size > 0)
        config_json["uniform_blocks"] = uniform_blocks_json;
    m_uniform_blocks_count += uniform_blks_size;
}

void ShaderDescriptor::writeUniformVariablesJSON(const glslang::TProgram& program, JSON& config_json) {
    JSON uniform_variables_json;
    auto uniform_vars_size = program.getNumLiveUniformVariables();
    for (int i = 0; i < uniform_vars_size; ++i) {
        auto index = program.getUniformBlockIndex(i);
        if (index >= 0) {
            continue;
        }

        JSON uniform_var_json;

        const auto& type = program.getUniformTType(i);
        writeBasicType(uniform_var_json, *type);
        if (type->getBasicType() == glslang::EbtSampler) {
            auto sampler = type->getSampler();
            JSON sampler_json;
            sampler_json["dim"] = sampler.dim;
            sampler_json["type"] = sampler.type;
            sampler_json["combined"] = sampler.combined;
            uniform_var_json["sampler"] = sampler_json;
        }

        auto offset = program.getUniformBufferOffset(i);
        if (offset >= 0) {
            uniform_var_json["offset"] = offset;
        }

        const auto& qualifier = type->getQualifier();
        setQualifier(qualifier, uniform_var_json);

        uniform_variables_json[program.getUniformName(i)] = uniform_var_json;
    }
    if (uniform_vars_size > 0)
        config_json["uniform_variables"] = uniform_variables_json;
    m_uniform_variables_count += uniform_vars_size;
}