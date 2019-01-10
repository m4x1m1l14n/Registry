// Registry.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
	auto key = Registry::LocalMachine->Open(L"");

    return 0;
}

