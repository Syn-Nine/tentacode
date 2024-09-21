#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include <random>
#include <chrono>
#include <fstream>
#include <algorithm>

#include "Environment.h"
#include "Literal.h"

#ifndef NO_RAYLIB
#include <raylib.h>
#endif


class Extensions
{
public:

    static void Include_Std(Environment* globals)
    {
		std::string nspace = "global::";
        // clock()
		Literal clockLiteral = Literal();
		clockLiteral.SetCallable(0, [](LiteralList args)->Literal
		{
			const auto p = std::chrono::system_clock::now();
			return double(std::chrono::duration_cast<std::chrono::microseconds>(p.time_since_epoch()).count());
		}, nspace);
		globals->Define("clock", clockLiteral, nspace);

        
        // cprintln()
		Literal cprintlnLiteral = Literal();
		cprintlnLiteral.SetCallable(1, [](LiteralList args)->Literal
		{
            printf("CONSOLE PRINT> %s\n", args[0].ToString().c_str());
            return Literal();
		}, nspace);
		globals->Define("cprintln", cprintlnLiteral, nspace);

        
        // fabs()
		Literal fabsLiteral = Literal();
		fabsLiteral.SetCallable(1, [](LiteralList args)->Literal
		{
			return fabs(args[0].DoubleValue());
		}, nspace);
		globals->Define("fabs", fabsLiteral, nspace);

		
		// floor()
		Literal floorLiteral = Literal();
		floorLiteral.SetCallable(1, [](LiteralList args)->Literal
		{
			return floor(args[0].DoubleValue());
		}, nspace);
		globals->Define("floor", floorLiteral, nspace);


        // input()
		Literal inputLiteral = Literal();
		inputLiteral.SetCallable(0, [](LiteralList args)->Literal
		{
			char inbuf[256];
			std::cin.getline(inbuf, 256);
			return std::string(inbuf);
		}, nspace);
		globals->Define("input", inputLiteral, nspace);


		// len()
		Literal lenLiteral = Literal();
		lenLiteral.SetCallable(1, [](LiteralList args)->Literal
		{
			return args[0].Len();
		}, nspace);
		globals->Define("len", lenLiteral, nspace);

        
        // min()
		Literal minLiteral = Literal();
		minLiteral.SetCallable(2, [](LiteralList args)->Literal
		{
            double m = args[0].DoubleValue();
            if (args[1].DoubleValue() < m) m = args[1].DoubleValue();
            return m;
		}, nspace);
		globals->Define("min", minLiteral, nspace);


        // max()
		Literal maxLiteral = Literal();
		maxLiteral.SetCallable(2, [](LiteralList args)->Literal
		{
            double m = args[0].DoubleValue();
            if (args[1].DoubleValue() > m) m = args[1].DoubleValue();
            return m;
		}, nspace);
		globals->Define("max", maxLiteral, nspace);


		// rand()
		Literal randLiteral = Literal();
		randLiteral.SetCallable(1, [](LiteralList args)->Literal
		{
            static bool first_pass = true;
            static std::mt19937_64 rng;
	        static std::uniform_real_distribution<double> unif(0, 1);

            if (first_pass)
            {
                first_pass = false;
                uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
                std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed>>32)};
                rng.seed(ss);
            }

            double ret = unif(rng);
			if (1 == args.size() && args[0].IsRange())
			{
				int32_t lhs = args[0].LeftValue();
				int32_t rhs = args[0].RightValue();
				int32_t delta = rhs - lhs;
				return int32_t(lhs + ret * delta);
			}
			else if (1 == args.size() && args[0].IsInt())
			{
				int sz = args[0].IntValue();
				if (0 < sz)
				{
					std::vector<double> ret;
					ret.reserve(sz);
					for (size_t i = 0; i < sz; ++i)
					{
						ret.push_back(unif(rng));
					}
					return Literal(ret);
				}
			}
			else if (2 == args.size() && args[0].IsRange() && args[1].IsInt())
			{
				int sz = args[1].IntValue();
				if (0 < sz)
				{
					std::vector<int32_t> ret;
					ret.reserve(sz);
					int32_t lhs = args[0].LeftValue();
					int32_t rhs = args[0].RightValue();
					int32_t delta = rhs - lhs;
					for (size_t i = 0; i < sz; ++i)
					{
						ret.push_back(int32_t(lhs + unif(rng) * delta));
					}
					return Literal(ret);
				}
			}
			return ret;

		}, nspace, false);
		globals->Define("rand", randLiteral, nspace);


		///////////////////////

		// file::readlines()
		Literal fileReadlinesLiteral = Literal();
		fileReadlinesLiteral.SetCallable(1, [](LiteralList args)->Literal
		{
			if (args[0].IsString())
			{
				std::ifstream file(args[0].StringValue().c_str());
				if (file.is_open())
				{
					std::vector<std::string> lines;
					for (std::string line; std::getline(file, line); )
					{
						lines.push_back(line);
					}
					return lines;
				}
			}
			return 0;
		}, nspace);
		globals->Define("readlines", fileReadlinesLiteral, "global::file::");

		// file::writelines()
		Literal fileWritelinesLiteral = Literal();
		fileWritelinesLiteral.SetCallable(2, [](LiteralList args)->Literal
		{
			if (args[0].IsString() && args[1].IsVector() && args[1].IsVecString())
			{
				std::ofstream file(args[0].StringValue().c_str());
				if (file.is_open())
				{
					std::vector<std::string> lines = args[1].VecValue_S();
					for (auto& l : lines)
					{
						file << l << std::endl;
					}
					file.close();
				}
			}
			return 0;
		}, nspace);
		globals->Define("writelines", fileWritelinesLiteral, "global::file::");
		
		///////////////////////

		// str::contains()
		Literal strContainsLiteral = Literal();
		strContainsLiteral.SetCallable(2, [](LiteralList args)->Literal
		{
			if (args[0].IsString() && args[1].IsString())
			{
				return std::string::npos != args[0].StringValue().find(args[1].StringValue());
			}
			return false;
		}, nspace);
		globals->Define("contains", strContainsLiteral, "global::str::");

		// str::replace()
		Literal strReplaceLiteral = Literal();
		strReplaceLiteral.SetCallable(3, [](LiteralList args)->Literal
		{
			if (args[0].IsString() && args[1].IsString() && args[2].IsString())
			{
				return StrReplace(args[0].StringValue(), args[1].StringValue(), args[2].StringValue());
			}
			return 0;
		}, nspace);
		globals->Define("replace", strReplaceLiteral, "global::str::");

		// str::split()
		Literal strSplitLiteral = Literal();
		strSplitLiteral.SetCallable(2, [](LiteralList args)->Literal
		{
			if (args[0].IsString() && args[1].IsString())
			{
				return StrSplit(args[0].StringValue(), args[1].StringValue());
			}
			return 0;
		}, nspace);
		globals->Define("split", strSplitLiteral, "global::str::");

		// str::join()
		Literal strJoinLiteral = Literal();
		strJoinLiteral.SetCallable(2, [](LiteralList args)->Literal
		{
			if (args[0].IsVector() && args[0].IsVecString() && args[1].IsString())
			{
				return StrJoin(args[0].VecValue_S(), args[1].StringValue());
			}
			return 0;
		}, nspace);
		globals->Define("join", strJoinLiteral, "global::str::");

		// str::substr()
		Literal strSubstrLiteral = Literal();
		strSubstrLiteral.SetCallable(3, [](LiteralList args)->Literal
		{
			if (args[0].IsString() && args[1].IsNumeric() && args[2].IsNumeric())
			{
				std::string input = args[0].StringValue();
				int idx = args[1].IntValue();
				int len = args[2].IntValue();
				if (idx >= 0 && idx + len <= input.size()) {
					return input.substr(idx, len);
				}
			}
			return 0;
		}, nspace);
		globals->Define("substr", strSubstrLiteral, "global::str::");

		// str::to_upper()
		Literal strToUpperLiteral = Literal();
		strToUpperLiteral.SetCallable(1, [](LiteralList args)->Literal
		{
			if (args[0].IsString())
			{
				std::string input = args[0].StringValue();
				std::transform(input.begin(), input.end(), input.begin(), ::toupper);
				return input;
			}
			return 0;
		}, nspace);
		globals->Define("to_upper", strToUpperLiteral, "global::str::");

		// str::to_lower()
		Literal strToLowerLiteral = Literal();
		strToLowerLiteral.SetCallable(1, [](LiteralList args)->Literal
		{
			if (args[0].IsString())
			{
				std::string input = args[0].StringValue();
				std::transform(input.begin(), input.end(), input.begin(), ::tolower);
				return input;
			}
			return 0;
		}, nspace);
		globals->Define("to_lower", strToLowerLiteral, "global::str::");

		// str::ltrim()
		Literal strLtrim = Literal();
		strLtrim.SetCallable(1, [](LiteralList args)->Literal
		{
			if (args[0].IsString())
			{
				std::string input = args[0].StringValue();
				input.erase(0, input.find_first_not_of(" \t\n\r\f\v"));
				return input;
			}
			return 0;
		}, nspace);
		globals->Define("ltrim", strLtrim, "global::str::");

		// str::rtrim()
		Literal strRtrim = Literal();
		strRtrim.SetCallable(1, [](LiteralList args)->Literal
		{
			if (args[0].IsString())
			{
				std::string input = args[0].StringValue();
				input.erase(input.find_last_not_of(" \t\n\r\f\v") + 1);
				return input;
			}
			return 0;
		}, nspace);
		globals->Define("rtrim", strRtrim, "global::str::");

		// str::trim()
		Literal strTrim = Literal();
		strTrim.SetCallable(1, [](LiteralList args)->Literal
		{
			if (args[0].IsString())
			{
				std::string input = args[0].StringValue();
				input.erase(0, input.find_first_not_of(" \t\n\r\f\v")); // ltrim
				input.erase(input.find_last_not_of(" \t\n\r\f\v") + 1); // rtrim
				return input;
			}
			return 0;
		}, nspace);
		globals->Define("trim", strTrim, "global::str::");
		
		///////////////////////


		// vec::append()
		Literal vecAppendLiteral = Literal();
		vecAppendLiteral.SetCallable(2, [](LiteralList args)->Literal
		{
			Literal lhs = args[0];
			Literal rhs = args[1];

			if (lhs.IsVecBool())
			{
				auto vals = lhs.VecValue_B();
				if (rhs.IsBool())
				{
					vals.push_back(rhs.BoolValue());
				}
				else if (rhs.IsVecBool())
				{
					auto vals_b = rhs.VecValue_B();
					if (!vals_b.empty())
					{
						vals.insert(vals.end(), vals_b.begin(), vals_b.end());
					}
				}
				return vals;
			}
			else if (lhs.IsVecInteger())
			{
				auto vals = lhs.VecValue_I();
				if (rhs.IsNumeric())
				{
					vals.push_back(rhs.IntValue());
				}
				else if (rhs.IsVecInteger())
				{
					auto vals_b = rhs.VecValue_I();
					if (!vals_b.empty())
					{
						vals.insert(vals.end(), vals_b.begin(), vals_b.end());
					}
				}
				return vals;
			}
			else if (lhs.IsVecDouble())
			{
				auto vals = lhs.VecValue_D();
				if (rhs.IsNumeric())
				{
					vals.push_back(rhs.DoubleValue());
				}
				else if (rhs.IsVecDouble())
				{
					auto vals_b = rhs.VecValue_D();
					if (!vals_b.empty())
					{
						vals.insert(vals.end(), vals_b.begin(), vals_b.end());
					}
				}
				return vals;
			}
			else if (lhs.IsVecString())
			{
				auto vals = lhs.VecValue_S();
				if (rhs.IsString())
				{
					vals.push_back(rhs.StringValue());
				}
				else if (rhs.IsVecString())
				{
					auto vals_b = rhs.VecValue_S();
					if (!vals_b.empty())
					{
						vals.insert(vals.end(), vals_b.begin(), vals_b.end());
					}
				}
				return vals;
			}
			else if (lhs.IsVecEnum())
			{
				auto vals = lhs.VecValue_E();
				if (rhs.IsEnum())
				{
					vals.push_back(rhs.EnumValue());
				}
				else if (rhs.IsVecEnum())
				{
					auto vals_b = rhs.VecValue_E();
					if (!vals_b.empty())
					{
						vals.insert(vals.end(), vals_b.begin(), vals_b.end());
					}
				}
				return vals;
			}
			else if (lhs.IsVecStruct())
			{
				auto vals = lhs.VecValue_U();
				if (rhs.IsInstance())
				{
					vals.push_back(rhs);
				}
				else if (rhs.IsVecStruct())
				{
					auto vals_b = rhs.VecValue_U();
					if (!vals_b.empty())
					{
						vals.insert(vals.end(), vals_b.begin(), vals_b.end());
					}
				}
				return Literal(vals, Literal::LITERAL_TYPE_TT_STRUCT);
			}
			

			printf("Error in vec::append() arguments.\n");
			return 0;
		}, nspace);
		globals->Define("append", vecAppendLiteral, "global::vec::");

        
        //////////////////////////////////////////////
        // Math

		// cos()
		Literal cosLiteral = Literal();
		cosLiteral.SetCallable(1, [](LiteralList args)->Literal
		{
			return cos(args[0].DoubleValue());
		}, nspace);
		globals->Define("cos", cosLiteral, nspace);

        // sin()
		Literal sinLiteral = Literal();
		sinLiteral.SetCallable(1, [](LiteralList args)->Literal
		{
			return sin(args[0].DoubleValue());
		}, nspace);
		globals->Define("sin", sinLiteral, nspace);

        // sgn()
		Literal sgnLiteral = Literal();
		sgnLiteral.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsInt() && args[0].IntValue() < 0) return int32_t(-1);
            if (args[0].IsDouble() && args[0].DoubleValue() < 0) return int32_t(-1);
            return int32_t(1);
		}, nspace);
		globals->Define("sgn", sgnLiteral, nspace);

        // sqrt()
		Literal sqrtLiteral = Literal();
		sqrtLiteral.SetCallable(1, [](LiteralList args)->Literal
		{
			return sqrt(args[0].DoubleValue());
		}, nspace);
		globals->Define("sqrt", sqrtLiteral, nspace);
    }


    ////////////////////////////////////
    ////////////////////////////////////
    ////////////////////////////////////

    
    static void Include_Raylib(Environment* globals)
    {
        std::string nspace = "ray::";
#ifndef NO_RAYLIB
        // ray_InitWindow
		Literal ray_InitWindow = Literal();
		ray_InitWindow.SetCallable(3, [](LiteralList args)->Literal
		{
			int w = args[0].IntValue();
			int h = args[1].IntValue();
			std::string s = args[2].StringValue();
			InitWindow(w, h, s.c_str());
			return 0;
		}, nspace);
		globals->Define("InitWindow", ray_InitWindow, nspace);

		// ray_SetTargetFPS
		Literal ray_SetTargetFPS = Literal();
		ray_SetTargetFPS.SetCallable(1, [](LiteralList args)->Literal
		{
			int fps = args[0].IntValue();
			SetTargetFPS(fps);
			return 0;
		}, nspace);
		globals->Define("SetTargetFPS", ray_SetTargetFPS, nspace);

		// ray_WindowShouldClose
		Literal ray_WindowShouldClose = Literal();
		ray_WindowShouldClose.SetCallable(0, [](LiteralList args)->Literal
		{
			return WindowShouldClose() == true;
		}, nspace);
		globals->Define("WindowShouldClose", ray_WindowShouldClose, nspace);

		// ray_BeginDrawing
		Literal ray_BeginDrawing = Literal();
		ray_BeginDrawing.SetCallable(0, [](LiteralList args)->Literal
		{
			BeginDrawing();
			return 0;
		}, nspace);
		globals->Define("BeginDrawing", ray_BeginDrawing, nspace);

		// ray_ClearBackground
		Literal ray_ClearBackground = Literal();
		ray_ClearBackground.SetCallable(1, [](LiteralList args)->Literal
		{
			std::string s = args[0].EnumValue().enumValue;
			auto d = args[0].VecValue_I();
            if (args[0].IsEnum())
			{
            	ClearBackground(StringToColor(s));
			} else if (args[0].IsVector() && 4 == d.size()) {
				ClearBackground((Color){ d[0], d[1], d[2], d[3] });
			}
			return 0;
		}, nspace);
		globals->Define("ClearBackground", ray_ClearBackground, nspace);

		// ray_EndDrawing
		Literal ray_EndDrawing = Literal();
		ray_EndDrawing.SetCallable(0, [](LiteralList args)->Literal
		{
			EndDrawing();
			return 0;
		}, nspace);
		globals->Define("EndDrawing", ray_EndDrawing, nspace);

		// ray_CloseWindow
		Literal ray_CloseWindow = Literal();
		ray_CloseWindow.SetCallable(0, [](LiteralList args)->Literal
		{
			CloseWindow();
			return 0;
		}, nspace);
		globals->Define("CloseWindow", ray_CloseWindow, nspace);


		// ray_BeginDrawing
		Literal ray_BeginBlendMode = Literal();
		ray_BeginBlendMode.SetCallable(1, [](LiteralList args)->Literal
		{
			if (args[0].IsEnum())
			{
				std::string s = args[0].EnumValue().enumValue;
				if (0 == s.compare(":BLEND_ALPHA")) BeginBlendMode(BLEND_ALPHA);
				else if (0 == s.compare(":BLEND_ALPHA_PREMULTIPLY")) BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
			}
			return 0;
		}, nspace);
		globals->Define("BeginBlendMode", ray_BeginBlendMode, nspace);

		// ray_EndDrawing
		Literal ray_EndBlendMode = Literal();
		ray_EndBlendMode.SetCallable(0, [](LiteralList args)->Literal
		{
			EndBlendMode();
			return 0;
		}, nspace);
		globals->Define("EndBlendMode", ray_EndBlendMode, nspace);

		


        ////////////////////////////////////////////////////////////////////////////////////////
        // Drawing Primitives
        ////////////////////////////////////////////////////////////////////////////////////////

		// ray_DrawCircle
		Literal ray_DrawCircle = Literal();
		ray_DrawCircle.SetCallable(4, [](LiteralList args)->Literal
		{
			int x = args[0].IntValue();
			int y = args[1].IntValue();
			int sz = args[2].IntValue();
			std::string s;
            if (args[3].IsEnum()) s = args[3].EnumValue().enumValue;
            
            DrawCircle(x, y, sz, StringToColor(s));
			return 0;
		}, nspace);
		globals->Define("DrawCircle", ray_DrawCircle, nspace);

        // ray_DrawLine
		Literal ray_DrawLine = Literal();
		ray_DrawLine.SetCallable(5, [](LiteralList args)->Literal
		{
			int x0 = args[0].IntValue();
			int y0 = args[1].IntValue();
            int x1 = args[2].IntValue();
			int y1 = args[3].IntValue();
            std::string s;
            if (args[4].IsEnum()) s = args[4].EnumValue().enumValue;
            
            DrawLine(x0, y0, x1, y1, StringToColor(s));
			return 0;
		}, nspace);
		globals->Define("DrawLine", ray_DrawLine, nspace);

        // ray_DrawRectangle
		Literal ray_DrawRectangle = Literal();
		ray_DrawRectangle.SetCallable(5, [](LiteralList args)->Literal
		{
			int x = args[0].IntValue();
			int y = args[1].IntValue();
            int w = args[2].IntValue();
            int h = args[3].IntValue();
			std::string s;
            if (args[4].IsEnum()) s = args[4].EnumValue().enumValue;
            
            DrawRectangle(x, y, w, h, StringToColor(s));
			return 0;
		}, nspace);
		globals->Define("DrawRectangle", ray_DrawRectangle, nspace);

        // ray_DrawTriangle
		Literal ray_DrawTriangle = Literal();
		ray_DrawTriangle.SetCallable(7, [](LiteralList args)->Literal
		{
            Vector2 v0 = { args[0].DoubleValue(), args[1].DoubleValue() };
            Vector2 v1 = { args[2].DoubleValue(), args[3].DoubleValue() };
            Vector2 v2 = { args[4].DoubleValue(), args[5].DoubleValue() };
            std::string s;
            if (args[6].IsEnum()) s = args[6].EnumValue().enumValue;

			DrawTriangle(v0, v1, v2, StringToColor(s));
			return 0;
		}, nspace);
		globals->Define("DrawTriangle", ray_DrawTriangle, nspace);


		// ray_DrawPixelRGBA
		Literal ray_DrawPixelRGBA = Literal();
		ray_DrawPixelRGBA.SetCallable(6, [](LiteralList args)->Literal
		{
            int32_t x = args[0].IntValue();
			int32_t y = args[1].IntValue();
			int32_t r = args[2].IntValue();
			int32_t g = args[3].IntValue();
			int32_t b = args[4].IntValue();
			int32_t a = args[5].IntValue();
			DrawPixel(x, y, {r, g, b, a});
            return 0;
		}, nspace);
		globals->Define("DrawPixelRGBA", ray_DrawPixelRGBA, nspace);



        ////////////////////////////////////////////////////////////////////////////////////////
        // Input
        ////////////////////////////////////////////////////////////////////////////////////////

        // ray_GetMouseX
		Literal ray_GetMouseX = Literal();
		ray_GetMouseX.SetCallable(0, [](LiteralList args)->Literal
		{
			return int32_t(GetMouseX());
		}, nspace);
		globals->Define("GetMouseX", ray_GetMouseX, nspace);

        // ray_GetMouseY
		Literal ray_GetMouseY = Literal();
		ray_GetMouseY.SetCallable(0, [](LiteralList args)->Literal
		{
			return int32_t(GetMouseY());
		}, nspace);
		globals->Define("GetMouseY", ray_GetMouseY, nspace);

        // ray_IsMouseButtonReleased
		Literal ray_IsMouseButtonReleased = Literal();
		ray_IsMouseButtonReleased.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsEnum())
            {
                return IsMouseButtonReleased(StringToKey(args[0].EnumValue().enumValue));
            }
			return false;

		}, nspace);
		globals->Define("IsMouseButtonReleased", ray_IsMouseButtonReleased, nspace);
		

        // ray_IsKeyPressed
		Literal ray_IsKeyPressed = Literal();
		ray_IsKeyPressed.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsEnum())
            {
                return IsKeyPressed(StringToKey(args[0].EnumValue().enumValue));
            }
			return false;

		}, nspace);
		globals->Define("IsKeyPressed", ray_IsKeyPressed, nspace);

        // ray_IsKeyDown
		Literal ray_IsKeyDown = Literal();
		ray_IsKeyDown.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsEnum())
            {
                return IsKeyDown(StringToKey(args[0].EnumValue().enumValue));
            }
			return false;

		}, nspace);
		globals->Define("IsKeyDown", ray_IsKeyDown, nspace);
        



        ////////////////////////////////////////////////////////////////////////////////////////
        // Images
        ////////////////////////////////////////////////////////////////////////////////////////

        // ray_LoadImage
		Literal ray_LoadImage = Literal();
		ray_LoadImage.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsString())
            {
                return LoadImage(args[0].StringValue().c_str());
            }
            return 0;
		}, nspace);
		globals->Define("LoadImage", ray_LoadImage, nspace);

        // ray_LoadImageFromScreen
		Literal ray_LoadImageFromScreen = Literal();
		ray_LoadImageFromScreen.SetCallable(0, [](LiteralList args)->Literal
		{
            return LoadImageFromScreen();
		}, nspace);
		globals->Define("LoadImageFromScreen", ray_LoadImageFromScreen, nspace);

		// ray_ExportImage
		Literal ray_ExportImage = Literal();
		ray_ExportImage.SetCallable(2, [](LiteralList args)->Literal
		{
            if (args[0].IsImage() && args[1].IsString())
            {
                return ExportImage(args[0].ImageValue(), args[1].StringValue().c_str());
            }
            return 0;
		}, nspace);
		globals->Define("ExportImage", ray_ExportImage, nspace);

        // ray_ImagePeek
		Literal ray_ImagePeek = Literal();
		ray_ImagePeek.SetCallable(4, [](LiteralList args)->Literal
		{
            if (args[0].IsImage() && args[1].IsInt() && args[2].IsInt() && args[3].IsInt())
            {
				// assumes 32 bpp
				Image img = args[0].ImageValue();
				uint8_t* raw = (uint8_t*)img.data;
				size_t x = args[1].IntValue();
				size_t y = args[2].IntValue();
				size_t ofs = args[3].IntValue();
				if (x >= 0 && y >= 0 && x < img.width && y < img.height && ofs < 4)
				{
					return raw[(y * img.width + x) * 4 + ofs];
				}
            }
            return 0;
		}, nspace);
		globals->Define("ImagePeek", ray_ImagePeek, nspace);

		// ray_ImageResize
		Literal ray_ImageResize = Literal();
		ray_ImageResize.SetCallable(3, [](LiteralList args)->Literal
		{
            if (args[0].IsImage())
            {
                int32_t x = args[1].IntValue();
                int32_t y = args[2].IntValue();
                ImageResize(&(args[0].ImageValue()), x, y);
            }
            return 0;
		}, nspace);
		globals->Define("ImageResize", ray_ImageResize, nspace);

        // ray_ImageWidth
		Literal ray_ImageWidth = Literal();
		ray_ImageWidth.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsImage())
            {
                return args[0].ImageValue().width;
            }
            return 0;
		}, nspace);
		globals->Define("ImageWidth", ray_ImageWidth, nspace);

        // ray_ImageHeight
		Literal ray_ImageHeight = Literal();
		ray_ImageHeight.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsImage())
            {
                return args[0].ImageValue().height;
            }
            return 0;
		}, nspace);
		globals->Define("ImageHeight", ray_ImageHeight, nspace);


		////////////////////////////////////////////////////////////////////////////////////////
        // Shaders
        ////////////////////////////////////////////////////////////////////////////////////////

		// ray_LoadShader
		Literal ray_LoadShader = Literal();
		ray_LoadShader.SetCallable(2, [](LiteralList args)->Literal
		{
			const char* vsFilename = args[0].IsString() ? args[0].StringValue().c_str() : nullptr;
			const char* fsFilename = args[1].IsString() ? args[1].StringValue().c_str() : nullptr;
			return LoadShader(vsFilename, fsFilename);
		}, nspace);
		globals->Define("LoadShader", ray_LoadShader, nspace);

		// ray_LoadShader
		Literal ray_UnloadShader = Literal();
		ray_UnloadShader.SetCallable(1, [](LiteralList args)->Literal
		{
			if (args[0].IsShader()) UnloadShader(args[0].ShaderValue());
			return 0;
		}, nspace);
		globals->Define("UnloadShader", ray_UnloadShader, nspace);

		// ray_BeginShaderMode
		Literal ray_BeginShaderMode = Literal();
		ray_BeginShaderMode.SetCallable(1, [](LiteralList args)->Literal
		{
			if (args[0].IsShader()) BeginShaderMode(args[0].ShaderValue());
			return 0;
		}, nspace);
		globals->Define("BeginShaderMode", ray_BeginShaderMode, nspace);

		// ray_EndShaderMode
		Literal ray_EndShaderMode = Literal();
		ray_EndShaderMode.SetCallable(0, [](LiteralList args)->Literal
		{
			EndShaderMode();
			return 0;
		}, nspace);
		globals->Define("EndShaderMode", ray_EndShaderMode, nspace);

        
        ////////////////////////////////////////////////////////////////////////////////////////
        // Sounds
        ////////////////////////////////////////////////////////////////////////////////////////

        // ray_InitAudioDevice
		Literal ray_InitAudioDevice = Literal();
		ray_InitAudioDevice.SetCallable(0, [](LiteralList args)->Literal
		{
            InitAudioDevice();
            return 0;
		}, nspace);
		globals->Define("InitAudioDevice", ray_InitAudioDevice, nspace);


        // ray_CloseAudioDevice
		Literal ray_CloseAudioDevice = Literal();
		ray_CloseAudioDevice.SetCallable(0, [](LiteralList args)->Literal
		{
            CloseAudioDevice();
            return 0;
		}, nspace);
		globals->Define("CloseAudioDevice", ray_CloseAudioDevice, nspace);


        // ray_LoadSound
		Literal ray_LoadSound = Literal();
		ray_LoadSound.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsString())
            {
                return LoadSound(args[0].StringValue().c_str());
            }
            return 0;
		}, nspace);
		globals->Define("LoadSound", ray_LoadSound, nspace);


        // ray_PlaySound
		Literal ray_PlaySound = Literal();
		ray_PlaySound.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsSound())
            {
                PlaySound(args[0].SoundValue());
            }
            return 0;
		}, nspace);
		globals->Define("PlaySound", ray_PlaySound, nspace);


        // ray_UnloadSound
		Literal ray_UnloadSound = Literal();
		ray_UnloadSound.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsSound())
            {
                UnloadSound(args[0].SoundValue());
            }
            return 0;
		}, nspace);
		globals->Define("UnloadSound", ray_UnloadSound, nspace);
        
        
        
        ////////////////////////////////////////////////////////////////////////////////////////
        // Textures
        ////////////////////////////////////////////////////////////////////////////////////////

        // ray_LoadTexture
		Literal ray_LoadTexture = Literal();
		ray_LoadTexture.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsString())
            {
                return LoadTexture(args[0].StringValue().c_str());
            }
            return 0;
		}, nspace);
		globals->Define("LoadTexture", ray_LoadTexture, nspace);

		// ray_LoadRenderTexture
		Literal ray_LoadRenderTexture = Literal();
		ray_LoadRenderTexture.SetCallable(2, [](LiteralList args)->Literal
		{
			return LoadRenderTexture(args[0].IntValue(), args[1].IntValue());
		}, nspace);
		globals->Define("LoadRenderTexture", ray_LoadRenderTexture, nspace);

        // ray_LoadTextureFromImage
		Literal ray_LoadTextureFromImage = Literal();
		ray_LoadTextureFromImage.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsImage())
            {
                return LoadTextureFromImage(args[0].ImageValue());
            }
            return 0;
		}, nspace);
		globals->Define("LoadTextureFromImage", ray_LoadTextureFromImage, nspace);

        // ray_TextureWidth
		Literal ray_TextureWidth = Literal();
		ray_TextureWidth.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsTexture())
            {
                return args[0].TextureValue().width;
            }
            return 0;
		}, nspace);
		globals->Define("TextureWidth", ray_TextureWidth, nspace);

        // ray_TextureHeight
		Literal ray_TextureHeight = Literal();
		ray_TextureHeight.SetCallable(1, [](LiteralList args)->Literal
		{
            if (args[0].IsTexture())
            {
                return args[0].TextureValue().height;
            }
            return 0;
		}, nspace);
		globals->Define("TextureHeight", ray_TextureHeight, nspace);

        // ray_DrawTexture
		Literal ray_DrawTexture = Literal();
		ray_DrawTexture.SetCallable(4, [](LiteralList args)->Literal
		{
            if ((args[0].IsTexture() || args[0].IsRenderTexture2D()) && args[3].IsEnum())
            {
				Texture tex = args[0].IsTexture() ? args[0].TextureValue() : args[0].RenderTexture2dValue().texture;
                int32_t x = args[1].IntValue();
                int32_t y = args[2].IntValue();
                std::string s = args[3].EnumValue().enumValue;

                DrawTexture(tex, x, y, StringToColor(s));
            }
            return 0;
		}, nspace);
		globals->Define("DrawTexture", ray_DrawTexture, nspace);

		// ray_DrawTextureRec
		Literal ray_DrawTextureRec = Literal();
		ray_DrawTextureRec.SetCallable(5, [](LiteralList args)->Literal
		{
            if ((args[0].IsTexture() || args[0].IsRenderTexture2D()) && args[1].IsVecInteger() && args[4].IsEnum())
            {
				Texture tex = args[0].IsTexture() ? args[0].TextureValue() : args[0].RenderTexture2dValue().texture;
				auto v = args[1].VecValue_I();
				if (4 == v.size()) {
					std::string s = args[4].EnumValue().enumValue;
					DrawTextureRec(tex, { v[0], v[1], v[2], v[3] }, { args[2].IntValue(), args[3].IntValue() }, StringToColor(s));
				}
            }
            return 0;
		}, nspace);
		globals->Define("DrawTextureRec", ray_DrawTextureRec, nspace);

        // ray_DrawTextureEx
		Literal ray_DrawTextureEx = Literal();
		ray_DrawTextureEx.SetCallable(6, [](LiteralList args)->Literal
		{
            if (args[0].IsTexture() && args[5].IsEnum())
            {
                int32_t x = args[1].IntValue();
                int32_t y = args[2].IntValue();
                double rot = args[3].DoubleValue();
                double scale = args[4].DoubleValue();
                std::string s = args[5].EnumValue().enumValue;

                DrawTextureEx(args[0].TextureValue(), (Vector2){x, y}, rot, scale, StringToColor(s));
            }
            return 0;
		}, nspace);
		globals->Define("DrawTextureEx", ray_DrawTextureEx, nspace);

		// ray_DrawTexturePro
		Literal ray_DrawTexturePro = Literal();
		ray_DrawTexturePro.SetCallable(7, [](LiteralList args)->Literal
		{
            if ((args[0].IsTexture() || args[0].IsRenderTexture2D()) &&
				args[1].IsVecInteger() &&
				args[2].IsVecInteger() &&
				args[6].IsEnum())
            {
				std::vector<int32_t> isrc = args[1].VecValue_I();
				std::vector<int32_t> idst = args[2].VecValue_I();
				if (4 == isrc.size() && 4 == idst.size())
				{
					Texture tex = args[0].IsTexture() ? args[0].TextureValue() : args[0].RenderTexture2dValue().texture;
					float x = args[3].DoubleValue();
					float y = args[4].DoubleValue();
					double rot = args[5].DoubleValue();
					std::string s = args[6].EnumValue().enumValue;

					Rectangle src = { isrc[0], isrc[1], isrc[2], isrc[3] };
					Rectangle dst = { idst[0], idst[1], idst[2], idst[3] };
					Vector2 org = { x, y };

					DrawTexturePro(tex, src, dst, org, rot, StringToColor(s));
				}
            }
            return 0;
		}, nspace);
		globals->Define("DrawTexturePro", ray_DrawTexturePro, nspace);

		
        // ray_DrawTextureTile
		Literal ray_DrawTextureExTile = Literal();
		ray_DrawTextureExTile.SetCallable(9, [](LiteralList args)->Literal
		{
            if (args[0].IsTexture() && args[8].IsEnum())
            {
                double x = args[1].DoubleValue();
                double y = args[2].DoubleValue();
                int32_t idx = args[3].IntValue();
                int32_t w = args[4].IntValue();
                int32_t h = args[5].IntValue();
                double rot = args[6].DoubleValue();
                double scale = args[7].DoubleValue();
                std::string s = args[8].EnumValue().enumValue;

                Texture2D tex = args[0].TextureValue();

                int32_t tw = (tex.width / w);
                int32_t u = idx % tw;
                int32_t v = (idx - u) / tw;

                int32_t x0 = u * w;
                int32_t y0 = v * w;
                
                Rectangle src = { x0, y0, w, h };
                Rectangle dst = { int(x * w * scale), int(y * h * scale), w * scale, h * scale };
                Vector2 org = { 0, 0 };

                DrawTexturePro(tex, src, dst, org, rot, StringToColor(s));
            }
            return 0;
		}, nspace);
		globals->Define("DrawTextureExTile", ray_DrawTextureExTile, nspace);


        // ray_DrawTextureTileMap
		Literal ray_DrawTextureTileMap = Literal();
		ray_DrawTextureTileMap.SetCallable(9, [](LiteralList args)->Literal
		{
            if (args[0].IsTexture() && args[7].IsVecInteger() && (args[8].IsVecEnum() || args[8].IsEnum()))
            {
                Texture2D tex = args[0].TextureValue();
                int32_t xx = args[1].IntValue();
                int32_t yy = args[2].IntValue();
                int32_t width = args[3].IntValue();
                int32_t w = args[4].IntValue();
                int32_t h = args[5].IntValue();
                double scale = args[6].DoubleValue();
                std::vector<int32_t> tiles = args[7].VecValue_I();
                std::vector<EnumLiteral> colors;
				std::string s;
				bool colorVec = args[8].IsVecEnum();
				if (colorVec)
				{
					colors = args[8].VecValue_E();
				}
				else
				{
					s = args[8].EnumValue().enumValue;
				}

                double rot = 0;
                int32_t height = tiles.size() / width;
                int32_t tw = (tex.width / w);
                        
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        size_t idx = y * width + x;
                        int32_t sprite = tiles[idx];
                        if (0 > sprite) continue;
                        
                        if (colorVec) s = colors[idx].enumValue;

                        int32_t u = sprite % tw;
                        int32_t v = (sprite - u) / tw;
                        int32_t x0 = u * w;
                        int32_t y0 = v * h;
                        
                        Rectangle src = { x0, y0, w, h };
                        Rectangle dst = { xx + int(x * w * scale), yy + int(y * h * scale), w * scale, h * scale };
                        Vector2 org = { 0, 0 };

                        DrawTexturePro(tex, src, dst, org, rot, StringToColor(s));
                    }
                }

            }
            return 0;
		}, nspace);
		globals->Define("DrawTextureTileMap", ray_DrawTextureTileMap, nspace);

		// ray_BeginTextureMode
		Literal ray_BeginTextureMode = Literal();
		ray_BeginTextureMode.SetCallable(1, [](LiteralList args)->Literal
		{
			if (args[0].IsRenderTexture2D()) BeginTextureMode(args[0].RenderTexture2dValue());
			return 0;
		}, nspace);
		globals->Define("BeginTextureMode", ray_BeginTextureMode, nspace);

		// ray_EndTextureMode
		Literal ray_EndTextureMode = Literal();
		ray_EndTextureMode.SetCallable(0, [](LiteralList args)->Literal
		{
			EndTextureMode();
			return 0;
		}, nspace);
		globals->Define("EndTextureMode", ray_EndTextureMode, nspace);

        
        ////////////////////////////////////////////////////////////////////////////////////////
        // Fonts
        ////////////////////////////////////////////////////////////////////////////////////////

        // ray_LoadFont
		Literal ray_LoadFont = Literal();
		ray_LoadFont.SetCallable(1, [](LiteralList args)->Literal
		{
			if (args[0].IsString())
            {
                return LoadFont(args[0].StringValue().c_str());
            }
            return 0;
		}, nspace);
		globals->Define("LoadFont", ray_LoadFont, nspace);


        // ray_DrawText
		Literal ray_DrawText = Literal();
		ray_DrawText.SetCallable(5, [](LiteralList args)->Literal
		{
            if (args[0].IsString() && args[4].IsEnum())
            {
                std::string txt = args[0].StringValue();
                int x = args[1].IntValue();
                int y = args[2].IntValue();
                int sz = args[3].IntValue();
                std::string s = args[4].EnumValue().enumValue;
                
                DrawText(txt.c_str(), x, y, sz, StringToColor(s));
            }
			return 0;
		}, nspace);
		globals->Define("DrawText", ray_DrawText, nspace);
        
        // ray_DrawText
		Literal ray_DrawTextEx = Literal();
		ray_DrawTextEx.SetCallable(7, [](LiteralList args)->Literal
		{
			if (args[0].IsFont() && args[1].IsString() && args[6].IsEnum())
            {
                std::string txt = args[1].StringValue();
                int x = args[2].IntValue();
                int y = args[3].IntValue();
                double sz = args[4].DoubleValue();
                double spc = args[5].DoubleValue();
                std::string s = args[6].EnumValue().enumValue;
                
                DrawTextEx(args[0].FontValue(), txt.c_str(), (Vector2){ x, y }, sz, spc, StringToColor(s));
            }
			return 0;
		}, nspace);
		globals->Define("DrawTextEx", ray_DrawTextEx, nspace);

        // ray_MeasureText
		Literal ray_MeasureText = Literal();
		ray_MeasureText.SetCallable(2, [](LiteralList args)->Literal
		{
			if (args[0].IsString())
            {
                std::string txt = args[0].StringValue();
                double sz = args[1].DoubleValue();
                return MeasureText(txt.c_str(), sz);
            }
			return 0;
		}, nspace);
		globals->Define("MeasureText", ray_MeasureText, nspace);

		// ray_MeasureTextEx
		Literal ray_MeasureTextEx = Literal();
		ray_MeasureTextEx.SetCallable(4, [](LiteralList args)->Literal
		{
			if (args[0].IsFont() && args[1].IsString())
            {
                std::string txt = args[1].StringValue();
                double sz = args[2].DoubleValue();
				double spc = args[3].DoubleValue();
                Vector2 ret = MeasureTextEx(args[0].FontValue(), txt.c_str(), sz, spc);
				return ret.x;
            }
			return 0;
		}, nspace);
		globals->Define("MeasureTextEx", ray_MeasureTextEx, nspace);


        ////////////////////////////////////////////////////////////////////////////////////////
        // Gamepad
        ////////////////////////////////////////////////////////////////////////////////////////

        // ray_IsGamepadAvailable
		Literal ray_IsGamepadAvailable = Literal();
		ray_IsGamepadAvailable.SetCallable(1, [](LiteralList args)->Literal
		{
            return IsGamepadAvailable(args[0].IntValue());
		}, nspace);
		globals->Define("IsGamepadAvailable", ray_IsGamepadAvailable, nspace);


        // ray_IsGamepadAvailable
		Literal ray_GetGamepadAxisMovement = Literal();
		ray_IsGamepadAvailable.SetCallable(2, [](LiteralList args)->Literal
		{
            if (args[1].IsEnum())
            {
                std::string s = args[1].EnumValue().enumValue;
                int axis = GAMEPAD_AXIS_LEFT_X;
                if (0 == s.compare(":GAMEPAD_AXIS_LEFT_Y")) axis = GAMEPAD_AXIS_LEFT_Y;
                
                return GetGamepadAxisMovement(args[0].IntValue(), axis);
            }
            return 0;
		}, nspace);
		globals->Define("GetGamepadAxisMovement", ray_IsGamepadAvailable, nspace);

        // ray_IsGamepadButtonDown
		Literal ray_IsGamepadButtonDown = Literal();
		ray_IsGamepadAvailable.SetCallable(2, [](LiteralList args)->Literal
		{
            if (args[1].IsEnum())
            {
                std::string s = args[1].EnumValue().enumValue;
                return IsGamepadButtonDown(args[0].IntValue(), StringToGamepadButton(s));
            }
            return 0;
		}, nspace);
		globals->Define("IsGamepadButtonDown", ray_IsGamepadAvailable, nspace);

#endif

    }

private:

#ifndef NO_RAYLIB
    static Color StringToColor(const std::string& s)
    {
		static bool first = true;
		static std::map<std::string, Color> enumMap;
		if (first)
		{
			first = false;
			enumMap.insert(std::make_pair(":LIGHTGRAY", LIGHTGRAY));
			enumMap.insert(std::make_pair(":GRAY", GRAY));
			enumMap.insert(std::make_pair(":DARKGRAY", Color { 60, 60, 60, 255 }));
			enumMap.insert(std::make_pair(":DARKDARKGRAY", Color { 20, 20, 20, 255 }));
			enumMap.insert(std::make_pair(":YELLOW", YELLOW));
			enumMap.insert(std::make_pair(":GOLD", GOLD));
			enumMap.insert(std::make_pair(":ORANGE", ORANGE));
			enumMap.insert(std::make_pair(":PINK", PINK));
			enumMap.insert(std::make_pair(":RED", RED));
			enumMap.insert(std::make_pair(":DARKRED", Color { 128, 20, 25, 255 }));
			enumMap.insert(std::make_pair(":MAROON", MAROON));
			enumMap.insert(std::make_pair(":GREEN", GREEN));
			enumMap.insert(std::make_pair(":LIME", LIME));
			enumMap.insert(std::make_pair(":DARKGREEN", DARKGREEN));
			enumMap.insert(std::make_pair(":SKYBLUE", SKYBLUE));
			enumMap.insert(std::make_pair(":BLUE", BLUE));
			enumMap.insert(std::make_pair(":DARKBLUE", DARKBLUE));
			enumMap.insert(std::make_pair(":PURPLE", PURPLE));
			enumMap.insert(std::make_pair(":VIOLET", VIOLET));
			enumMap.insert(std::make_pair(":DARKPURPLE", DARKPURPLE));
			enumMap.insert(std::make_pair(":BEIGE", BEIGE));
			enumMap.insert(std::make_pair(":BROWN", BROWN));
			enumMap.insert(std::make_pair(":DARKBROWN", DARKBROWN));
			enumMap.insert(std::make_pair(":WHITE", WHITE));
			enumMap.insert(std::make_pair(":BLACK", BLACK));
			enumMap.insert(std::make_pair(":BLANK", BLANK));
			enumMap.insert(std::make_pair(":MAGENTA", MAGENTA));
			enumMap.insert(std::make_pair(":RAYWHITE", RAYWHITE));
			enumMap.insert(std::make_pair(":SHARKGRAY", Color { 34, 32, 39, 255 }));
			enumMap.insert(std::make_pair(":SLATEGRAY", Color { 140, 173, 181, 255 }));
			enumMap.insert(std::make_pair(":DARKSLATEGRAY", Color { 67, 99, 107, 255 }));
			
		}
		if (0 != enumMap.count(s)) return enumMap.at(s);
        return MAROON;
    }

    static int StringToKey(const std::string& s)
    {
		static bool first = true;
		static std::map<std::string, int> enumMap;
		if (first)
		{
			first = false;
			enumMap.insert(std::make_pair(":KEY_UP", KEY_UP));
			enumMap.insert(std::make_pair(":KEY_DOWN", KEY_DOWN));
			enumMap.insert(std::make_pair(":KEY_LEFT", KEY_LEFT));
			enumMap.insert(std::make_pair(":KEY_RIGHT", KEY_RIGHT));
			enumMap.insert(std::make_pair(":KEY_SPACE", KEY_SPACE));
			enumMap.insert(std::make_pair(":KEY_ENTER", KEY_ENTER));
			enumMap.insert(std::make_pair(":KEY_BACKSPACE", KEY_BACKSPACE));
			//
			enumMap.insert(std::make_pair(":KEY_A", KEY_A));
			enumMap.insert(std::make_pair(":KEY_B", KEY_B));
			enumMap.insert(std::make_pair(":KEY_C", KEY_C));
			enumMap.insert(std::make_pair(":KEY_D", KEY_D));
			enumMap.insert(std::make_pair(":KEY_E", KEY_E));
			enumMap.insert(std::make_pair(":KEY_F", KEY_F));
			enumMap.insert(std::make_pair(":KEY_G", KEY_G));
			enumMap.insert(std::make_pair(":KEY_H", KEY_H));
			enumMap.insert(std::make_pair(":KEY_I", KEY_I));
			enumMap.insert(std::make_pair(":KEY_J", KEY_J));
			enumMap.insert(std::make_pair(":KEY_K", KEY_K));
			enumMap.insert(std::make_pair(":KEY_L", KEY_L));
			enumMap.insert(std::make_pair(":KEY_M", KEY_M));
			enumMap.insert(std::make_pair(":KEY_N", KEY_N));
			enumMap.insert(std::make_pair(":KEY_O", KEY_O));
			enumMap.insert(std::make_pair(":KEY_P", KEY_P));
			enumMap.insert(std::make_pair(":KEY_Q", KEY_Q));
			enumMap.insert(std::make_pair(":KEY_R", KEY_R));
			enumMap.insert(std::make_pair(":KEY_S", KEY_S));
			enumMap.insert(std::make_pair(":KEY_T", KEY_T));
			enumMap.insert(std::make_pair(":KEY_U", KEY_U));
			enumMap.insert(std::make_pair(":KEY_V", KEY_V));
			enumMap.insert(std::make_pair(":KEY_W", KEY_W));
			enumMap.insert(std::make_pair(":KEY_X", KEY_X));
			enumMap.insert(std::make_pair(":KEY_Y", KEY_Y));
			enumMap.insert(std::make_pair(":KEY_Z", KEY_Z));
			//
			enumMap.insert(std::make_pair(":MOUSE_BUTTON_LEFT", MOUSE_BUTTON_LEFT));
			enumMap.insert(std::make_pair(":MOUSE_BUTTON_RIGHT", MOUSE_BUTTON_RIGHT));
			enumMap.insert(std::make_pair(":MOUSE_BUTTON_MIDDLE", MOUSE_BUTTON_MIDDLE));
			
		}
		if (0 != enumMap.count(s)) return enumMap.at(s);
        return KEY_NULL;
    }

    static int StringToGamepadButton(const std::string& s)
    {
		static bool first = true;
		static std::map<std::string, int> enumMap;
		if (first)
		{
			first = false;
			enumMap.insert(std::make_pair(":GAMEPAD_BUTTON_LEFT_FACE_UP", GAMEPAD_BUTTON_LEFT_FACE_UP));
			enumMap.insert(std::make_pair(":GAMEPAD_BUTTON_LEFT_FACE_DOWN", GAMEPAD_BUTTON_LEFT_FACE_DOWN));
			enumMap.insert(std::make_pair(":GAMEPAD_BUTTON_LEFT_FACE_LEFT", GAMEPAD_BUTTON_LEFT_FACE_LEFT));
			enumMap.insert(std::make_pair(":GAMEPAD_BUTTON_LEFT_FACE_RIGHT", GAMEPAD_BUTTON_LEFT_FACE_RIGHT));
		}
		if (0 != enumMap.count(s)) return enumMap.at(s);
		return KEY_NULL;
    }
#endif

};


#endif //EXTENSIONS_H