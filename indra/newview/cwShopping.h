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