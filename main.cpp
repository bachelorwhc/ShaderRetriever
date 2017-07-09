#include <iostream>
#include <fstream>
#include <exception>
#include <streambuf>
#include <new>
#include <cstring>
#include <map>
#include <glslang/ShaderLang.h>
#include <glslang/Types.h>
#include "init_resources.h"

class Config {
public:
	enum Option {
		DEF,
		STAGE
	};
	enum OptionValue {
		UNKNOWN,
		OPENGL,
		VULKAN,
		VERTEX,
		FRAGMENT
	};

	Config() = delete;
	
	Config(int argc, char** argv) {
		if (argc == 1) {
			throw std::runtime_error("input file should be set.");
		}
		std::vector<std::string> arguments(argc - 1);
		int minus_count = 0;

		for (int i = 1; i < argc; ++i) { // ignore the exe filename.
			arguments[i - 1] = argv[i];
			++minus_count;
		}

		if(minus_count > 0) {
			setOption(arguments);
		}

		if (isUnset(m_input)) {
			for (const auto& arg : arguments) {
				if (arg[0] != '-' && arg.size() > 0) {
					m_input = arg;
					break;
				}
			}
			if (isUnset(m_input))
				throw std::runtime_error("input file should be set.");
		}

		if (isUnset(DEF)) {
			m_options[DEF] = OPENGL;
		}

		if (isUnset(STAGE)) {
			throw std::runtime_error("stage should be set.");
		}
	}

	~Config() {
	
	};

	bool isVulkanDef() const { 
		if(m_options.count(DEF) > 0)
			return m_options.find(DEF)->second == VULKAN; 
		else
			throw std::runtime_error("stage should be set.");
		return false;
	}

	OptionValue getStage() const { 
		if (m_options.count(STAGE) > 0)
			return m_options.find(STAGE)->second;
		else 
			throw std::runtime_error("stage should be set.");
		return UNKNOWN;
	}
	std::string getInputFilename() const { return m_input; }
	std::string getOutputFilename() const { return m_output; }

private:
	void setOption(const std::vector<std::string>& arguments) {
		auto size = arguments.size();
		for (int i = 0; i < size;) {
			if (arguments[i][0] == '-' && arguments[i].size() == 2) {
				char option = arguments[i][1];
				const auto& argument = arguments[i + 1];
				if (option == 'o') { // OUTPUT FILE
					m_output = argument;
				}
				else if (option == 'i') { // INPUT FILE
					m_input = argument;
				}
				else if (option == 's') { // STAGE
					if (argument == "fragment") {
						m_options[STAGE] = FRAGMENT;
					}
					else if (argument == "vertex") {
						m_options[STAGE] = VERTEX;
					}
				}
				else if (option == 'v') { // IS VULKAN
					if (argument == "true") {
						m_options[DEF] = VULKAN;
					}
					else {
						m_options[DEF] = OPENGL;
					}
				}
				i += 2;
			}
			else
				++i;
		}
	}

	bool isUnset(const std::string& str) const {
		return str.empty();
	}

	bool isUnset(const Option option) const {
		return m_options.count(option) == 0;
	}

	std::map<Option, OptionValue> m_options;
	std::string m_input;
	std::string m_output;
};

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

		std::cout << "---------------------------------------------" << std::endl;
		std::cout << "Attributes:" << std::endl;
		auto attributes_size = program.getNumLiveAttributes();
		for (int i = 0; i < attributes_size; ++i) {
			std::cout << program.getAttributeName(i) << std::endl;
			std::cout << program.getAttributeType(i) << std::endl;
			const auto& type = program.getAttributeTType(i);
			std::cout << type->getVectorSize() << std::endl;
			const auto& qualifier = type->getQualifier();
			if (qualifier.hasBinding())
				std::cout << "binding: " << qualifier.layoutBinding << std::endl;
			if (qualifier.hasLocation())
				std::cout << "location: " << qualifier.layoutLocation << std::endl;
			if (qualifier.layoutPushConstant)
				std::cout << "PushConstant" << std::endl;
			std::cout << std::endl;
		}

		std::cout << "---------------------------------------------" << std::endl;
		std::cout << "UniformBlocks:" << std::endl;
		auto uniforms_size = program.getNumLiveUniformBlocks();
		for (int i = 0; i < uniforms_size; ++i) {
			std::cout << program.getUniformBlockName(i) << std::endl;
			std::cout << "block size:" << program.getUniformBlockSize(i) << std::endl;
			const auto& type = program.getUniformBlockTType(i);
			const auto& qualifier = type->getQualifier();
			if (qualifier.hasBinding())
				std::cout << "binding: " << qualifier.layoutBinding << std::endl;
			if (qualifier.hasLocation())
				std::cout << "location: " << qualifier.layoutLocation << std::endl;
			if (qualifier.layoutPushConstant)
				std::cout << "PushConstant" << std::endl;
			std::cout << std::endl;
		}

		glslang::FinalizeProcess();
	}
	catch(std::runtime_error& error) {
		std::cout << error.what() << std::endl;
		std::cout << "ShaderRetriever <filename> <vertex|fragment> <Output Filename>" << std::endl;
		return 1;
	}
	return 0;
}