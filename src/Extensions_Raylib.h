#ifndef EXTENSIONS_RAYLIB_H
#define EXTENSIONS_RAYLIB_H
#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

#include "Environment.h"
#include <raylib.h>

#include <map>
#include <string>

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif


//-----------------------------------------------------------------------------
// AUTO-CODED RAYLIB BINDINGS
//-----------------------------------------------------------------------------


static Environment* rayenv = nullptr;

//

static Color StringToColor(const std::string& s);
static int StringToKey(const std::string& s);
static int StringToGamepadAxis(const std::string& s);
static int StringToGamepadButton(const std::string& s);

//-----------------------------------------------------------------------------
// manual functions

extern "C" DLLEXPORT int32_t ray_ImagePeek(Image* in0, int32_t in1, int32_t in2, int32_t in3)
{
    Image img = *in0;
    uint8_t* raw = (uint8_t*)img.data;
    size_t x = in1;
    size_t y = in2;
    size_t ofs = in3;
    if (x >= 0 && y >= 0 && x < img.width && y < img.height && ofs < 4)
    {
        return raw[(y * img.width + x) * 4 + ofs];
    }
    else
    {
        printf("ray_ImagePeek out of bounds\n");
    }
    return 0;
}

extern "C" DLLEXPORT void ray_DrawTextureTileMap(Texture2D* in0, int32_t xx, int32_t yy, int32_t width, int32_t w, int32_t h, double scale, void* srcPtr, int32_t color)
{
    Texture2D tex = *in0;
    llvm::SmallVector<int32_t>* src = static_cast<llvm::SmallVector<int32_t>*>(srcPtr);
    llvm::SmallVector<int32_t>& tiles = *src;
    double rot = 0;
    int32_t height = tiles.size() / width;
    int32_t tw = (tex.width / w);

    Color c = StringToColor(rayenv->GetEnumAsString(color));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = y * width + x;
            int32_t sprite = tiles[idx];
            if (0 > sprite) continue;

            int32_t u = sprite % tw;
            int32_t v = (sprite - u) / tw;
            int32_t x0 = u * w;
            int32_t y0 = v * h;

            Rectangle src = { float(x0), float(y0), float(w), float(h) };
            Rectangle dst = { float(xx + int(x * w * scale)), float(yy + int(y * h * scale)), float(w * scale), float(h * scale) };
            Vector2 org = { 0, 0 };

            DrawTexturePro(tex, src, dst, org, rot, c);
        }
    }
}


//-----------------------------------------------------------------------------
//

extern "C" DLLEXPORT void ray_BeginDrawing() { BeginDrawing(); }
extern "C" DLLEXPORT void ray_ClearBackground(int32_t in0) { ClearBackground(StringToColor(rayenv->GetEnumAsString(in0))); }
extern "C" DLLEXPORT void ray_CloseWindow() { CloseWindow(); }
extern "C" DLLEXPORT void ray_DrawCircle(int32_t in0, int32_t in1, double in2, int32_t in3) { DrawCircle(in0, in1, in2, StringToColor(rayenv->GetEnumAsString(in3))); }
extern "C" DLLEXPORT void ray_DrawLine(int32_t in0, int32_t in1, int32_t in2, int32_t in3, int32_t in4) { DrawLine(in0, in1, in2, in3, StringToColor(rayenv->GetEnumAsString(in4))); }
extern "C" DLLEXPORT void ray_DrawRectangle(int32_t in0, int32_t in1, int32_t in2, int32_t in3, int32_t in4) { DrawRectangle(in0, in1, in2, in3, StringToColor(rayenv->GetEnumAsString(in4))); }
extern "C" DLLEXPORT void ray_DrawText(std::string* in0, int32_t in1, int32_t in2, int32_t in3, int32_t in4) { DrawText(in0->c_str(), in1, in2, in3, StringToColor(rayenv->GetEnumAsString(in4))); }
extern "C" DLLEXPORT void ray_EndDrawing() { EndDrawing(); }
extern "C" DLLEXPORT void ray_InitWindow(int32_t in0, int32_t in1, std::string* in2) { InitWindow(in0, in1, in2->c_str()); }
extern "C" DLLEXPORT bool ray_IsKeyDown(int32_t in0) { return IsKeyDown(StringToKey(rayenv->GetEnumAsString(in0))); }
extern "C" DLLEXPORT int32_t ray_GetMouseX() { return GetMouseX(); }
extern "C" DLLEXPORT int32_t ray_GetMouseY() { return GetMouseY(); }
extern "C" DLLEXPORT void ray_SetTargetFPS(int32_t in0) { SetTargetFPS(in0); }
extern "C" DLLEXPORT bool ray_WindowShouldClose() { return WindowShouldClose(); }
extern "C" DLLEXPORT Texture2D* ray_LoadTexture(std::string* in0) { return new Texture2D(LoadTexture(in0->c_str())); }
extern "C" DLLEXPORT void ray_DrawTexture(Texture2D* in0, int32_t in1, int32_t in2, int32_t in3) { DrawTexture(*in0, in1, in2, StringToColor(rayenv->GetEnumAsString(in3))); }
extern "C" DLLEXPORT void ray_DrawTextureV(Texture2D* in0, double in1a, double in1b, int32_t in2) { DrawTextureV(*in0, { float(in1a), float(in1b) }, StringToColor(rayenv->GetEnumAsString(in2))); }
extern "C" DLLEXPORT void ray_DrawTextureEx(Texture2D* in0, double in1a, double in1b, double in2, double in3, int32_t in4) { DrawTextureEx(*in0, { float(in1a), float(in1b) }, in2, in3, StringToColor(rayenv->GetEnumAsString(in4))); }
extern "C" DLLEXPORT void ray_DrawTexturePro(Texture2D* in0, double in1a, double in1b, double in1c, double in1d, double in2a, double in2b, double in2c, double in2d, double in3a, double in3b, double in4, int32_t in5) { DrawTexturePro(*in0, { float(in1a), float(in1b), float(in1c), float(in1d) }, { float(in2a), float(in2b), float(in2c), float(in2d) }, { float(in3a), float(in3b) }, in4, StringToColor(rayenv->GetEnumAsString(in5))); }
extern "C" DLLEXPORT bool ray_IsGamepadAvailable(int32_t in0) { return IsGamepadAvailable(in0); }
extern "C" DLLEXPORT double ray_GetGamepadAxisMovement(int32_t in0, int32_t in1) { return GetGamepadAxisMovement(in0, StringToGamepadAxis(rayenv->GetEnumAsString(in1))); }
extern "C" DLLEXPORT bool ray_IsGamepadButtonDown(int32_t in0, int32_t in1) { return IsGamepadButtonDown(in0, StringToGamepadButton(rayenv->GetEnumAsString(in1))); }
extern "C" DLLEXPORT Sound* ray_LoadSound(std::string* in0) { return new Sound(LoadSound(in0->c_str())); }
extern "C" DLLEXPORT void ray_UnloadSound(Sound* in0) { UnloadSound(*in0); }
extern "C" DLLEXPORT void ray_PlaySound(Sound* in0) { PlaySound(*in0); }
extern "C" DLLEXPORT void ray_InitAudioDevice() { InitAudioDevice(); }
extern "C" DLLEXPORT Font* ray_LoadFont(std::string* in0) { return new Font(LoadFont(in0->c_str())); }
extern "C" DLLEXPORT void ray_DrawTextEx(Font* in0, std::string* in1, double in2a, double in2b, double in3, double in4, int32_t in5) { DrawTextEx(*in0, in1->c_str(), { float(in2a), float(in2b) }, in3, in4, StringToColor(rayenv->GetEnumAsString(in5))); }
extern "C" DLLEXPORT Image* ray_LoadImage(std::string* in0) { return new Image(LoadImage(in0->c_str())); }
extern "C" DLLEXPORT Texture2D* ray_LoadTextureFromImage(Image* in0) { return new Texture2D(LoadTextureFromImage(*in0)); }

//

static void LoadExtensions_Raylib(
    std::unique_ptr<llvm::IRBuilder<>>& builder,
    std::unique_ptr<llvm::Module>& module,
    Environment* env)
{
    rayenv = env;

    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_BeginDrawing", *module);
        env->DefineFunction("ray::BeginDrawing", ftn, {  }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_ClearBackground", *module);
        env->DefineFunction("ray::ClearBackground", ftn, { LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_CloseWindow", *module);
        env->DefineFunction("ray::CloseWindow", ftn, {  }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getDoubleTy());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawCircle", *module);
        env->DefineFunction("ray::DrawCircle", ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawLine", *module);
        env->DefineFunction("ray::DrawLine", ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawRectangle", *module);
        env->DefineFunction("ray::DrawRectangle", ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawText", *module);
        env->DefineFunction("ray::DrawText", ftn, { LITERAL_TYPE_STRING, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_EndDrawing", *module);
        env->DefineFunction("ray::EndDrawing", ftn, {  }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_InitWindow", *module);
        env->DefineFunction("ray::InitWindow", ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_STRING }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_IsKeyDown", *module);
        env->DefineFunction("ray::IsKeyDown", ftn, { LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_BOOL);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_GetMouseX", *module);
        env->DefineFunction("ray::GetMouseX", ftn, {  }, { }, args, {}, LITERAL_TYPE_INTEGER);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_GetMouseY", *module);
        env->DefineFunction("ray::GetMouseY", ftn, {  }, { }, args, {}, LITERAL_TYPE_INTEGER);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_SetTargetFPS", *module);
        env->DefineFunction("ray::SetTargetFPS", ftn, { LITERAL_TYPE_INTEGER }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_WindowShouldClose", *module);
        env->DefineFunction("ray::WindowShouldClose", ftn, {  }, { }, args, {}, LITERAL_TYPE_BOOL);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_LoadTexture", *module);
        env->DefineFunction("ray::LoadTexture", ftn, { LITERAL_TYPE_STRING }, { }, args, {}, LITERAL_TYPE_POINTER);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawTexture", *module);
        env->DefineFunction("ray::DrawTexture", ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getDoubleTy()); args.push_back(builder->getDoubleTy());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawTextureV", *module);
        env->DefineFunction("ray::DrawTextureV", ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getDoubleTy()); args.push_back(builder->getDoubleTy());
        args.push_back(builder->getDoubleTy());
        args.push_back(builder->getDoubleTy());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawTextureEx", *module);
        env->DefineFunction("ray::DrawTextureEx", ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getDoubleTy()); args.push_back(builder->getDoubleTy()); args.push_back(builder->getDoubleTy()); args.push_back(builder->getDoubleTy());
        args.push_back(builder->getDoubleTy()); args.push_back(builder->getDoubleTy()); args.push_back(builder->getDoubleTy()); args.push_back(builder->getDoubleTy());
        args.push_back(builder->getDoubleTy()); args.push_back(builder->getDoubleTy());
        args.push_back(builder->getDoubleTy());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawTexturePro", *module);
        env->DefineFunction("ray::DrawTexturePro", ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_IsGamepadAvailable", *module);
        env->DefineFunction("ray::IsGamepadAvailable", ftn, { LITERAL_TYPE_INTEGER }, { }, args, {}, LITERAL_TYPE_BOOL);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_GetGamepadAxisMovement", *module);
        env->DefineFunction("ray::GetGamepadAxisMovement", ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_DOUBLE);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_IsGamepadButtonDown", *module);
        env->DefineFunction("ray::IsGamepadButtonDown", ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_BOOL);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_LoadSound", *module);
        env->DefineFunction("ray::LoadSound", ftn, { LITERAL_TYPE_STRING }, { }, args, {}, LITERAL_TYPE_POINTER);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_UnloadSound", *module);
        env->DefineFunction("ray::UnloadSound", ftn, { LITERAL_TYPE_POINTER }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_PlaySound", *module);
        env->DefineFunction("ray::PlaySound", ftn, { LITERAL_TYPE_POINTER }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_InitAudioDevice", *module);
        env->DefineFunction("ray::InitAudioDevice", ftn, {  }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_LoadFont", *module);
        env->DefineFunction("ray::LoadFont", ftn, { LITERAL_TYPE_STRING }, { }, args, {}, LITERAL_TYPE_POINTER);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getDoubleTy()); args.push_back(builder->getDoubleTy());
        args.push_back(builder->getDoubleTy());
        args.push_back(builder->getDoubleTy());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawTextEx", *module);
        env->DefineFunction("ray::DrawTextEx", ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_STRING, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_ENUM }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_LoadImage", *module);
        env->DefineFunction("ray::LoadImage", ftn, { LITERAL_TYPE_STRING }, { }, args, {}, LITERAL_TYPE_POINTER);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_LoadTextureFromImage", *module);
        env->DefineFunction("ray::LoadTextureFromImage", ftn, { LITERAL_TYPE_POINTER }, { }, args, {}, LITERAL_TYPE_POINTER);
    }

    //-------------------------------------------------------------------------
    // manual functions
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_ImagePeek", *module);
        env->DefineFunction("ray::ImagePeek", ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER }, { }, args, {}, LITERAL_TYPE_INTEGER);
    }

    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getDoubleTy());
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawTextureTileMap", *module);
        env->DefineFunction("ray::DrawTextureTileMap", ftn, {
            LITERAL_TYPE_POINTER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER,
            LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_DOUBLE, LITERAL_TYPE_POINTER,
            LITERAL_TYPE_ENUM
            }, { }, args, {}, LITERAL_TYPE_INVALID);
    }
}

static Color StringToColor(const std::string& s)
{
    static bool first = true;
    static std::map<std::string, Color> enumMap;
    if (first)
    {
        first = false;
        enumMap.insert(std::make_pair(":LIGHTGRAY", LIGHTGRAY));
        enumMap.insert(std::make_pair(":GRAY", GRAY));
        enumMap.insert(std::make_pair(":DARKGRAY", Color{ 60, 60, 60, 255 }));
        enumMap.insert(std::make_pair(":DARKDARKGRAY", Color{ 20, 20, 20, 255 }));
        enumMap.insert(std::make_pair(":YELLOW", YELLOW));
        enumMap.insert(std::make_pair(":GOLD", GOLD));
        enumMap.insert(std::make_pair(":ORANGE", ORANGE));
        enumMap.insert(std::make_pair(":PINK", PINK));
        enumMap.insert(std::make_pair(":RED", RED));
        enumMap.insert(std::make_pair(":DARKRED", Color{ 128, 20, 25, 255 }));
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
        enumMap.insert(std::make_pair(":SHARKGRAY", Color{ 34, 32, 39, 255 }));
        enumMap.insert(std::make_pair(":SLATEGRAY", Color{ 140, 173, 181, 255 }));
        enumMap.insert(std::make_pair(":DARKSLATEGRAY", Color{ 67, 99, 107, 255 }));

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

static int StringToGamepadAxis(const std::string& s)
{
    static bool first = true;
    static std::map<std::string, int> enumMap;
    if (first)
    {
        first = false;
        enumMap.insert(std::make_pair(":GAMEPAD_AXIS_LEFT_X", GAMEPAD_AXIS_LEFT_X));
        enumMap.insert(std::make_pair(":GAMEPAD_AXIS_LEFT_Y", GAMEPAD_AXIS_LEFT_Y));
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
        enumMap.insert(std::make_pair(":GAMEPAD_BUTTON_RIGHT_FACE_UP", GAMEPAD_BUTTON_RIGHT_FACE_UP));
        enumMap.insert(std::make_pair(":GAMEPAD_BUTTON_RIGHT_FACE_DOWN", GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
        enumMap.insert(std::make_pair(":GAMEPAD_BUTTON_RIGHT_FACE_LEFT", GAMEPAD_BUTTON_RIGHT_FACE_LEFT));
        enumMap.insert(std::make_pair(":GAMEPAD_BUTTON_RIGHT_FACE_RIGHT", GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));
    }
    if (0 != enumMap.count(s)) return enumMap.at(s);
    return KEY_NULL;
}
#endif // EXTENSIONS_RAYLIB_H
