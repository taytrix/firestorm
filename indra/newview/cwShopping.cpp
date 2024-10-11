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

extern LLInventoryModel gInventory;

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
        cw_FolderDemoUUID     = LLUUID("7e4d2b00-35d7-3e93-866d-b91042956298");
        cw_FolderPurchaseUUID = LLUUID("2c4524b0-e4d8-3b41-86ff-36548b012dfb");
        cw_ObjectDemoUUID     = LLUUID("279b8f85-3015-3155-899d-494a08f0861d");
        cw_ObjectPurchaseUUID = LLUUID("170e51e1-af49-319c-ac07-05de0af0fd92");

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