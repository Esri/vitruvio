// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "UnrealGeometryEncoderModule.h"

#define LOCTEXT_NAMESPACE "UnrealGeometryEncoderModule"

void UnrealGeometryEncoderModule::StartupModule() {}

void UnrealGeometryEncoderModule::ShutdownModule() {}

#undef LOCTEXT_NAMESPACE

#if IS_MONOLITHIC

// If we're linking monolithically we assume all modules are linked in with the main binary.
#define IMPLEMENT_MODULE_UNREAL(ModuleImplClass, ModuleName)                                                                                         \
	/** Global registrant object for this module when linked statically */                                                                           \
	static FStaticallyLinkedModuleRegistrant<ModuleImplClass> ModuleRegistrant##ModuleName(TEXT(#ModuleName));                                       \
	/* Forced reference to this function is added by the linker to check that each module uses IMPLEMENT_MODULE */                                   \
	extern "C" void IMPLEMENT_MODULE_##ModuleName() {}                                                                                               \
	PER_MODULE_BOILERPLATE_ANYLINK(ModuleImplClass, ModuleName)

#else

// We don't want to overwrite new/delete for the module but setting FORCE_ANSI_ALLOCATOR to 1 does not work so we change
// macros defined in ModuleBoilerplate.h to just not overwrite them.

#define IMPLEMENT_MODULE_UNREAL(ModuleImplClass, ModuleName)                                                                                         \
                                                                                                                                                     \
	/**/                                                                                                                                             \
	/* InitializeModule function, called by module manager after this module's DLL has been loaded */                                                \
	/**/                                                                                                                                             \
	/* @return	Returns an instance of this module */                                                                                                 \
	/**/                                                                                                                                             \
	extern "C" DLLEXPORT IModuleInterface* InitializeModule() { return new ModuleImplClass(); }                                                      \
	/* Forced reference to this function is added by the linker to check that each module uses IMPLEMENT_MODULE */                                   \
	extern "C" void IMPLEMENT_MODULE_##ModuleName() {}                                                                                               \
	PER_MODULE_BOILERPLATE_ANYLINK(ModuleImplClass, ModuleName)

#endif // IS_MONOLITHIC

IMPLEMENT_MODULE_UNREAL(UnrealGeometryEncoderModule, UnrealGeometryEncoder)
