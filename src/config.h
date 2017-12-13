#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <exception>
#include <map>
#include <cassert>
#include <ShaderLang.h>
#include <ResourceLimits.h>
#include <json.hpp>
#include <vulkan/vulkan.h>

extern const TBuiltInResource DefaultTBuiltInResource;

class Config {
public:
    enum LanguageDef {
		UNKNOWN,
		OPENGL,
		VULKAN,
        HLSL
	};

	Config() = delete;

	Config(char* argv) {
        using JSON = nlohmann::json;
        std::string filepath(argv);
        std::ifstream config_file(filepath);
        if (!config_file.is_open()) {
            throw std::exception("file open failed.");
        }
        JSON json;
        config_file >> json;

        if (json.count("sources") == 0) {
            throw std::exception("shader must exists.");
        }

        for (auto& obj : json["sources"].get<JSON::object_t>()) {
            auto stage = VK_SHADER_STAGE_ALL;
            if (obj.first == "fragment") {
                stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                m_stages.push_back(stage);
            }
            else if (obj.first == "vertex") {
                stage = VK_SHADER_STAGE_VERTEX_BIT;
                m_stages.push_back(stage);
            }
            else {
                assert(false);
            }
            shaderFilepaths[stage] = obj.second["path"].get<std::string>();
            if (obj.second.count("entry") == 0) {
                shaderEntrys[stage] = "main";
            }
            else {
                shaderEntrys[stage] = obj.second["entry"].get<std::string>();
            }
        }
        
        if (json.count("vulkan_define") > 0) {
            if(json["vulkan_define"].get<bool>())
                m_language_def = VULKAN;
            else
                m_language_def = OPENGL;
        }
        else {
            m_language_def = OPENGL;
        }

        if (json.count("spv") > 0) {
            spv_path = json["spv"].get<std::string>();
        }
        else {
            spv_path = "output.spv";
        }

        if (json.count("descriptor") > 0) {
            sd_path = json["descriptor"].get<std::string>();
        }
        else {
            sd_path = "output.sd";
        }

        switch (m_language_def)
        {
        case VULKAN:
            m_messages = (EShMessages)((unsigned long)m_messages | EShMsgSpvRules | EShMsgVulkanRules);
            break;
        case HLSL:
            m_messages = (EShMessages)((unsigned long)m_messages | EShMsgSpvRules | EShMsgVulkanRules | EShMsgReadHlsl | EShMsgHlslOffsets);
            break;
        default:
            assert(true);
            break;
        }
	}

	~Config() {

	};

	bool isVulkanDef() const {
        return m_language_def == VULKAN;
	}

    bool isHLSLDef() const {
        return m_language_def == HLSL;
    }

    glslang::EShSource getSource() const {
        if (isHLSLDef()) {
            return glslang::EShSourceHlsl;
        }
        return glslang::EShSourceGlsl;
    }

	std::string getShaderDescriptorFilename() const { return sd_path; }
	std::string getShaderBinFilename(VkShaderStageFlagBits stage) const {
        std::string app = "";
        switch (stage)
        {
        case VK_SHADER_STAGE_VERTEX_BIT:
            app = "vert";
            break;
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            app = "tesc";
            break;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            app = "tese";
            break;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            app = "geom";
            break;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            app = "frag";
            break;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            app = "comp";
            break;
        case VK_SHADER_STAGE_ALL_GRAPHICS:
        case VK_SHADER_STAGE_ALL:
        default:
            assert(false);
            break;
        }
        return spv_path + "." + app + ".sr";
    }
    const std::vector<VkShaderStageFlagBits>& getStages() { return m_stages; }
    EShMessages getMessages() const { return m_messages; }

    std::map<VkShaderStageFlagBits, std::string> shaderFilepaths;
    std::map<VkShaderStageFlagBits, std::string> shaderEntrys;
private:
    LanguageDef m_language_def;
    std::string spv_path;
    std::string sd_path;
    EShMessages m_messages = (EShMessages)(EShMsgDefault);
    std::vector<VkShaderStageFlagBits> m_stages;
};