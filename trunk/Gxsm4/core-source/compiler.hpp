#pragma once

#include <GL/glew.h>
#include <glm/gtc/type_precision.hpp>

#include <map>
#include <string>
#include <vector>

std::string format(const char* Message, ...);
bool checkError(const char* Title);

class compiler
{
	typedef std::map<std::string, GLuint> names_map;
	typedef std::map<GLuint, std::string> files_map;

	class commandline
	{
		enum profile
		{
			CORE,
			COMPATIBILITY
		};

	public:
		commandline(std::string const & Filename, std::string const & Arguments);

		void parseArguments(std::string const & Arguments);

		int getVersion() const {return this->Version;}
		std::string getProfile() const {return this->Profile;}
		std::string getDefines() const;
		std::vector<std::string> getIncludes() const {return this->Includes;}

	private:
		std::string Profile;
		int Version;
		std::vector<std::string> Defines;
		std::vector<std::string> Includes;
	};

	class parser
	{
	public:
		std::string operator() (commandline const & CommandLine, std::string const & Filename) const;

	private:
		std::string parseInclude(std::string const & Line, std::size_t const & Offset) const;
	};

public:
	~compiler();

	GLuint create(GLenum Type, std::string const & Filename, std::string const & Arguments = std::string());
	bool destroy(GLuint const & Name);

	bool check_program(GLuint ProgramName) const;
	bool validate_program(GLuint ProgramName) const;

	bool check();
	// TODO: Not defined
	bool check(GLuint const & Name);
	void clear();

private:
	names_map ShaderNames;
	files_map ShaderFiles;
	names_map PendingChecks;
};

std::string load_file(std::string const & Filename);
bool load_binary(std::string const & Filename, GLenum & Format, std::vector<glm::uint8> & Data, GLint & Size);
bool save_binary(std::string const & Filename, GLenum const & Format, std::vector<glm::uint8> const & Data, GLint const & Size);
