// Copyright © 2017-2022 Esri R&D Center Zurich. All rights reserved.

#include "MaterialReplacement.h"

uint32 GetTypeHash(const UMaterialDescription& Object)
{
	uint32 Hash = 0x274110C5;
	Hash = HashCombine(Hash, GetMapHash<FString, FString>(Object.TextureProperties));
	Hash = HashCombine(Hash, GetMapHash<FString, FLinearColor>(Object.ColorProperties));
	Hash = HashCombine(Hash, GetMapHash<FString, double>(Object.ScalarProperties));
	Hash = HashCombine(Hash, GetMapHash<FString, FString>(Object.StringProperties));
	Hash = HashCombine(Hash, GetTypeHash(Object.BlendMode));
	return Hash;
}

bool MaterialEquals(UMaterialInterface* Lhs, UMaterialInterface* Rhs)
{
	if (!Lhs || !Rhs)
	{
		return false;
	}

	return true;
}
