/** 
 * @file llfloaterperformance.cpp
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llfloaterperformance.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llappearancemgr.h"
#include "llavataractions.h"
#include "llavatarrendernotifier.h"
#include "llcheckboxctrl.h"
#include "llfeaturemanager.h"
#include "llfloaterpreference.h" // LLAvatarComplexityControls
#include "llfloaterreg.h"
#include "llnamelistctrl.h"
#include "llradiogroup.h"
#include "llsliderctrl.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llvoavatarself.h" 
#include "pipeline.h"
#include "llviewercontrol.h"
#include "fsavatarrenderpersistence.h"

const F32 REFRESH_INTERVAL = 1.0f;
const S32 BAR_LEFT_PAD = 2;
const S32 BAR_RIGHT_PAD = 5;
const S32 BAR_BOTTOM_PAD = 9;

class LLExceptionsContextMenu : public LLListContextMenu
{
public:
    LLExceptionsContextMenu(LLFloaterPerformance* floater_settings)
        :   mFloaterPerformance(floater_settings)
    {}
protected:
    LLContextMenu* createMenu()
    {
        LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
        LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
        registrar.add("Settings.SetRendering", boost::bind(&LLFloaterPerformance::onCustomAction, mFloaterPerformance, _2, mUUIDs.front()));
        enable_registrar.add("Settings.IsSelected", boost::bind(&LLFloaterPerformance::isActionChecked, mFloaterPerformance, _2, mUUIDs.front()));
        LLContextMenu* menu = createFromFile("menu_avatar_rendering_settings.xml");

        return menu;
    }

    LLFloaterPerformance* mFloaterPerformance;
};

LLFloaterPerformance::LLFloaterPerformance(const LLSD& key)
:   LLFloater(key),
    mUpdateTimer(new LLTimer()),
    mNearbyMaxComplexity(0)
{
    mContextMenu = new LLExceptionsContextMenu(this);
}

LLFloaterPerformance::~LLFloaterPerformance()
{
    mComplexityChangedSignal.disconnect();
    delete mContextMenu;
    delete mUpdateTimer;
}

BOOL LLFloaterPerformance::postBuild()
{
    mMainPanel = getChild<LLPanel>("panel_performance_main");
    mNearbyPanel = getChild<LLPanel>("panel_performance_nearby");
    mComplexityPanel = getChild<LLPanel>("panel_performance_complexity");
    mSettingsPanel = getChild<LLPanel>("panel_performance_preferences");
    mHUDsPanel = getChild<LLPanel>("panel_performance_huds");

    getChild<LLPanel>("nearby_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mNearbyPanel));
    getChild<LLPanel>("complexity_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mComplexityPanel));
    getChild<LLPanel>("settings_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mSettingsPanel));
    getChild<LLPanel>("huds_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mHUDsPanel));

    initBackBtn(mNearbyPanel);
    initBackBtn(mComplexityPanel);
    initBackBtn(mSettingsPanel);
    initBackBtn(mHUDsPanel);

    mHUDList = mHUDsPanel->getChild<LLNameListCtrl>("hud_list");
    mHUDList->setNameListType(LLNameListCtrl::SPECIAL);
    mHUDList->setHoverIconName("StopReload_Off");
    mHUDList->setIconClickedCallback(boost::bind(&LLFloaterPerformance::detachItem, this, _1));

    mObjectList = mComplexityPanel->getChild<LLNameListCtrl>("obj_list");
    mObjectList->setNameListType(LLNameListCtrl::SPECIAL);
    mObjectList->setHoverIconName("StopReload_Off");
    mObjectList->setIconClickedCallback(boost::bind(&LLFloaterPerformance::detachItem, this, _1));

    mSettingsPanel->getChild<LLButton>("advanced_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickAdvanced, this));
    mSettingsPanel->getChild<LLRadioGroup>("graphics_quality")->setCommitCallback(boost::bind(&LLFloaterPerformance::onChangeQuality, this, _2));

    mNearbyPanel->getChild<LLButton>("exceptions_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickExceptions, this));
    mNearbyPanel->getChild<LLCheckBoxCtrl>("hide_avatars")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickHideAvatars, this));
    mNearbyPanel->getChild<LLCheckBoxCtrl>("hide_avatars")->set(!LLPipeline::hasRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR));
    mNearbyList = mNearbyPanel->getChild<LLNameListCtrl>("nearby_list");
    mNearbyList->setRightMouseDownCallback(boost::bind(&LLFloaterPerformance::onAvatarListRightClick, this, _1, _2, _3));

    updateComplexityText();
    mComplexityChangedSignal = gSavedSettings.getControl("RenderAvatarMaxComplexity")->getCommitSignal()->connect(boost::bind(&LLFloaterPerformance::updateComplexityText, this));
    mNearbyPanel->getChild<LLSliderCtrl>("IndirectMaxComplexity")->setCommitCallback(boost::bind(&LLFloaterPerformance::updateMaxComplexity, this));

    updateMaxRenderTime();
    updateMaxRenderTimeText();
    mMaxARTChangedSignal = gSavedSettings.getControl("RenderAvatarMaxART")->getCommitSignal()->connect(boost::bind(&LLFloaterPerformance::updateMaxRenderTimeText, this));
    mNearbyPanel->getChild<LLSliderCtrl>("RenderAvatarMaxART")->setCommitCallback(boost::bind(&LLFloaterPerformance::updateMaxRenderTime, this));

    LLAvatarComplexityControls::setIndirectMaxArc();


    return TRUE;
}

void LLFloaterPerformance::showSelectedPanel(LLPanel* selected_panel)
{
    hidePanels();
    mMainPanel->setVisible(FALSE);
    selected_panel->setVisible(TRUE);

    if (mHUDsPanel == selected_panel)
    {
        populateHUDList();
    }
    else if (mNearbyPanel == selected_panel)
    {
        populateNearbyList();
    }
    else if (mComplexityPanel == selected_panel)
    {
        populateObjectList();
    }
}

void LLFloaterPerformance::draw()
{
    const S32 NUM_PERIODS = 50;
    static LLCachedControl<U32> fps_cap(gSavedSettings, "FramePerSecondLimit"); // user limited FPS
    static LLCachedControl<U32> target_fps(gSavedSettings, "FSTargetFPS"); // desired FPS
    static LLCachedControl<bool> auto_tune(gSavedSettings, "FSAutoTuneFPS"); // auto tune enabled?
    static LLCachedControl<F32> max_render_cost(gSavedSettings, "RenderAvatarMaxART", 0);    

    if (mUpdateTimer->hasExpired())
    {
       	LLStringUtil::format_map_t args;

        auto fps = LLTrace::get_frame_recording().getPeriodMedianPerSec(LLStatViewer::FPS, NUM_PERIODS);
        getChild<LLTextBox>("fps_value")->setValue((S32)llround(fps));
        auto tot_frame_time_ns = 1000000000/fps;
        auto target_frame_time_ns = 1000000000/(target_fps==0?1:target_fps);
        auto tot_avatar_time = FSTelemetry::RecordObjectTime<const LLVOAvatar*>::getSum(FSTelemetry::ObjStatType::RENDER_COMBINED);
        auto tot_huds_time = FSTelemetry::RecordSceneTime::get(FSTelemetry::SceneStatType::RENDER_HUDS) ;
        auto tot_sleep_time = FSTelemetry::RecordSceneTime::get(FSTelemetry::SceneStatType::RENDER_SLEEP);
        auto tot_ui_time = FSTelemetry::RecordSceneTime::get(FSTelemetry::SceneStatType::RENDER_UI);
        auto tot_idle_time = FSTelemetry::RecordSceneTime::get(FSTelemetry::SceneStatType::RENDER_IDLE);
        auto tot_limit_time = FSTelemetry::RecordSceneTime::get(FSTelemetry::SceneStatType::RENDER_FPSLIMIT);
        auto tot_swap_time = FSTelemetry::RecordSceneTime::get(FSTelemetry::SceneStatType::RENDER_SWAP);

        // once the rest is extracted what is left is the scene cost (we don't include non-render activities such as network here prlloy should.)
        auto tot_scene_time = tot_frame_time_ns - tot_avatar_time - tot_huds_time - tot_ui_time - tot_sleep_time - tot_limit_time - tot_swap_time;
        // remove time spent sleeping for fps limit or out of focus.
        tot_frame_time_ns -= tot_limit_time;
        tot_frame_time_ns -= tot_sleep_time;

        if(tot_frame_time_ns == 0)
        {
            LL_WARNS("performance") << "things went wrong, quit while we can." << LL_ENDL;
            return;
        }
        auto pct_avatar_time = (tot_avatar_time*100)/tot_frame_time_ns;
        auto pct_huds_time = (tot_huds_time*100)/tot_frame_time_ns;
        auto pct_ui_time = (tot_ui_time*100)/tot_frame_time_ns;
        auto pct_idle_time = (tot_idle_time*100)/tot_frame_time_ns;
        auto pct_swap_time = (tot_swap_time*100)/tot_frame_time_ns;
        auto pct_scene_time = (tot_scene_time*100)/tot_frame_time_ns;

        args["AV_FRAME_PCT"] = llformat("%02u", (U32)llround(pct_avatar_time));
        args["HUDS_FRAME_PCT"] = llformat("%02u", (U32)llround(pct_huds_time));
        args["UI_FRAME_PCT"] = llformat("%02u", (U32)llround(pct_ui_time));
        args["IDLE_FRAME_PCT"] = llformat("%02u", (U32)llround(pct_idle_time));
        args["SWAP_FRAME_PCT"] = llformat("%02u", (U32)llround(pct_swap_time));
        args["SCENE_FRAME_PCT"] = llformat("%02u", (U32)llround(pct_scene_time));
        args["FPSCAP"] = llformat("%02u", (U32)fps_cap);
        args["FPSTARGET"] = llformat("%02u", (U32)target_fps);

        getChild<LLTextBox>("av_frame_stats")->setText(getString("av_frame_pct", args));
        getChild<LLTextBox>("huds_frame_stats")->setText(getString("huds_frame_pct", args));
        getChild<LLTextBox>("frame_breakdown")->setText(getString("frame_stats", args));
        
        auto textbox = getChild<LLTextBox>("fps_warning");
        if(tot_sleep_time > 0) // We are sleeping because view is not focussed
        {
            textbox->setVisible(true);
            textbox->setText(getString("focus_fps"));
            textbox->setColor(LLUIColorTable::instance().getColor("DrYellow"));
        }
        else if (tot_limit_time > 0)
        {
            textbox->setVisible(true);
            textbox->setText(getString("limit_fps", args));
            textbox->setColor(LLUIColorTable::instance().getColor("DrYellow"));
        }
        else if(auto_tune)
        {
            textbox->setVisible(true);
            textbox->setText(getString("tuning_fps", args));
            textbox->setColor(LLUIColorTable::instance().getColor("green"));
        }
        else
        {
            textbox->setVisible(false);
        }

        if( auto_tune )
        {
            auto av_render_max = FSTelemetry::RecordObjectTime<const LLVOAvatar*>::getMax(FSTelemetry::ObjStatType::RENDER_COMBINED);

            // if( target_frame_time_ns <= tot_frame_time_ns )
            // {
            //     U32 non_avatar_time_ns = tot_frame_time_ns - tot_avatar_time;
            //     if( non_avatar_time_ns < target_frame_time_ns )
            //     {
            //         F32 target_avatar_time_ms {F32(target_frame_time_ns-non_avatar_time_ns)/1000000};
            //         gSavedSettings.setF32( "RenderAvatarMaxART",  target_avatar_time_ms / LLVOAvatar::sMaxNonImpostors );
            //         LL_INFOS() << "AUTO_TUNE: Target frame time:"<<target_frame_time_ns/1000000 << " (non_avatar is " << non_avatar_time_ns/1000000 << ") Avatar budget=" << max_render_cost << " x " << LLVOAvatar::sMaxNonImpostors << LL_ENDL;
            //     }
            // }

            if( target_frame_time_ns <= tot_frame_time_ns )
            {
                LL_INFOS() << "AUTO_TUNE: adapting frame rate" << LL_ENDL;
                U32 non_avatar_time_ns = tot_frame_time_ns - tot_avatar_time;
                LL_INFOS() << "AUTO_TUNE: adapting frame rate: target_frame=" << target_frame_time_ns << " nonav_frame_time=" << non_avatar_time_ns << " headroom=" << target_frame_time_ns - non_avatar_time_ns << LL_ENDL;
                if( non_avatar_time_ns < target_frame_time_ns )
                {
                    U64 target_avatar_time_ns {target_frame_time_ns-non_avatar_time_ns};
                    LL_INFOS() << "AUTO_TUNE: avatar_budget:" << target_avatar_time_ns << LL_ENDL;
                    if(target_avatar_time_ns < tot_avatar_time)
                    {
                        F32 new_render_limit = (F32)(av_render_max-100000)/1000000;
                        if(new_render_limit >= max_render_cost)
                        {
                            // we caught a bad frame possibly with a forced refresh render.
                            new_render_limit = max_render_cost - 0.1;  
                        }
                        gSavedSettings.setF32( "RenderAvatarMaxART",  new_render_limit);
                        LL_INFOS() << "AUTO_TUNE: avatar_budget adjusted to:" << new_render_limit << LL_ENDL;
                    }
                    LL_INFOS() << "AUTO_TUNE: Target frame time:"<<target_frame_time_ns/1000000 << " (non_avatar is " << non_avatar_time_ns/1000000 << ") Max cost limited=" << max_render_cost << LL_ENDL;
                }
            }
            else 
                if( target_frame_time_ns > (tot_frame_time_ns + max_render_cost))
            {
                // if we have more space to spare let's shift up little in the hope we'll restore an avatar.
                gSavedSettings.setF32( "RenderAvatarMaxART",  max_render_cost + 0.5 );
            }
        }
        if (mHUDsPanel->getVisible())
        {
            populateHUDList();
        }
        else if (mNearbyPanel->getVisible())
        {
            populateNearbyList();
            mNearbyPanel->getChild<LLCheckBoxCtrl>("hide_avatars")->set(!LLPipeline::hasRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR));
        }
        else if (mComplexityPanel->getVisible())
        {
            populateObjectList();
        }

        mUpdateTimer->setTimerExpirySec(REFRESH_INTERVAL);
    }
    LLFloater::draw();
}

void LLFloaterPerformance::showMainPanel()
{
    hidePanels();
    mMainPanel->setVisible(TRUE);
}

void LLFloaterPerformance::hidePanels()
{
    mNearbyPanel->setVisible(FALSE);
    mComplexityPanel->setVisible(FALSE);
    mHUDsPanel->setVisible(FALSE);
    mSettingsPanel->setVisible(FALSE);
}

void LLFloaterPerformance::initBackBtn(LLPanel* panel)
{
    panel->getChild<LLButton>("back_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::showMainPanel, this));

    panel->getChild<LLTextBox>("back_lbl")->setShowCursorHand(false);
    panel->getChild<LLTextBox>("back_lbl")->setSoundFlags(LLView::MOUSE_UP);
    panel->getChild<LLTextBox>("back_lbl")->setClickedCallback(boost::bind(&LLFloaterPerformance::showMainPanel, this));
}

void LLFloaterPerformance::populateHUDList()
{
    S32 prev_pos = mHUDList->getScrollPos();
    LLUUID prev_selected_id = mHUDList->getSelectedSpecialId();
    mHUDList->clearRows();
    mHUDList->updateColumns(true);

    hud_complexity_list_t complexity_list = LLHUDRenderNotifier::getInstance()->getHUDComplexityList();

    hud_complexity_list_t::iterator iter = complexity_list.begin();
    hud_complexity_list_t::iterator end = complexity_list.end();

    U32 max_complexity = 0;
    for (; iter != end; ++iter)
    {
        max_complexity = llmax(max_complexity, (*iter).objectsCost);
    }
   
    for (iter = complexity_list.begin(); iter != end; ++iter)
    {
        LLHUDComplexity hud_object_complexity = *iter;        
        S32 obj_cost_short = llmax((S32)hud_object_complexity.objectsCost / 1000, 1);
        LLSD item;
        item["special_id"] = hud_object_complexity.objectId;
        item["target"] = LLNameListCtrl::SPECIAL;
        LLSD& row = item["columns"];
        row[0]["column"] = "complex_visual";
        row[0]["type"] = "bar";
        LLSD& value = row[0]["value"];
        value["ratio"] = (F32)hud_render_time / huds_max_render_time;
        value["bottom"] = BAR_BOTTOM_PAD;
        value["left_pad"] = BAR_LEFT_PAD;
        value["right_pad"] = BAR_RIGHT_PAD;

        row[1]["column"] = "complex_value";
        row[1]["type"] = "text";
        LL_INFOS() << "HUD : hud[" << hud_ptr << " time:" << hud_render_time <<" total_time:" << huds_max_render_time << LL_ENDL;
        row[1]["value"] = llformat("%.2f",((double)hud_render_time / 1000000000));
        row[1]["font"]["name"] = "SANSSERIF";
 
        row[2]["column"] = "name";
        row[2]["type"] = "text";
        row[2]["value"] = hud_object_complexity.objectName;
        row[2]["font"]["name"] = "SANSSERIF";

        LLScrollListItem* obj = mHUDList->addElement(item);
        if (obj)
        {
            LLScrollListText* value_text = dynamic_cast<LLScrollListText*>(obj->getColumn(1));
            if (value_text)
            {
                value_text->setAlignment(LLFontGL::HCENTER);
            }
        }
    }
    mHUDList->sortByColumnIndex(1, FALSE);
    mHUDList->setScrollPos(prev_pos);
    mHUDList->selectItemBySpecialId(prev_selected_id);
}

void LLFloaterPerformance::populateObjectList()
{
    S32 prev_pos = mObjectList->getScrollPos();
    LLUUID prev_selected_id = mObjectList->getSelectedSpecialId();
    mObjectList->clearRows();
    mObjectList->updateColumns(true);

    object_complexity_list_t complexity_list = LLAvatarRenderNotifier::getInstance()->getObjectComplexityList();

    object_complexity_list_t::iterator iter = complexity_list.begin();
    object_complexity_list_t::iterator end = complexity_list.end();

    U32 max_complexity = 0;
    for (; iter != end; ++iter)
    {
        max_complexity = llmax(max_complexity, (*iter).objectCost);
    }

    for (iter = complexity_list.begin(); iter != end; ++iter)
    {
        LLObjectComplexity object_complexity = *iter;        
        S32 obj_cost_short = llmax((S32)object_complexity.objectCost / 1000, 1);
        LLSD item;
        item["special_id"] = object_complexity.objectId;
        item["target"] = LLNameListCtrl::SPECIAL;
        LLSD& row = item["columns"];
        row[0]["column"] = "complex_visual";
        row[0]["type"] = "bar";
        LLSD& value = row[0]["value"];
        value["ratio"] = (F32)obj_cost_short / max_complexity * 1000;
        value["bottom"] = BAR_BOTTOM_PAD;
        value["left_pad"] = BAR_LEFT_PAD;
        value["right_pad"] = BAR_RIGHT_PAD;

        row[1]["column"] = "complex_value";
        row[1]["type"] = "text";
        row[1]["value"] = std::to_string(obj_cost_short);
        row[1]["font"]["name"] = "SANSSERIF";

        row[2]["column"] = "name";
        row[2]["type"] = "text";
        row[2]["value"] = object_complexity.objectName;
        row[2]["font"]["name"] = "SANSSERIF";

        LLScrollListItem* obj = mObjectList->addElement(item);
        if (obj)
        {
            LLScrollListText* value_text = dynamic_cast<LLScrollListText*>(obj->getColumn(1));
            if (value_text)
            {
                value_text->setAlignment(LLFontGL::HCENTER);
            }
        }
    }
    mObjectList->sortByColumnIndex(1, FALSE);
    mObjectList->setScrollPos(prev_pos);
    mObjectList->selectItemBySpecialId(prev_selected_id);
}

void LLFloaterPerformance::populateNearbyList()
{
    S32 prev_pos = mNearbyList->getScrollPos();
    LLUUID prev_selected_id = mNearbyList->getStringUUIDSelectedItem();
    mNearbyList->clearRows();
    mNearbyList->updateColumns(true);

    static LLCachedControl<F32> max_render_cost(gSavedSettings, "RenderAvatarMaxART", 0);
    updateMaxRenderTime();
    updateMaxRenderTimeText();

    std::vector<LLCharacter*> valid_nearby_avs;
    getNearbyAvatars(valid_nearby_avs);

    std::vector<LLCharacter*>::iterator char_iter = valid_nearby_avs.begin();
    auto render_max = FSTelemetry::RecordObjectTime<const LLVOAvatar*>::getMax(FSTelemetry::ObjStatType::RENDER_COMBINED);
    while (char_iter != valid_nearby_avs.end())
    {
        LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*char_iter);
        if (avatar)
        {
            auto overall_appearance = avatar->getOverallAppearance();
            if(overall_appearance == LLVOAvatar::AOA_INVISIBLE)
                continue;

            // S32 complexity_short = llmax((S32)avatar->getVisualComplexity() / 1000, 1);
            auto render_av  = FSTelemetry::RecordObjectTime<const LLVOAvatar*>::get(avatar,FSTelemetry::ObjStatType::RENDER_COMBINED);
            auto is_slow = avatar->isTooSlow(true);
            // auto is_slow_without_shadows = avatar->isTooSlow();

            LLSD item;
            item["id"] = avatar->getID();
            LLSD& row = item["columns"];
            row[0]["column"] = "complex_visual";
            row[0]["type"] = "bar";
            LLSD& value = row[0]["value"];
            value["ratio"] = (double)render_av / render_max;
            value["bottom"] = BAR_BOTTOM_PAD;
            value["left_pad"] = BAR_LEFT_PAD;
            value["right_pad"] = BAR_RIGHT_PAD;

            row[1]["column"] = "complex_value";
            row[1]["type"] = "text";
            if(is_slow)
            {
                row[1]["value"] = llformat("%.2f", ((double)avatar->getLastART() / 1000000));
            }
            else
            {
                row[1]["value"] = llformat("%.2f",((double)render_av / 1000000));
            }

            row[1]["font"]["name"] = "SANSSERIF";
            row[1]["width"] = "50";

            row[2]["column"] = "name";
            row[2]["type"] = "text";
            row[2]["value"] = avatar->getFullname();
            row[2]["font"]["name"] = "SANSSERIF";

            LLScrollListItem* av_item = mNearbyList->addElement(item);
            if(av_item)
            {
                LLScrollListText* value_text = dynamic_cast<LLScrollListText*>(av_item->getColumn(1));
                if (value_text)
                {
                    value_text->setAlignment(LLFontGL::HCENTER);
                }
                LLScrollListText* name_text = dynamic_cast<LLScrollListText*>(av_item->getColumn(2));
                if (name_text)
                {
                    if (avatar->isSelf())
                    {
                        name_text->setColor(LLUIColorTable::instance().getColor("DrYellow"));
                    }
                    else
                    {
                        std::string color = "white";
                        if (is_slow || LLVOAvatar::AOA_JELLYDOLL == overall_appearance)
                        {
                            color = "LabelDisabledColor";
                            LLScrollListBar* bar = dynamic_cast<LLScrollListBar*>(av_item->getColumn(0));
                            if (bar)
                            {
                                bar->setColor(LLUIColorTable::instance().getColor(color));
                            }
                        }
                        else if (LLVOAvatar::AOA_NORMAL == overall_appearance)
                        {
                            color = LLAvatarActions::isFriend(avatar->getID()) ? "ConversationFriendColor" : "white";
                        }
                        name_text->setColor(LLUIColorTable::instance().getColor(color));
                    }
                }
            }
        }
        char_iter++;
    }
    mNearbyList->sortByColumnIndex(1, FALSE);
    mNearbyList->setScrollPos(prev_pos);
    mNearbyList->selectByID(prev_selected_id);
}

void LLFloaterPerformance::getNearbyAvatars(std::vector<LLCharacter*> &valid_nearby_avs)
{
    static LLCachedControl<F32> render_far_clip(gSavedSettings, "RenderFarClip", 64);
    mNearbyMaxComplexity = 0;
    F32 radius = render_far_clip * render_far_clip;
    std::vector<LLCharacter*>::iterator char_iter = LLCharacter::sInstances.begin();
    while (char_iter != LLCharacter::sInstances.end())
    {
        LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*char_iter);
        if (avatar && !avatar->isDead() && !avatar->isControlAvatar())
        {
            if ((dist_vec_squared(avatar->getPositionGlobal(), gAgent.getPositionGlobal()) > radius) &&
                (dist_vec_squared(avatar->getPositionGlobal(), gAgentCamera.getCameraPositionGlobal()) > radius))
            {
                char_iter++;
                continue;
            }
            avatar->calculateUpdateRenderComplexity();
            mNearbyMaxComplexity = llmax(mNearbyMaxComplexity, (S32)avatar->getVisualComplexity());
            valid_nearby_avs.push_back(*char_iter);
        }
        char_iter++;
    }
}

void LLFloaterPerformance::detachItem(const LLUUID& item_id)
{
    LLAppearanceMgr::instance().removeItemFromAvatar(item_id);
}

void LLFloaterPerformance::onClickAdvanced()
{
    LLFloaterPreference* instance = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
    if (instance)
    {
        instance->saveSettings();
    }
    LLFloaterReg::showInstance("prefs_graphics_advanced");
}

void LLFloaterPerformance::onChangeQuality(const LLSD& data)
{
    LLFloaterPreference* instance = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
    if (instance)
    {
        instance->onChangeQuality(data);
    }
}

void LLFloaterPerformance::onClickHideAvatars()
{
    LLPipeline::toggleRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR);
}

void LLFloaterPerformance::onClickExceptions()
{
    // <FS:Ansariel> [FS Persisted Avatar Render Settings]
    //LLFloaterReg::showInstance("avatar_render_settings");
    LLFloaterReg::showInstance("fs_avatar_render_settings");
    // </FS:Ansariel>
}

void LLFloaterPerformance::updateMaxComplexity()
{
    LLAvatarComplexityControls::updateMax(
        mNearbyPanel->getChild<LLSliderCtrl>("IndirectMaxComplexity"),
        mNearbyPanel->getChild<LLTextBox>("IndirectMaxComplexityText"), 
        true);
}

void LLFloaterPerformance::updateMaxRenderTime()
{
    LLAvatarComplexityControls::updateMaxRenderTime(
        mNearbyPanel->getChild<LLSliderCtrl>("RenderAvatarMaxART"),
        mNearbyPanel->getChild<LLTextBox>("RenderAvatarMaxARTText"), 
        true);
}

void LLFloaterPerformance::updateMaxRenderTimeText()
{
    LLAvatarComplexityControls::setRenderTimeText(
        gSavedSettings.getF32("RenderAvatarMaxART"),
        mNearbyPanel->getChild<LLTextBox>("RenderAvatarMaxARTText", true), 
        true);
}

void LLFloaterPerformance::updateComplexityText()
{
    LLAvatarComplexityControls::setText(gSavedSettings.getU32("RenderAvatarMaxComplexity"),
        mNearbyPanel->getChild<LLTextBox>("IndirectMaxComplexityText", true), 
        true);
}

static LLVOAvatar* find_avatar(const LLUUID& id)
{
    LLViewerObject *obj = gObjectList.findObject(id);
    while (obj && obj->isAttachment())
    {
        obj = (LLViewerObject *)obj->getParent();
    }

    if (obj && obj->isAvatar())
    {
        return (LLVOAvatar*)obj;
    }
    else
    {
        return NULL;
    }
}

void LLFloaterPerformance::onCustomAction(const LLSD& userdata, const LLUUID& av_id)
{
    const std::string command_name = userdata.asString();

    LLVOAvatar::VisualMuteSettings new_setting = LLVOAvatar::AV_RENDER_NORMALLY;
    if ("default" == command_name)
    {
        new_setting = LLVOAvatar::AV_RENDER_NORMALLY;
    }
    else if ("never" == command_name)
    {
        new_setting = LLVOAvatar::AV_DO_NOT_RENDER;
    }
    else if ("always" == command_name)
    {
        new_setting = LLVOAvatar::AV_ALWAYS_RENDER;
    }

    LLVOAvatar *avatarp = find_avatar(av_id);
    if (avatarp)
    {
        avatarp->setVisualMuteSettings(new_setting);
    }
    else
    {
        // <FS:Ansariel> [FS Persisted Avatar Render Settings]
        //LLRenderMuteList::getInstance()->saveVisualMuteSetting(av_id, new_setting);
        FSAvatarRenderPersistence::instance().setAvatarRenderSettings(av_id, LLVOAvatar::VisualMuteSettings(new_setting));
        // </FS:Ansariel>
    }
}


bool LLFloaterPerformance::isActionChecked(const LLSD& userdata, const LLUUID& av_id)
{
    const std::string command_name = userdata.asString();

    // <FS:Ansariel> [FS Persisted Avatar Render Settings]
    //S32 visual_setting = LLRenderMuteList::getInstance()->getSavedVisualMuteSetting(av_id);
    S32 visual_setting = (S32)FSAvatarRenderPersistence::instance().getAvatarRenderSettings(av_id);
    // </FS:Ansariel>
    if ("default" == command_name)
    {
        return (visual_setting == LLVOAvatar::AV_RENDER_NORMALLY);
    }
    else if ("non_default" == command_name)
    {
        return (visual_setting != LLVOAvatar::AV_RENDER_NORMALLY);
    }
    else if ("never" == command_name)
    {
        return (visual_setting == LLVOAvatar::AV_DO_NOT_RENDER);
    }
    else if ("always" == command_name)
    {
        return (visual_setting == LLVOAvatar::AV_ALWAYS_RENDER);
    }
    return false;
}

void LLFloaterPerformance::onAvatarListRightClick(LLUICtrl* ctrl, S32 x, S32 y)
{
    LLNameListCtrl* list = dynamic_cast<LLNameListCtrl*>(ctrl);
    if (!list) return;
    list->selectItemAt(x, y, MASK_NONE);
    uuid_vec_t selected_uuids;

    if((list->getCurrentID().notNull()) && (list->getCurrentID() != gAgentID))
    {
        selected_uuids.push_back(list->getCurrentID());
        mContextMenu->show(ctrl, selected_uuids, x, y);
    }
}

// EOF
