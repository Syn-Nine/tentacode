//#include "stdafx.h"
#include <stdint.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <conio.h>

#include "Scanner.h"
#include "Enums.h"
#include "Expressions.h"
#include "Parser.h"
#include "Interpreter.h"
#include "ErrorHandler.h"

Interpreter* interpreter;
ErrorHandler* errorHandler;

void RunFile(const char* filename);

bool Run(const char* buf, const char* filename)
{
	if (std::string(buf).compare("quit") == 0) return false;
	if (std::string(buf).compare("q") == 0) return false;

	Scanner scanner(buf, errorHandler, filename);
	TokenList tokens = scanner.ScanTokens();

	if (!tokens.empty() && tokens.at(0).GetType() != TOKEN_END_OF_FILE)
	{
		/*for (auto& token : tokens)
		{
			printf("%s\n", token.ToString().c_str());
		}*/

		Parser parser(tokens, errorHandler);
		StmtList stmts = parser.Parse();

		if (!errorHandler->HasErrors())
		{
			printf("\nResult:\n");
			interpreter->Interpret(stmts);
		}

		if (errorHandler->HasErrors())
		{
			/*printf("AST:\n");
			parser.Print(stmts);*/

			errorHandler->Print();
			errorHandler->Clear();
		}
	}

	return true;
}


void RunPrompt()
{
	//char* cmd = "(1 + 2) * 3.0;";
	//char* cmd = "1 + 2 * 3.5 - 4 < 3;";
	//char* cmd = "i32 x = \"test\";";
	//char* cmd = "if 1 == 2 { println(1); } else if 2 == 3 { println(2); } else { println(3); }";
	//char* cmd = "if 1 == 2 || 2 == 2 { println(1+2); }";
	//char* cmd = "i32 i = 0; while i < 5 { println(i); i = i + 1; }";
	//char* cmd = "println(0..5); println(0..=5);";
	//char* cmd = "f32 t = clock(); println(t); for x in 1..=5 { print(x); print(\" \"); print(clock() - t); print(\" \"); println(rand()); if x == 5 { println(\"weeee!\"); } }";
	//char* cmd = "i32 i = 0; while i < 5 { i = i + 1; }";
	//char* cmd = "println(1.2 as i32); println(1.23 as string + \"test\"); println(2 as f32); println(3 as string + \"b\"); println(\"55\" as i32); println(\"55.6\" as f32);";
	//char* cmd = "println(\"what is your name?\"); string x = input(); println(\"Hello \" + x + \"!\");";
	//char* cmd = "i32 x = 1; loop { loop { println(x); x = x + 1; if x == 3 { break; } println(111); } println(55); break; }";
	//char* cmd = "println(\"f\" as i32);";
	//char* cmd = "i32 x;";
	//char* cmd = "f32 y; i32 i = 100; for x in 0..i { i32 z = rand(0..=5); print(z as string + \", \"); y = y + z; } println(\"avg: \" + (y / i) as string);";
	//char* cmd = "i32 i = 10; i32 j = 2; println(j..=i);";
	//char* cmd = "i32 i = 5; for x in 0..=i { println(x); }";
	//char* cmd = "string i;";
	//char* cmd = "ray_InitWindow(800, 600, \"test\");";
	//char* cmd = "i32 a::b = 5; i32 c::d = a::b; println(a::b as string + c::d as string);";
	//char* cmd = "vec bla::x = [1, 2, 0, 999];\nprintln(bla::x);";
	//char* cmd = "string out; println(out);";
	//char* cmd = "vec x = [1, 2, 3];\nvec y = [4, 5, 6];\nstring out;\nfor i in 0..len(x) {\n  out = out + \" \" + (x[i] + y[i]) as string;\n}\nprintln(\"out = '\" + out + \"'\");\nprintln(\"len(out) = \" + len(out) as string);";
	//char* cmd = "vec x = [1, 2, 3];\nvec y = [4, 5, 6];\nstring out;\nfor i in 0..len(x) {\n  out = out + \" \" + (x[i] + y[i]) as string;\n}\nprintln(\"out = '\" + out + \"'\");\nprintln(\"len(out) = \" + len(out) as string + \"\");";
	//char* cmd = "vec x = [1, 2, 3, 4, 5];\nvec y = x[1..4];\nprintln(y);";
	//char* cmd = "vec x = [1..4, 5, [1, 2, 3], [0..8]];\nprintln(x);";
	//char* cmd = "vec x = [0..5]; x[0] = 99; println(x);";
	//char* cmd = "enum y = :name;\nprintln(:name);\nif y == :name {\n  println(true);\n} else {\n  println(false);\n}";
	//char* cmd = "bool x = true; bool y = false; println(x); println(x == y); println(\"x=\" + x as string);";
	//char* cmd = "vec<i32> x = [1, 2, 3]; x[2] = 4;\nvec<bool> y = [true, false]; y = [y, true];\nvec<f32> z = [1.1, 22222.2, 56.66, 99.99]; z[0] = 0.1;\nvec<string> w = [\"test1\", \"test2\"]; w = [w, \"test append\"];\nprintln(x);\nprintln(y);\nprintln(z);\nprintln(w);";
	//char* cmd = "vec<enum> x = [:a, :b, :cccccc];\nx[1] = :two;\nprintln(x);\nfor i in 0..len(x) {\n  print(\" \" + x[i] as string);\n}\nprintln(\"\");";
	//char* cmd = "/*i32 XRES\n= 800;*/";
	//char* cmd = "vec<i32> i = [0; 2];\nvec<bool> b = [true; 3, false];\nvec<f32> f = [1.1; 4];\nvec<string> s = [\"test\"; 5];\nvec<enum> e = [:meow; 3];\nprintln(i);\nprintln(b);\nprintln(f);\nprintln(s);\nprintln(e);\nprintln([0; 5]);";
	//char* cmd = "def test(x) {\n  println(x);\n  println(\"from test\");\n  return 5;\n}\nprintln(test(true));";
	//char* cmd = "struct mystruct {\n  vec<i32> x;\n}\nmystruct ms;\nms.x = [1, 2];\nms.x[0] = 5;\nprintln(ms.x);";
	//char* cmd = "struct mystruct {\n  i32 x;\n}\nvec<mystruct> v;\nprintln(v);\nmystruct ms;\nv = [ms; 3];\nprintln(v);\nv[0].x = 5;\nprintln(v[0]);";
	//char* cmd = "struct position {\n  f32 x;\n  f32 y;\n}\nstruct cat {\n  position pos;\n  enum name;\n}\ncat floof;\nfloof.name = :APOLLO;\nfloof.pos.x = 2.2;\nfloof.pos.y = 3.3;\nprintln(floof);\nprintln(floof.name as string + \" is at \" + floof.pos.x as string + \", \" + floof.pos.y as string);";
	//char* cmd = "struct position {\n  f32 x;\n  f32 y;\n}\nstruct cat {\n  vec<position> data;\n}\ncat floof;\nfor i in 0..5 {\n  position p;\n  p.x = i;\n  p.y = i * 2;\n  floof.data = [floof.data, p];\n  floof.data[i].x = floof.data[i].x * 3;\n}\n\nfor i in 0..len(floof.data) {\n  println(floof.data[i].x as string + \", \" + floof.data[i].y as string);\n}";
	/*std::fstream fp;
	fp.open("test.tt", std::ios::out);
	fp << "struct position {\n  f32 x;\n  f32 y;\n}\nstruct cat {\n  vec<position> data;\n}\ncat floof;\nfor i in 0..5 {\n  position p;\n  p.x = i;\n  p.y = i * 2;\n  floof.data = [floof.data, p];\n  floof.data[i].x = floof.data[i].x * 3;\n}\n\nfor i in 0..len(floof.data) {\n  println(floof.data[i].x as string + \", \" + floof.data[i].y as string);\n}";
	fp.close();	
	char* cmd = "include \"test.tt\";";*/
	//char* cmd = "vec<i32> x = [1, 2, 3, 4, 5];\nprintln(x);\nvec<i32> y = x[1..3];\nprintln(y); vec<i32> z; z = [z, 3];\nprintln(z);\nz = [z, 4];\nprintln(z);";
	//char* cmd = "struct vec2i { i32 x; }\nstruct entity_s { vec2i ipos; }\nentity_s e0; e0.ipos.x = 5;\nentity_s e1; e1.ipos.x = 9;\nvec<entity_s> ents = [e0, e1];\nprintln(ents);\nprintln(ents[1].ipos.x);\nents[1].ipos.x = 10;\nprintln(ents[1].ipos.x);\nprintln(ents);";
	//char* cmd = "include \"test.tt\";";
	//char* cmd = "println(rand());\nprintln(rand(0..10));\nprintln(rand(3));\nprintln(rand(0..10, 5));";
	//char* cmd = "i32 x, y;\nprintln(x as string + \", \" + y as string);";
	//char* cmd = "i32 x, y = 1, 2;\nprintln(x as string + \", \" + y as string);\nx, y = y, x;\nprintln(x as string + \", \" + y as string);";
	//char* cmd = "FILELINE";
		
	//printf("%s\n", cmd);
	//Run(cmd, "Console");

	// play game override
	for (;;)
	{
		char inbuf[256];
		std::cout << "> ";
		std::cin.getline(inbuf, 256);

		if (!Run(inbuf, "Console")) break;
	}
}

void RunFile(const char* filename)
{
	std::ifstream f;
	f.open(filename, std::ios::in | std::ios::binary | std::ios::ate);

	if (!f.is_open())
	{
		printf("Failed to open file: %s\n", filename);
		RunPrompt();
		return;
	}

	const std::size_t n = f.tellg();
	char* buffer = new char[n + 1];
	buffer[n] = '\0';

	f.seekg(0, std::ios::beg);
	f.read(buffer, n);
	f.close();
	
	Run(buffer, filename);
	
	delete[] buffer;
}


int main(int nargs, char* argsv[])
{
	const char* version = "0.1.1";
	printf("Launching Tentacode Interpreter v%s\n", version);

	errorHandler = new ErrorHandler();
	interpreter = new Interpreter(errorHandler);

	if (1 == nargs)
	{
		std::ifstream f;
		f.open("autoplay.tt", std::ios::in | std::ios::binary | std::ios::ate);
		if (f.is_open())
		{
			RunFile("autoplay.tt");
		}
		else {
			RunPrompt();
		}
	}
	else if (2 == nargs)
	{
		printf("Run File: %s\n", argsv[1]);
		RunFile(argsv[1]);
	}
	else
	{
		printf("Usage: interp [script]\nOmit [script] to run prompt\n");
	}

	//printf("\nPress return to quit...\n");
	//getch();
}