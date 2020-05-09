#pragma once

#ifdef _WIN32
#define CODEC_EXPORTS_API __declspec(dllexport)
#else
#define CODEC_EXPORTS_API __attribute__((visibility("default")))
#endif
