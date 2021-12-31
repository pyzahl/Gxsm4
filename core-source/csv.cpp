///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Samples Pack (ogl-samples.g-truc.net)
///
/// Copyright (c) 2004 - 2014 G-Truc Creation (www.g-truc.net)
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
/// 
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////

#include "csv.hpp"
#include <cstdio>
#include <cstring>

std::string format(const char * Message, ...)
{
	assert(Message);
	
	char Text[1024];
	std::memset(Text, 0, sizeof(Text));

	va_list ap;
	va_start(ap, Message);
		std::vsprintf(Text, Message, ap);
	va_end(ap);

	return Text;
}

void csv::log(char const * String, double Convergent, double Min, double Max)
{
	this->Data.push_back(data(String, Convergent, Min, Max));
}

void csv::save(char const * Filename)
{
	FILE* File(fopen(Filename, "a+"));
	assert(File);
	fprintf(File, "%s;%s;%s;%s\n", "Tests", "average", "max", "min");

	for(std::size_t i = 0; i < this->Data.size(); ++i)
	{
		fprintf(File, "%s;%d;%d;%d\n",
			Data[i].String.c_str(),
			static_cast<int>(Data[i].Convergent),
			static_cast<int>(Data[i].Max), static_cast<int>(Data[i].Min));
	}
	fclose(File);
}

void csv::print()
{
	fprintf(stdout, "\n");
	for(std::size_t i = 0; i < this->Data.size(); ++i)
	{
		fprintf(stdout, "%s, %2.5f, %2.5f, %2.5f\n",
			Data[i].String.c_str(),
			Data[i].Convergent / 1000.0,
			Data[i].Min / 1000.0, Data[i].Max / 1000.0);
	}
}

