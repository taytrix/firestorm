// [CATWALK:RBN] - Created 2024-10-10 (Catwalk 0.0.1)
//
// @file    cwShopping.h
// @brief   Catwalk Shopping functions
// @author  RBN
// @date    2024-10-10
// @version 0.0.1
//

#include "lluuid.h"
#include "llassettype.h"

/**
 * @brief Retrieves the UUID of a folder based on the asset type and description.
 * 
 * Helps sort new items into folders with an educated guess on whether it's a demo
 * or a purchase. Keeps the root Inventory folder, and Objects folder clean.
 * 
 * @param cwType The type of the asset (e.g., object, folder).
 * @param cwDesc The description of the asset which we search for "demo" in.
 * @return LLUUID The UUID of the corresponding folder.
 */
LLUUID cwGetFolderUUID(LLAssetType::EType cwType, std::string cwDesc);

/**
 * @brief Resets the default object and category folders to the default values.
 */
void cw_ResetDefaultFolders();

/**
 * @brief Retrieves the UUID of a folder based on the folder name.
 * 
 * @param cw_FolderName The name of the folder.
 * @return LLUUID The UUID of the corresponding folder.
 */
LLUUID cw_GetSavedShoppingFolderUUID(std::string cw_FolderName);

/**
 * @brief Retrieves the path of a folder based on the folder UUID.
 */
std::string cw_GetShoppingFolderPath(LLUUID cw_FolderUUID);
