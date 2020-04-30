#include "PRTActorDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailGroup.h"

TSharedRef<IDetailCustomization> PRTActorDetails::MakeInstance()
{
	return MakeShareable(new PRTActorDetails);
}

void PRTActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{

}
