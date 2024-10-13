// [CATWALK:RBN] - Created 2024-10-10 (Catwalk 0.0.1)
//
// @file    cwShopping.cpp
// @brief   Catwalk Shopping functions
// @author  RBN
// @date    2024-10-10
// @version 0.0.1
//

#include "cwShopping.h"
#include "llassettype.h"
#include "lluuid.h"
#include "llstring.h"
#include "llinventory.h"
#include "llinventorymodel.h"
#include "llinventorytype.h"
#include "llinventorypanel.h"
#include "llcontrol.h"
#include "llinventoryfunctions.h"

extern LLInventoryModel gInventory;
extern LLControlGroup   gSavedPerAccountSettings;

// Consumed by llfloaterbuycontents.cpp LLFloaterBuyContents::onBtnBuy()
// Consumed by llviewermessage.cpp      LLOfferInfo::inventory_task_callback()

LLUUID cwGetFolderUUID(LLAssetType::EType cwType, std::string cwDesc)
{
    LLAssetType::EType cw_OfferedItemType;
    std::string        cw_OfferedItemName;
    LLUUID             cw_FolderDemoUUID;
    LLUUID             cw_FolderPurchaseUUID;
    LLUUID             cw_ObjectDemoUUID;
    LLUUID             cw_ObjectPurchaseUUID;
    std::string        cw_LowerItemName;
    bool               cw_isDemo;
    bool               cw_isObject;
    bool               cw_isFolder;

    cw_OfferedItemType = cwType;
    cw_OfferedItemName = cwDesc;
    cw_isObject        = (cw_OfferedItemType == LLAssetType::AT_OBJECT  );
    cw_isFolder        = (cw_OfferedItemType == LLAssetType::AT_CATEGORY);

    if (cw_isObject || cw_isFolder)
    {
        // Default UUIDs for demo and purchase folders
        // Temporary home until we can add them in user settings
        // cw_FolderDemoUUID     = LLUUID("7e4d2b00-35d7-3e93-866d-b91042956298");
        // cw_FolderPurchaseUUID = LLUUID("2c4524b0-e4d8-3b41-86ff-36548b012dfb");
        // cw_ObjectDemoUUID     = LLUUID("279b8f85-3015-3155-899d-494a08f0861d");
        // cw_ObjectPurchaseUUID = LLUUID("170e51e1-af49-319c-ac07-05de0af0fd92");

        cw_FolderDemoUUID     = cw_GetSavedShoppingFolderUUID("cw_DemoFolder");
        cw_FolderPurchaseUUID = cw_GetSavedShoppingFolderUUID("cw_PurchaseFolder");
        cw_ObjectDemoUUID     = cw_GetSavedShoppingFolderUUID("cw_DemoBoxedFolder");
        cw_ObjectPurchaseUUID = cw_GetSavedShoppingFolderUUID("cw_PurchaseBoxedFolder");

        cw_LowerItemName = cw_OfferedItemName;
        LLStringUtil::toLower(cw_LowerItemName);

        cw_isDemo = (cw_LowerItemName.find("demo") != std::string::npos);

        switch (cw_isDemo)
        {
            case true:
                if (cw_isObject)
                    return cw_ObjectDemoUUID;
                else if (cw_isFolder)
                    return cw_FolderDemoUUID;
                break;
            case false:
                if (cw_isObject)
                    return cw_ObjectPurchaseUUID;
                else if (cw_isFolder)
                    return cw_FolderPurchaseUUID;
                break;
        }
    }

    // Return the default folder for the given asset type
    LLFolderType::EType cw_folder_type = LLFolderType::assetTypeToFolderType(cw_OfferedItemType);
    LLUUID              cw_folder_uuid = gInventory.findCategoryUUIDForType(cw_folder_type);

    return cw_folder_uuid;
}

LLUUID cw_GetSavedShoppingFolderUUID(std::string cw_FolderName)
{
    LLUUID cw_FolderUUID;
    // Get the UUID from saved settings
    std::string savedUUID = gSavedPerAccountSettings.getString(cw_FolderName);
    
    // Check if the saved string is a valid UUID
    if (LLUUID::validate(savedUUID))
    {
        cw_FolderUUID = LLUUID(savedUUID);
    }
    else
    {
        // If not a valid UUID, use default folders based on the folder name
        if (cw_FolderName == "cw_DemoFolder" || cw_FolderName == "cw_PurchaseFolder")
        {
            // For regular folders, use the default folder for folders
            cw_FolderUUID = gInventory.findCategoryUUIDForType(LLFolderType::FT_CATEGORY);
        }
        else if (cw_FolderName == "cw_DemoBoxedFolder" || cw_FolderName == "cw_PurchaseBoxedFolder")
        {
            // For boxed objects, use the default folder for objects
            cw_FolderUUID = gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT);
        }
    }

    return cw_FolderUUID;
}

std::string cw_GetShoppingFolderPath(LLUUID cw_FolderUUID)
{
    return get_category_path(cw_FolderUUID);
}

void cw_ResetDefaultFolders()
{
    gSavedPerAccountSettings.set("cw_DemoFolder",           gInventory.findCategoryUUIDForType(LLFolderType::FT_CATEGORY).asString());
    gSavedPerAccountSettings.set("cw_PurchaseFolder",       gInventory.findCategoryUUIDForType(LLFolderType::FT_CATEGORY).asString());
    gSavedPerAccountSettings.set("cw_DemoBoxedFolder",      gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT).asString());
    gSavedPerAccountSettings.set("cw_PurchaseBoxedFolder",  gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT).asString());
}