// This file is part of LibPkg.
//
// Copyright 2003-2020 Graham Shaw
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdlib>
#include <list>
#include <map>
#include <fstream>

#include "libpkg/os/os.h"

#include "libpkg/filesystem.h"
#include "libpkg/dirstream.h"
#include "libpkg/path_table.h"
#include "libpkg/pkgbase.h"
#include "libpkg/sysvars.h"

namespace {

using std::string;

/** Convert filename to system variable name.
 * Variable names are represented using quoted-printable encoding
 * with "=" as the escape character.
 */
string filename_to_varname(const string& filename)
{
	string varname;
	enum state_type
	{
		state_normal,
		state_escape1,
		state_escape2
	};
	state_type state=state_normal;
	for (string::const_iterator i=filename.begin();i!=filename.end();++i)
	{
		switch (state)
		{
		case state_normal:
			varname+=*i;
			if (*i=='=') state=state_escape1;
			break;
		case state_escape1:
			varname+=*i;
			if (*i=='=') state=state_escape1;
			else if (isxdigit(*i)) state=state_escape2;
			else state=state_normal;
			break;
		case state_escape2:
			varname+=*i;
			if (*i=='=') state=state_escape1;
			else if (isxdigit(*i))
			{
				char hex[3];
				hex[0]=varname[varname.length()-2];
				hex[1]=varname[varname.length()-1];
				hex[2]=0;
				varname[varname.length()-3]=strtol(hex,0,16);
				varname.resize(varname.length()-2);
				state=state_normal;
			}
			else state=state_normal;
			break;
		}
	}
	return varname;
}

}; /* anonymous namespace */

namespace pkg {

// At present update_sysvars() does not immediately unset variables that
// are no longer in use.  This could be done, but it is not entirely clear
// that it is the correct/desirable behaviour.  Certainly, leaving them
// set is easier to implement and does little if any harm.

void update_sysvars(pkgbase& pb)
{
	// Filetype definitions.
	enum
	{
		filetype_Obey=0xfeb,
		filetype_Absolute=0xff8,
		filetype_BASIC=0xffb,
		filetype_Utility=0xffc,
		filetype_Data=0xffd,
		filetype_Command=0xffe,
		filetype_Text=0xfff
	};

	// Set pathnames.
	string dst_pathname=pb.setvars_pathname();
	string tmp_pathname=dst_pathname+string("++");
	string bak_pathname=dst_pathname+string("--");
	string apps_pathname=pb.paths()("Apps","");

	// Create map of system variable names to values;
	std::map<string,string> values;

	if (object_type(pb.sysvars_pathname())!=0)
	{
		// Create list of system variable definition files.
		std::list<dirstream::object> objects;
		{
			dirstream ds(pb.sysvars_pathname());
			while (ds)
			{
				dirstream::object obj;
				ds >> obj;
				objects.push_back(obj);
			}
		}

		// Evaluate system variable definition files (static ones first).
		for (std::list<dirstream::object>::const_iterator i=objects.begin(),
			i_end=objects.end();i!=i_end;++i)
		{
			const dirstream::object& obj=*i;
			string varname=filename_to_varname(obj.name);
			string pathname=pb.sysvars_pathname()+string(".")+obj.name;
			switch (obj.filetype)
			{
			case filetype_Data:
				{
					// If file is a data file then use
					// literal file content as value.
					std::ifstream in(pathname.c_str());
					string varval;
					getline(in,varval);
					values[varname]=varval;
				}
			case filetype_Text:
				{
					// If file is a text file then use
					// file content as value after resolving pathrefs.
					std::ifstream in(pathname.c_str());
					string varval;
					getline(in,varval);
					varval=resolve_pathrefs(pb.paths(),varval);
					values[varname]=varval;
				}
				break;
			default:
				/* no action. */
				break;
			}
		}

		// Evaluate system variable definition files (dynamic ones last).
		for (std::list<dirstream::object>::const_iterator i=objects.begin(),
			i_end=objects.end();i!=i_end;++i)
		{
			const dirstream::object& obj=*i;
			string varname=filename_to_varname(obj.name);
			string pathname=pb.sysvars_pathname()+string(".")+obj.name;
			switch (obj.filetype)
			{
			case filetype_Obey:
			case filetype_Absolute:
			case filetype_BASIC:
			case filetype_Utility:
			case filetype_Command:
				{
					// If file is executable then execute it,
					// use stdout as value.
					std::ifstream in("pipe:riscpkg.sysvar");
					string command=pathname+
						string(" { > pipe:riscpkg.sysvar }");
					if (!std::system(command.c_str()))
					{
						string varval;
						getline(in,varval);
						values[varname]=varval;
					}
				}
			default:
				/* no action. */
				break;
			}
		}
	}

	// Create obey file and set live variables.
	std::ofstream out(tmp_pathname.c_str());
	out << "| This file is automatically generated by !RiscPkg." << std::endl;
	out << "| Alterations will not be preserved." << std::endl;
	out << std::endl;
	out << "Set Packages$Apps " << apps_pathname << std::endl;
	for (std::map<string,string>::const_iterator i=values.begin(),
		i_end=values.end();i!=i_end;++i)
	{
		string varname=i->first;
		string varval=i->second;
		pkg::os::OS_SetVarVal(varname.c_str(),varval.c_str(),varval.length(),
			0,0,0,0);
		string::size_type j=varval.find('%',0);
		while (j!=string::npos)
		{
			varval.insert(j,1,'%');
			j=varval.find('%',j+2);
		}
		out << "Set " << varname << " \"" << varval << "\"" << std::endl;
	}
	out.close();
	write_filetype(tmp_pathname,filetype_Obey);

	// Backup existing obey file if it exists.
	if (object_type(dst_pathname)!=0)
	{
		force_move(dst_pathname,bak_pathname,true);
	}

	// Move new obey file to destination.
	force_move(tmp_pathname,dst_pathname,false);

	// Delete backup.
	force_delete(bak_pathname);
}

}; /* namespace pkg */
