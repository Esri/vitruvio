#include "UnrealGeometryEncoderModule.h"

#define LOCTEXT_NAMESPACE "UnrealGeometryEncoderModule"

void UnrealGeometryEncoderModule::StartupModule()
{
}

void UnrealGeometryEncoderModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

// We don't want to overwrite new/delete for the module but setting FORCE_ANSI_ALLOCATOR to 1 does not work so we change
// macros defined in ModuleBoilerplate.h to just not overwrite them.

#define IMPLEMENT_MODULE_UNREAL(ModuleImplClass, ModuleName)                                                                                                                       \
                                                                                                                                                                                   \
	/**/                                                                                                                                                                           \
	/* InitializeModule function, called by module manager after this module's DLL has been loaded */                                                                              \
	/**/                                                                                                                                                                           \
	/* @return	Returns an instance of this module */                                                                                                                               \
	/**/                                                                                                                                                                           \
	extern "C" DLLEXPORT IModuleInterface* InitializeModule()                                                                                                                      \
	{                                                                                                                                                                              \
		return new ModuleImplClass();                                                                                                                                              \
	}                                                                                                                                                                              \
	/* Forced reference to this function is added by the linker to check that each module uses IMPLEMENT_MODULE */                                                                 \
	extern "C" void IMPLEMENT_MODULE_##ModuleName()                                                                                                                                \
	{                                                                                                                                                                              \
	}                                                                                                                                                                              \
	PER_MODULE_BOILERPLATE_ANYLINK(ModuleImplClass, ModuleName)

IMPLEMENT_MODULE_UNREAL(UnrealGeometryEncoderModule, UnrealGeometryEncoder)
