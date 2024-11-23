#ifndef EXTENSIONS_RAYLIB_H
#define EXTENSIONS_RAYLIB_H
#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

#include "Environment.h"
#include <raylib.h>
#define GLSL_VERSION 330
#include <rlgl.h>

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

extern "C" DLLEXPORT void ray_DrawTextureTileMap(Texture2D* in0, int32_t xx, int32_t yy, int32_t width, int32_t w, int32_t h, double scale, TVec* srcPtr, int32_t color)
{
    Texture2D tex = *in0;
    int32_t* tiles = static_cast<int32_t*>(srcPtr->Data());
    double rot = 0;
    int32_t height = srcPtr->Size() / width;
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
    return 0;
}

extern "C" DLLEXPORT int32_t ray_rlLoadVertexBuffer(TVec* vecPtr, bool dynamic)
{
    float* src = static_cast<float*>(vecPtr->Data());
    return rlLoadVertexBuffer(src, vecPtr->Size() * sizeof(float), dynamic);
}

extern "C" DLLEXPORT int32_t ray_rlLoadVertexBufferElement(TVec* vecPtr, bool dynamic)
{
    int32_t* src = static_cast<int32_t*>(vecPtr->Data());
    llvm::SmallVector<unsigned short> tmp;
    tmp.reserve(vecPtr->Size());
    for (size_t i = 0; i < vecPtr->Size(); ++i)
    {
        tmp.push_back(src[i]);
    }
    return rlLoadVertexBufferElement(tmp.data(), tmp.size() * sizeof(unsigned short), dynamic);
}

extern "C" DLLEXPORT void ray_rlSetVertexAttribute(int32_t in0, int32_t in1, int32_t in2, bool in3, int32_t in4, int32_t in5)
{
    size_t offsetNative = in5;
    rlSetVertexAttribute(in0, in1, in2, in3, in4, (void*)offsetNative);
}

extern "C" DLLEXPORT void ray_rlDrawVertexArrayElements(int32_t in0, int32_t in1, int32_t in2)
{
    rlDrawVertexArrayElements(in0, in1, (void*)in2);
}

extern "C" DLLEXPORT void ray_rlSetUniform(int32_t in0, TVec* in1, int32_t in2)
{
    void* ptr = nullptr;
    int sz = 0;

    switch (in2)
    {
    case RL_SHADER_UNIFORM_FLOAT: // intentional fall-through
    case RL_SHADER_UNIFORM_VEC2:
    case RL_SHADER_UNIFORM_VEC3:
    case RL_SHADER_UNIFORM_VEC4:
    {
        float* src = static_cast<float*>(in1->Data());
        llvm::SmallVector<float> tmp;
        tmp.reserve(in1->Size());
        for (size_t i = 0; i < in1->Size(); ++i)
        {
            tmp.push_back(src[i]);
        }
        ptr = static_cast<void*>(tmp.data());
        sz = tmp.size();
        break;
    }
    case RL_SHADER_UNIFORM_INT: // intentional fall-through
    case RL_SHADER_UNIFORM_IVEC2:
    case RL_SHADER_UNIFORM_IVEC3:
    case RL_SHADER_UNIFORM_IVEC4:
    case RL_SHADER_UNIFORM_SAMPLER2D:
    {
        ptr = static_cast<void*>(in1->Data());
        sz = in1->Size();
        break;
    }
    }

    if (ptr && sz) rlSetUniform(in0, ptr, in2, sz);
}


extern "C" DLLEXPORT int32_t ray_GetTextureId(Texture2D* texPtr)
{
    return texPtr->id;
}

extern "C" DLLEXPORT void ray_test()
{

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
extern "C" DLLEXPORT void ray_UnloadImage(Image* in0) { UnloadImage(*in0); }
extern "C" DLLEXPORT Texture2D* ray_LoadTextureFromImage(Image* in0) { return new Texture2D(LoadTextureFromImage(*in0)); }
extern "C" DLLEXPORT void ray_UnloadTexture(Texture2D* in0) { UnloadTexture(*in0); }
extern "C" DLLEXPORT void ray_ImageFlipVertical(Image* in0) { ImageFlipVertical(in0); }
extern "C" DLLEXPORT void ray_ImageFlipHorizontal(Image* in0) { ImageFlipHorizontal(in0); }
extern "C" DLLEXPORT void ray_rlClearColor(int32_t in0, int32_t in1, int32_t in2, int32_t in3) { rlClearColor(in0, in1, in2, in3); }
extern "C" DLLEXPORT Shader* ray_LoadShader(std::string* in0, std::string* in1) { return new Shader(LoadShader(in0->c_str(), in1->c_str())); }
extern "C" DLLEXPORT int32_t ray_rlLoadVertexArray() { return rlLoadVertexArray(); }
extern "C" DLLEXPORT void ray_rlEnableVertexArray(int32_t in0) { rlEnableVertexArray(in0); }
extern "C" DLLEXPORT void ray_rlDisableVertexArray() { rlDisableVertexArray(); }
extern "C" DLLEXPORT void ray_rlClearScreenBuffers() { rlClearScreenBuffers(); }
extern "C" DLLEXPORT void ray_BeginShaderMode(Shader* in0) { BeginShaderMode(*in0); }
extern "C" DLLEXPORT void ray_rlEnableVertexAttribute(int32_t in0) { rlEnableVertexAttribute(in0); }
extern "C" DLLEXPORT void ray_rlEnableVertexBuffer(int32_t in0) { rlEnableVertexBuffer(in0); }
extern "C" DLLEXPORT void ray_rlDisableVertexBuffer() { rlDisableVertexBuffer(); }
extern "C" DLLEXPORT void ray_rlDrawVertexArray(int32_t in0, int32_t in1) { rlDrawVertexArray(in0, in1); }
extern "C" DLLEXPORT void ray_rlDisableVertexAttribute(int32_t in0) { rlDisableVertexAttribute(in0); }
extern "C" DLLEXPORT void ray_EndShaderMode() { EndShaderMode(); }
extern "C" DLLEXPORT void ray_rlUnloadVertexBuffer(int32_t in0) { rlUnloadVertexBuffer(in0); }
extern "C" DLLEXPORT void ray_rlUnloadVertexArray(int32_t in0) { rlUnloadVertexArray(in0); }
extern "C" DLLEXPORT void ray_UnloadShader(Shader* in0) { UnloadShader(*in0); }
extern "C" DLLEXPORT void ray_rlEnableShader(int32_t in0) { rlEnableShader(in0); }
extern "C" DLLEXPORT void ray_rlDisableShader() { rlDisableShader(); }
extern "C" DLLEXPORT int32_t ray_rlLoadShaderCode(std::string* in0, std::string* in1) { return rlLoadShaderCode(in0->c_str(), in1->c_str()); }
extern "C" DLLEXPORT void ray_rlUnloadShaderProgram(int32_t in0) { rlUnloadShaderProgram(in0); }
extern "C" DLLEXPORT int32_t ray_rlGetLocationUniform(int32_t in0, std::string* in1) { return rlGetLocationUniform(in0, in1->c_str()); }
extern "C" DLLEXPORT void ray_rlEnableVertexBufferElement(int32_t in0) { rlEnableVertexBufferElement(in0); }
extern "C" DLLEXPORT void ray_rlDisableVertexBufferElement() { rlDisableVertexBufferElement(); }
extern "C" DLLEXPORT void ray_rlEnableTexture(int32_t in0) { rlEnableTexture(in0); }
extern "C" DLLEXPORT void ray_rlDisableTexture() { rlDisableTexture(); }
extern "C" DLLEXPORT void ray_rlCheckErrors() { rlCheckErrors(); }
extern "C" DLLEXPORT void ray_rlActiveTextureSlot(int32_t in0) { rlActiveTextureSlot(in0); }
extern "C" DLLEXPORT void ray_rlDisableBackfaceCulling() { rlDisableBackfaceCulling(); }

//

static void LoadExtensions_Raylib(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
    rayenv = env;

    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_BeginDrawing", *module);
        std::string lexeme = "ray::BeginDrawing";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_ClearBackground", *module);
        std::string lexeme = "ray::ClearBackground";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_CloseWindow", *module);
        std::string lexeme = "ray::CloseWindow";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getDoubleTy());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawCircle", *module);
        std::string lexeme = "ray::DrawCircle";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_FLOAT, LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_INVALID), lexeme);
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
        std::string lexeme = "ray::DrawLine";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_INVALID), lexeme);
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
        std::string lexeme = "ray::DrawRectangle";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_INVALID), lexeme);
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
        std::string lexeme = "ray::DrawText";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_STRING, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_EndDrawing", *module);
        std::string lexeme = "ray::EndDrawing";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_InitWindow", *module);
        std::string lexeme = "ray::InitWindow";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_STRING }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_IsKeyDown", *module);
        std::string lexeme = "ray::IsKeyDown";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_BOOL), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_GetMouseX", *module);
        std::string lexeme = "ray::GetMouseX";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INTEGER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_GetMouseY", *module);
        std::string lexeme = "ray::GetMouseY";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INTEGER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_SetTargetFPS", *module);
        std::string lexeme = "ray::SetTargetFPS";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_WindowShouldClose", *module);
        std::string lexeme = "ray::WindowShouldClose";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_BOOL), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_LoadTexture", *module);
        std::string lexeme = "ray::LoadTexture";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_STRING }, args, LITERAL_TYPE_POINTER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawTexture", *module);
        std::string lexeme = "ray::DrawTexture";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getDoubleTy()); args.push_back(builder->getDoubleTy());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_DrawTextureV", *module);
        std::string lexeme = "ray::DrawTextureV";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_INVALID), lexeme);
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
        std::string lexeme = "ray::DrawTextureEx";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_INVALID), lexeme);
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
        std::string lexeme = "ray::DrawTexturePro";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_IsGamepadAvailable", *module);
        std::string lexeme = "ray::IsGamepadAvailable";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_BOOL), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_GetGamepadAxisMovement", *module);
        std::string lexeme = "ray::GetGamepadAxisMovement";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_FLOAT), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_IsGamepadButtonDown", *module);
        std::string lexeme = "ray::IsGamepadButtonDown";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_BOOL), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_LoadSound", *module);
        std::string lexeme = "ray::LoadSound";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_STRING }, args, LITERAL_TYPE_POINTER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_UnloadSound", *module);
        std::string lexeme = "ray::UnloadSound";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_PlaySound", *module);
        std::string lexeme = "ray::PlaySound";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_InitAudioDevice", *module);
        std::string lexeme = "ray::InitAudioDevice";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_LoadFont", *module);
        std::string lexeme = "ray::LoadFont";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_STRING }, args, LITERAL_TYPE_POINTER), lexeme);
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
        std::string lexeme = "ray::DrawTextEx";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_STRING, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_FLOAT, LITERAL_TYPE_ENUM }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_LoadImage", *module);
        std::string lexeme = "ray::LoadImage";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_STRING }, args, LITERAL_TYPE_POINTER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_UnloadImage", *module);
        std::string lexeme = "ray::UnloadImage";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_LoadTextureFromImage", *module);
        std::string lexeme = "ray::LoadTextureFromImage";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER }, args, LITERAL_TYPE_POINTER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_UnloadTexture", *module);
        std::string lexeme = "ray::UnloadTexture";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_ImageFlipVertical", *module);
        std::string lexeme = "ray::ImageFlipVertical";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_ImageFlipHorizontal", *module);
        std::string lexeme = "ray::ImageFlipHorizontal";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlClearColor", *module);
        std::string lexeme = "ray::rlClearColor";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_LoadShader", *module);
        std::string lexeme = "ray::LoadShader";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_STRING, LITERAL_TYPE_STRING }, args, LITERAL_TYPE_POINTER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlLoadVertexArray", *module);
        std::string lexeme = "ray::rlLoadVertexArray";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INTEGER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlEnableVertexArray", *module);
        std::string lexeme = "ray::rlEnableVertexArray";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlDisableVertexArray", *module);
        std::string lexeme = "ray::rlDisableVertexArray";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlClearScreenBuffers", *module);
        std::string lexeme = "ray::rlClearScreenBuffers";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_BeginShaderMode", *module);
        std::string lexeme = "ray::BeginShaderMode";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlEnableVertexAttribute", *module);
        std::string lexeme = "ray::rlEnableVertexAttribute";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlEnableVertexBuffer", *module);
        std::string lexeme = "ray::rlEnableVertexBuffer";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlDisableVertexBuffer", *module);
        std::string lexeme = "ray::rlDisableVertexBuffer";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlDrawVertexArray", *module);
        std::string lexeme = "ray::rlDrawVertexArray";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlDisableVertexAttribute", *module);
        std::string lexeme = "ray::rlDisableVertexAttribute";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_EndShaderMode", *module);
        std::string lexeme = "ray::EndShaderMode";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlUnloadVertexBuffer", *module);
        std::string lexeme = "ray::rlUnloadVertexBuffer";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlUnloadVertexArray", *module);
        std::string lexeme = "ray::rlUnloadVertexArray";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_UnloadShader", *module);
        std::string lexeme = "ray::UnloadShader";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlEnableShader", *module);
        std::string lexeme = "ray::rlEnableShader";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlDisableShader", *module);
        std::string lexeme = "ray::rlDisableShader";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlLoadShaderCode", *module);
        std::string lexeme = "ray::rlLoadShaderCode";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_STRING, LITERAL_TYPE_STRING }, args, LITERAL_TYPE_INTEGER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlUnloadShaderProgram", *module);
        std::string lexeme = "ray::rlUnloadShaderProgram";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlGetLocationUniform", *module);
        std::string lexeme = "ray::rlGetLocationUniform";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_STRING }, args, LITERAL_TYPE_INTEGER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlEnableVertexBufferElement", *module);
        std::string lexeme = "ray::rlEnableVertexBufferElement";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlDisableVertexBufferElement", *module);
        std::string lexeme = "ray::rlDisableVertexBufferElement";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlEnableTexture", *module);
        std::string lexeme = "ray::rlEnableTexture";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlDisableTexture", *module);
        std::string lexeme = "ray::rlDisableTexture";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlCheckErrors", *module);
        std::string lexeme = "ray::rlCheckErrors";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlActiveTextureSlot", *module);
        std::string lexeme = "ray::rlActiveTextureSlot";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlDisableBackfaceCulling", *module);
        std::string lexeme = "ray::rlDisableBackfaceCulling";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {  }, args, LITERAL_TYPE_INVALID), lexeme);
    }

    //-------------------------------------------------------------------------
    // manual functions
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
        std::string lexeme = "ray::DrawTextureTileMap";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, {
            LITERAL_TYPE_POINTER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER,
            LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_FLOAT, LITERAL_TYPE_POINTER,
            LITERAL_TYPE_ENUM
            }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_ImagePeek", *module);
        std::string lexeme = "ray::ImagePeek";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INTEGER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt1Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlSetVertexAttribute", *module);
        std::string lexeme = "ray::rlSetVertexAttribute";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_BOOL, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getInt1Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlLoadVertexBuffer", *module);
        std::string lexeme = "ray::rlLoadVertexBuffer";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_BOOL }, args, LITERAL_TYPE_INTEGER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getInt1Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlLoadVertexBufferElement", *module);
        std::string lexeme = "ray::rlLoadVertexBufferElement";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER, LITERAL_TYPE_BOOL }, args, LITERAL_TYPE_INTEGER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getPtrTy());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlSetUniform", *module);
        std::string lexeme = "ray::rlSetUniform";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_POINTER, LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getPtrTy());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_GetTextureId", *module);
        std::string lexeme = "ray::GetTextureId";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_POINTER }, args, LITERAL_TYPE_INTEGER), lexeme);
    }
    {
        std::vector<llvm::Type*> args;
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        args.push_back(builder->getInt32Ty());
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_rlDrawVertexArrayElements", *module);
        std::string lexeme = "ray::rlDrawVertexArrayElements";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER, LITERAL_TYPE_INTEGER }, args, LITERAL_TYPE_INVALID), lexeme);
    }

    {
        llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), {}, false);
        llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ray_test", *module);
        std::string lexeme = "ray::test";
        env->DefineFunction(TFunction::Construct_Internal(lexeme, ftn, { }, {}, LITERAL_TYPE_INVALID), lexeme);
    }


    // Inject enums into environment
    {
        std::vector<int> vals = {
            RL_FLOAT,
            RL_SHADER_UNIFORM_FLOAT,
            RL_SHADER_UNIFORM_VEC2,
            RL_SHADER_UNIFORM_VEC3,
            RL_SHADER_UNIFORM_VEC4,
            RL_SHADER_UNIFORM_INT,
            RL_SHADER_UNIFORM_IVEC2,
            RL_SHADER_UNIFORM_IVEC3,
            RL_SHADER_UNIFORM_IVEC4,
            RL_SHADER_UNIFORM_SAMPLER2D
        };

        std::vector<std::string> names = {
            "RL_FLOAT",
            "RL_SHADER_UNIFORM_FLOAT",
            "RL_SHADER_UNIFORM_VEC2",
            "RL_SHADER_UNIFORM_VEC3",
            "RL_SHADER_UNIFORM_VEC4",
            "RL_SHADER_UNIFORM_INT",
            "RL_SHADER_UNIFORM_IVEC2",
            "RL_SHADER_UNIFORM_IVEC3",
            "RL_SHADER_UNIFORM_IVEC4",
            "RL_SHADER_UNIFORM_SAMPLER2D"
        };

        for (size_t i = 0; i < vals.size(); ++i)
        {
            llvm::Value* defval = new llvm::GlobalVariable(*module, builder->getInt32Ty(), false, llvm::GlobalValue::InternalLinkage, builder->getInt32(vals[i]), names[i]);
            TValue v = TValue::MakeInt(nullptr, 32, defval);
            v.SetStorage(true);
            env->DefineVariable(v, names[i]);
        }
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
