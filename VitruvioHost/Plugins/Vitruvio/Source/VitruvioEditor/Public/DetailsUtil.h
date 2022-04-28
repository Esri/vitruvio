#pragma once

template <typename T>
TOptional<T> GetValue(const TSharedPtr<IPropertyHandle>& ValueProperty)
{
	if (!ValueProperty)
	{
		return TOptional<T>();
	}

	FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(ValueProperty->GetProperty());
	if (!StructProperty)
	{
		return TOptional<T>();
	}
	if (StructProperty->Struct != TBaseStructure<T>::Get())
	{
		return TOptional<T>();
	}

	T Value = T();
	bool bValueSet = false;

	TArray<void*> RawData;
	ValueProperty->AccessRawData(RawData);
	for (void* Data : RawData)
	{
		if (Data)
		{
			T CurValue = *reinterpret_cast<T*>(Data);
			if (!bValueSet)
			{
				bValueSet = true;
				Value = CurValue;
			}
			else if (CurValue != Value)
			{
				// Multiple values
				return TOptional<T>();
			}
		}
	}

	return TOptional<T>(Value);
}
