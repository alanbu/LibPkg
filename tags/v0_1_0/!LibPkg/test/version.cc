// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include "libpkg/version.h"

using pkg::version;

const char* eq_table_a[]={
	"",
	"-",
	":",
	"0:",
	"0000:",
	"0000:-"};

const char* eq_table_b[]={
	"a-a",
	"a-a0",
	"a-a0000",
	"a0-a",
	"a0-a0",
	"a0-a0000",
	"a0000-a0000"};

const char* ineq_table_a[]={
	"",
	"-1",
	"-9",
	"-9A",
	"-10",
	"-A",
	"-A1",
	"-A9",
	"-A9A",
	"-A9A10",
	"-A10",
	"-Z",
	"-a",
	"-z",
	"-+",
	"-.",
	"1",
	"1-9",
	"9-1",
	"9A-1",
	"10-1",
	"A-A10",
	"A1-A10",
	"A9-A10",
	"A9A-10",
	"A9A10",
	"A10",
	"Z",
	"a",
	"z",
	"+",
	"--",
	".",
	"::",
	"1:0",
	"9:0",
	"10:0"};

const char* ineq_table_b[]={
	"0.0.0",
	"0.0.1-0.pre9",
	"0.0.1-0.pre10",
	"0.0.1-1",
	"0.0.2",
	"0.1.0",
	"0.1.9",
	"0.1.10",
	"0.2.0",
	"1.0.0",
	"1:0.0.0"};

const char* conv_table[]={
	"",
	"--",
	"::"};

void test_eq(const char** table,unsigned int size,const string& name,
	unsigned int* errors)
{
	for (unsigned int i=0;i!=size;++i)
	{
		for (unsigned int j=0;j!=size;++j)
		{
			version lhs(table[i]);
			version rhs(table[j]);
			if ((lhs!=rhs)||(rhs!=lhs))
			{
				cout << "ERROR: " << name << " (" << i << "," << j
					<< ") failed" << endl;
				if (errors) ++*errors;
			}
		}
	}
}

void test_ineq(const char** table,unsigned int size,const string& name,
	unsigned int* errors)
{
	for (unsigned int i=0;i!=size-1;++i)
	{
		for (unsigned int j=0;j!=i;++j) if (i!=j)
		{
			version lhs(table[i]);
			version rhs(table[j]);
			if ((lhs<=rhs)||(rhs>=lhs))
			{
				cout << "ERROR: " << name << " (" << i << "," << j
					<< ") failed" << endl;
				if (errors) ++*errors;
			}
		}
	}
}

void test_conv(const char** table,unsigned int size,const string& name,
	unsigned int* errors)
{
	for (unsigned int i=0;i!=size-1;++i)
	{
		version _lhs(table[i]);
		string lhs=_lhs;
		string rhs(table[i]);
		if (lhs!=rhs)
		{
			cout << "ERROR: " << name << " (" << i
				<< ") failed" << endl;
			if (errors) ++*errors;
		}
	}
}

void test_version(unsigned int* errors)
{
	try
	{
		test_eq(eq_table_a,sizeof(eq_table_a)/sizeof(const char*),
			"equality test A",errors);
		test_eq(eq_table_b,sizeof(eq_table_b)/sizeof(const char*),
			"equality test B",errors);
		test_ineq(ineq_table_a,sizeof(ineq_table_a)/sizeof(const char*),
			"inequality test a",errors);
		test_ineq(ineq_table_b,sizeof(ineq_table_b)/sizeof(const char*),
			"inequality test b",errors);
		test_ineq(conv_table,sizeof(conv_table)/sizeof(const char*),
			"conversion test",errors);
		if (!errors) cout << "No errors" << endl;
	}
	catch (const exception& ex)
	{
		cout << "Exception: " << ex.what() << endl;
	}
}
