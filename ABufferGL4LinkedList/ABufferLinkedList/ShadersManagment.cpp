/**
 * Fast Single-pass A-Buffer using OpenGL 4.0 V2.0
 * Copyright Cyril Crassin, July 2010
**/

#include "ShadersManagment.h"


#include <string>
#include <iostream>
#include <fstream>
#include <vector>

///////////////////////////////////////////

void defineMacro(std::string &shaderSource, const char *macro, const char *value);
void defineMacro(std::string &shaderSource, const char *macro, int val);

std::string manageIncludes(std::string src, std::string sourceFileName);

///////////////////////////////////////////

//Size of the string, the shorter is better
#define STRING_BUFFER_SIZE 2048
char stringBuffer[STRING_BUFFER_SIZE];


struct ShaderMacroStruct{
	std::string macro;
	std::string value;
};
std::vector<ShaderMacroStruct>	shadersMacroList;

void resetShadersGlobalMacros(){
	shadersMacroList.clear();
}

void setShadersGlobalMacro(const char *macro, int val){
	ShaderMacroStruct ms;
	ms.macro=std::string(macro);

	char buff[128];
	sprintf(buff, "%d", val);
	ms.value=std::string(buff);

	shadersMacroList.push_back(ms);
}
void setShadersGlobalMacro(const char *macro, float val){
	ShaderMacroStruct ms;
	ms.macro=std::string(macro);

	char buff[128];
	sprintf(buff, "%ff", val);

	ms.value=std::string(buff);

	shadersMacroList.push_back(ms);
}

//GLSL shader program creation
GLuint createShaderProgram(const char *fileNameVS, const char *fileNameGS, const char *fileNameFS, GLuint programID){
	bool reload=programID!=0;

	GLuint vertexShaderID=0;
	GLuint geometryShaderID=0;
	GLuint fragmentShaderID=0;

	if(!reload){
		// Create GLSL program
		programID=glCreateProgram();
	}else{
		GLsizei count;
		GLuint shaders[3];
		glGetAttachedShaders(programID, 3, &count, shaders);

		for(int i=0; i<count; i++){
			GLint shadertype;
			glGetShaderiv(	shaders[i], GL_SHADER_TYPE, &shadertype);
			if(shadertype == GL_VERTEX_SHADER){
				vertexShaderID=shaders[i];
			}else if(shadertype == GL_GEOMETRY_SHADER){
				geometryShaderID=shaders[i];
			}else if(shadertype == GL_FRAGMENT_SHADER){
				fragmentShaderID=shaders[i];
			}
		}
	}


	if(fileNameVS){
		// Create vertex shader
		vertexShaderID=createShader(fileNameVS, GL_VERTEX_SHADER, vertexShaderID);
		if(!reload){
			// Attach vertex shader to program object
			glAttachShader(programID, vertexShaderID);
		}
	}

	if(fileNameGS){
		// Create geometry shader
		geometryShaderID=createShader(fileNameGS, GL_GEOMETRY_SHADER, geometryShaderID);
		if(!reload){
			// Attach vertex shader to program object
			glAttachShader(programID, geometryShaderID);
		}
	}
	
	if(fileNameFS){
		// Create fragment shader
		fragmentShaderID=createShader(fileNameFS, GL_FRAGMENT_SHADER, fragmentShaderID);
		if(!reload){
			// Attach fragment shader to program object
			glAttachShader(programID, fragmentShaderID);
		}
	}
	

	return programID;
}

void linkShaderProgram(GLuint programID){
	int linkStatus;
	glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus);
	//if(!linkStatus){
		// Link all shaders togethers into the GLSL program
		glLinkProgram(programID);
		checkProgramInfos(programID, GL_LINK_STATUS);

		// Validate program executability giving current OpenGL states
		glValidateProgram(programID);
		checkProgramInfos(programID, GL_VALIDATE_STATUS);
		//std::cout<<"Program "<<programID<<" linked\n";
	//}
}


//GLSL shader creation (of a certain type, vertex shader, fragment shader oe geometry shader)
GLuint createShader(const char *fileName, GLuint shaderType, GLuint shaderID){
	if(shaderID==0){
		shaderID=glCreateShader(shaderType);
	}
	
	std::string shaderSource=loadTextFile(fileName);

	//Manage #includes
    shaderSource=manageIncludes(shaderSource, std::string(fileName));

    //Define global macros
	for(unsigned int i=0; i<shadersMacroList.size(); i++){
		defineMacro(shaderSource, shadersMacroList[i].macro.c_str(), shadersMacroList[i].value.c_str());
	}

	//Passing shader source code to GL
	//Source used for "shaderID" shader, there is only "1" source code and the string is NULL terminated (no sizes passed)
	const char *src=shaderSource.c_str();
	glShaderSource(shaderID, 1, &src, NULL);

	//Compile shader object
	glCompileShader(shaderID);

	//Check compilation status
	GLint ok;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &ok);
	if (!ok){
		int ilength;
		glGetShaderInfoLog(shaderID, STRING_BUFFER_SIZE, &ilength, stringBuffer);
		std::cout<<"Compilation error ("<<fileName<<") : "<<stringBuffer; 
	}

	return shaderID;
}


//Text file loading for shaders sources
std::string loadTextFile(const char *name){
	//Source file reading
	std::string buff("");
	std::ifstream file;
	file.open(name);

	if(file.fail())
		std::cout<<"loadFile: unable to open file: "<<name;
	
	buff.reserve(1024*1024);

	std::string line;
	while(std::getline(file, line)){
		buff += line + "\n";
	}

	const char *txt=buff.c_str();

	return std::string(txt);
}

void checkProgramInfos(GLuint programID, GLuint stat){
	GLint ok = 0;
	glGetProgramiv(programID, stat, &ok);
	if (!ok){
		int ilength;
		glGetProgramInfoLog(programID, STRING_BUFFER_SIZE, &ilength, stringBuffer);
		std::cout<<"Program error :\n"<<stringBuffer<<"\n"; 
	}
}

void defineMacro(std::string &shaderSource, const char *macro, const char *value){
	char buff[512];


	sprintf(buff, "#define %s", macro);

	int mstart = (int)shaderSource.find(buff);
	sprintf(buff, "#define %s %s\n", macro, value);
	if(mstart>=0){
		//std::cout<<"Found at "<<mstart<<"\n";
		int mlen = (int)shaderSource.find("\n", mstart)-mstart+1 ;
		std::string prevval=shaderSource.substr(mstart, mlen);
		if( strcmp(prevval.c_str(), buff ) ){
			shaderSource.replace(mstart, mlen, buff);
		}
	}else{
		shaderSource.insert(0, buff);
	}

}


std::string manageIncludes(std::string src, std::string sourceFileName){
	std::string res;
	res.reserve(100000);

	char buff[512];
	sprintf(buff, "#include");


	size_t includepos = src.find(buff, 0);

	while(includepos!=std::string::npos){
		bool comment=src.substr(includepos-2, 2)==std::string("//");

		if(!comment){
			
			size_t fnamestartLoc = src.find("\"", includepos);
			size_t fnameendLoc = src.find("\"", fnamestartLoc+1);

			size_t fnamestartLib = src.find("<", includepos);
			size_t fnameendLib = src.find(">", fnamestartLib+1);

			size_t fnameEndOfLine = src.find("\n", includepos);

			size_t fnamestart;
			size_t fnameend;

			bool uselibpath=false;
			if( (fnamestartLoc == std::string::npos || fnamestartLib < fnamestartLoc) && fnamestartLib < fnameEndOfLine){
				fnamestart=fnamestartLib;
				fnameend=fnameendLib;
				uselibpath=true;
			}else if(fnamestartLoc != std::string::npos && fnamestartLoc < fnameEndOfLine){
				fnamestart=fnamestartLoc;
				fnameend=fnameendLoc;
				uselibpath=false;
			}else{
                std::cerr<<"manageIncludes : invalid #include directive into \""<<sourceFileName.c_str()<<"\"\n";
				return src;
			}

			std::string incfilename=src.substr(fnamestart+1, fnameend-fnamestart-1);
			std::string incsource;

			if(uselibpath){
				std::string usedPath;

				//TODO: Add paths types into the manager -> search only onto shaders paths.
				std::vector<std::string> pathsList;
				//ResourcesManager::getManager()->getPaths(pathsList);
                pathsList.push_back("./");

				for(std::vector<std::string>::iterator it= pathsList.begin(); it!= pathsList.end(); it++){
					std::string fullpathtmp=(*it)+incfilename;
					
					FILE *file=0;
					file=fopen(fullpathtmp.c_str(), "r");
					if(file){
						usedPath=(*it);
						fclose(file);
						break;
					}else{
						usedPath="";
					}
				}
				
				if(usedPath != ""){
					incsource=loadTextFile( (usedPath + incfilename ).c_str() );
				}else{
                    std::cerr   <<"manageIncludes : Unable to find included file \""
                                <<incfilename.c_str()<<"\" in system paths.\n";
					return src;
				}
			}else{
				incsource=loadTextFile(
					( sourceFileName.substr(0, sourceFileName.find_last_of("/", sourceFileName.size())+1 )
						+ incfilename ).c_str() 
				);
			}


			incsource=manageIncludes(incsource, sourceFileName);
			incsource = incsource.substr(0, incsource.size()-1);
			
			std::string preIncludePart=src.substr(0, includepos);
			std::string postIncludePart=src.substr(fnameend+1, src.size()-fnameend);

			int numline=0;
			size_t newlinepos=0;
			do{
				newlinepos=preIncludePart.find("\n", newlinepos+1);
				numline++;
			}while(newlinepos!=std::string::npos);
			numline--;
			
			char buff2[512];
			sprintf(buff2, "\n#line 0\n");
			std::string linePragmaPre(buff2);
			sprintf(buff2, "\n#line %d\n", numline);
			std::string linePragmaPost(buff2);


			res=preIncludePart+ linePragmaPre+incsource + linePragmaPost + postIncludePart;

			src=res;
		}
		includepos = src.find(buff, includepos+1);
	}

	return src;
}