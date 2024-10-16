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
    LL_INFOS() << "Entering cwGetFolderUUID" << LL_ENDL;

    if (cw_areNullOrInvalidSavedFolders())
    {
        LL_INFOS() << "Initializing missing folders" << LL_ENDL;
        cw_InitializeMissingFolders();
    }

    LLAssetType::EType cw_OfferedItemType = cwType;
    std::string        cw_OfferedItemName = cwDesc;
    bool               cw_isObject        = (cw_OfferedItemType == LLAssetType::AT_OBJECT);
    bool               cw_isFolder        = (cw_OfferedItemType == LLAssetType::AT_CATEGORY);

    if (cw_isObject || cw_isFolder)
    {
        // Default UUIDs for demo and purchase folders
        // Temporary home until we can add them in user settings
        // cw_FolderDemoUUID     = LLUUID("7e4d2b00-35d7-3e93-866d-b91042956298");
        // cw_FolderPurchaseUUID = LLUUID("2c4524b0-e4d8-3b41-86ff-36548b012dfb");
        // cw_ObjectDemoUUID     = LLUUID("279b8f85-3015-3155-899d-494a08f0861d");
        // cw_ObjectPurchaseUUID = LLUUID("170e51e1-af49-319c-ac07-05de0af0fd92");

        LL_INFOS() << "Getting saved shopping folder UUIDs" << LL_ENDL;
        LLUUID cw_FolderDemoUUID     = cw_GetSavedShoppingFolderUUID("cw_DemoFolder");
        LLUUID cw_FolderPurchaseUUID = cw_GetSavedShoppingFolderUUID("cw_PurchaseFolder");
        LLUUID cw_ObjectDemoUUID     = cw_GetSavedShoppingFolderUUID("cw_DemoBoxedFolder");
        LLUUID cw_ObjectPurchaseUUID = cw_GetSavedShoppingFolderUUID("cw_PurchaseBoxedFolder");

        // // Check if any of the UUIDs are null
        // if (cw_FolderDemoUUID.isNull() || cw_FolderPurchaseUUID.isNull() || 
        //     cw_ObjectDemoUUID.isNull() || cw_ObjectPurchaseUUID.isNull())
        // {
        //     LL_WARNS() << "One or more shopping folder UUIDs are null" << LL_ENDL;
        //     // Fall back to default folder
        //     return gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(cw_OfferedItemType));
        // }

        std::string cw_LowerItemName = cw_OfferedItemName;
        LLStringUtil::toLower(cw_LowerItemName);

        bool cw_isDemo = (cw_LowerItemName.find("demo") != std::string::npos);

        if (cw_isDemo)
        {
            return cw_isObject ? cw_ObjectDemoUUID : cw_FolderDemoUUID;
        }
        else
        {
            return cw_isObject ? cw_ObjectPurchaseUUID : cw_FolderPurchaseUUID;
        }
    }

    // Return the default folder for the given asset type
    LL_INFOS() << "Returning the default folder for the given asset type" << LL_ENDL;
    LLFolderType::EType cw_folder_type = LLFolderType::assetTypeToFolderType(cw_OfferedItemType);
    LLUUID              cw_folder_uuid = gInventory.findCategoryUUIDForType(cw_folder_type);

    LL_INFOS() << "Exiting cwGetFolderUUID" << LL_ENDL;
    return cw_folder_uuid;
}

LLUUID cw_GetSavedShoppingFolderUUID(std::string cw_FolderName)
{
    LLUUID cw_FolderUUID;

    // // Check if gSavedPerAccountSettings is initialized
    // if (!LLControlGroup::getInstance("PerAccount"))
    // {
    //     LL_WARNS() << "PerAccount settings not initialized" << LL_ENDL;
    //     return LLUUID::null;
    // }

    try
    {
        LL_INFOS() << "Getting saved shopping folder UUID: " << cw_FolderName << LL_ENDL;
        // Get the UUID from saved settings
        std::string savedUUID = gSavedPerAccountSettings.getString(cw_FolderName);
        
        // Check if the saved string is not empty and is a valid UUID
        if (!savedUUID.empty() && LLUUID::validate(savedUUID))
        {
            LL_INFOS() << "Saved UUID is valid" << LL_ENDL;
            cw_FolderUUID = LLUUID(savedUUID);
        }
        else
        {
            LL_INFOS() << "Saved UUID is not valid, using default folders based on the folder name" << LL_ENDL;
            // If not a valid UUID or empty, use default folders based on the folder name
            if (cw_FolderName == "cw_DemoFolder" || cw_FolderName == "cw_PurchaseFolder")
            {
                // For regular folders, use the default folder for folders
                cw_FolderUUID = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);
            }
            else if (cw_FolderName == "cw_DemoBoxedFolder" || cw_FolderName == "cw_PurchaseBoxedFolder")
            {
                // For boxed objects, use the default folder for objects
                cw_FolderUUID = gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT);
            }
            
            // Save the default UUID back to the settings
            gSavedPerAccountSettings.setString(cw_FolderName, cw_FolderUUID.asString());
        }
    }
    catch (const std::exception& e)
    {
        LL_WARNS() << "Exception in cw_GetSavedShoppingFolderUUID: " << e.what() << LL_ENDL;
        return LLUUID::null;
    }

    return cw_FolderUUID;
}

std::string cw_GetShoppingFolderPath(LLUUID cw_FolderUUID)
{
    return get_category_path(cw_FolderUUID);
}

void cw_ResetDefaultFolders()
{
    gSavedPerAccountSettings.set("cw_DemoFolder",           gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY).asString());
    gSavedPerAccountSettings.set("cw_PurchaseFolder",       gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY).asString());
    gSavedPerAccountSettings.set("cw_DemoBoxedFolder",      gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT).asString());
    gSavedPerAccountSettings.set("cw_PurchaseBoxedFolder",  gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT).asString());
}

// Returns true if any saved folders are null or invalid
bool cw_areNullOrInvalidSavedFolders()
{
    LL_INFOS() << "Entering cw_areNullOrInvalidSavedFolders" << LL_ENDL;
    try
    {
        std::vector<std::string> folderNames = {
            "cw_DemoFolder", "cw_PurchaseFolder", 
            "cw_DemoBoxedFolder", "cw_PurchaseBoxedFolder"
        };

        for (const auto& folderName : folderNames)
        {
            std::string savedUUID = gSavedPerAccountSettings.getString(folderName);
            if (savedUUID.empty() || !LLUUID::validate(savedUUID))
            {
                LL_INFOS() << "Found an invalid or null UUID: " << savedUUID << LL_ENDL;
                return true; // Found an invalid or null UUID
            }
        }

        return false; // All UUIDs are valid and non-null
    }
    catch (const std::exception& e)
    {
        LL_WARNS() << "Exception in cw_areNullOrInvalidSavedFolders: " << e.what() << LL_ENDL;
        return true; // Assume there's an issue if an exception occurs
    }
}

// Initialize any missing folders without overwriting values of existing valid saved folders
void cw_InitializeMissingFolders()
{
    LL_INFOS() << "Entering cw_InitializeMissingFolders" << LL_ENDL;
    try
    {
        if (cw_areNullOrInvalidSavedFolders())
        {
            std::vector<std::string> folderNames = {
                "cw_DemoFolder", "cw_PurchaseFolder", 
                "cw_DemoBoxedFolder", "cw_PurchaseBoxedFolder"
            };

            for (const auto& folderName : folderNames)
            {
                LLUUID savedUUID = LLUUID(gSavedPerAccountSettings.getString(folderName));
                if (savedUUID.isNull() || !LLUUID::validate(savedUUID.asString()))
                {
                    LL_INFOS() << "Initializing missing folder: " << folderName << LL_ENDL;
                    LLUUID newUUID = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(LLAssetType::AT_CATEGORY));
                    gSavedPerAccountSettings.setString(folderName, newUUID.asString());
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        LL_WARNS() << "Exception in cw_InitializeMissingFolders: " << e.what() << LL_ENDL;
    }
}
