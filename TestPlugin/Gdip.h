#pragma once
#include<string>
#include<vector>

struct FuncInfo
{
	std::string Module;
	std::string FuncName;
	void* Func;
};

std::vector<FuncInfo> GetOverrideFunctions();
