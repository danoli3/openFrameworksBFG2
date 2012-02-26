/*
 * CBLinuxProject.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#ifndef CBLINUXPROJECT_H_
#define CBLINUXPROJECT_H_

#include "ofConstants.h"
#include "pugixml.hpp"
#include "ofAddon.h"
#include "baseProject.h"

class CBLinuxProject: public baseProject {
public:

	enum Arch{
		Linux,
		Linux64
	};

    CBLinuxProject(Arch arch);
    CBLinuxProject();

    void setArch(Arch arch);
	
    void setup(string ofRoot= "../../../");

	bool load(string path);
	bool create(string path);
	bool save(string path){return true;}

	void addSrc(string srcName, string folder);
	void addInclude(string includeName);
	void addLibrary(string libraryName);

	void addAddon(ofAddon & addon);

	string getName();
	string getPath();

	static string LOG_NAME;

private:
	
    //void parseAddons();

	pugi::xml_document doc;
	string projectDir, projectName;
	vector<ofAddon> addons;
	string ofRoot;
	Arch arch;
};

#endif /* CBLINUXPROJECT_H_ */
