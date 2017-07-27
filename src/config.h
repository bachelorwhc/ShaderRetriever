#pragma once
#include <string>
#include <vector>
#include <exception>
#include <map>
#include <ResourceLimits.h>

extern const TBuiltInResource DefaultTBuiltInResource;

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

		if (minus_count > 0) {
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
		if (m_options.count(DEF) > 0)
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