/** 
 * @file llchathistory.cpp
 * @brief LLTextEditor base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llchathistory.h"
#include "llpanel.h"
#include "lluictrlfactory.h"
#include "llscrollcontainer.h"
#include "llavatariconctrl.h"
#include "llimview.h"
#include "llcallingcard.h" //for LLAvatarTracker
#include "llagentdata.h"
#include "llavataractions.h"
#include "lltrans.h"
#include "llfloaterreg.h"
#include "llmutelist.h"

#include "llsidetray.h"//for blocked objects panel

static LLDefaultChildRegistry::Register<LLChatHistory> r("chat_history");

class LLChatHistoryHeader: public LLPanel
{
public:
	static LLChatHistoryHeader* createInstance(const std::string& file_name)
	{
		LLChatHistoryHeader* pInstance = new LLChatHistoryHeader;
		LLUICtrlFactory::getInstance()->buildPanel(pInstance, file_name);	
		return pInstance;
	}

	BOOL handleMouseUp(S32 x, S32 y, MASK mask)
	{
		return LLPanel::handleMouseUp(x,y,mask);
	}

	void onObjectIconContextMenuItemClicked(const LLSD& userdata)
	{
		std::string level = userdata.asString();

		if (level == "profile")
		{
			LLSD params;
			params["object_id"] = getAvatarId();

			LLFloaterReg::showInstance("inspect_object", params);
		}
		else if (level == "block")
		{
			LLMuteList::getInstance()->add(LLMute(getAvatarId(), mFrom, LLMute::OBJECT));

			LLSideTray::getInstance()->showPanel("panel_block_list_sidetray", LLSD().insert("blocked_to_select", getAvatarId()));
		}
	}

	void onAvatarIconContextMenuItemClicked(const LLSD& userdata)
	{
		std::string level = userdata.asString();

		if (level == "profile")
		{
			LLAvatarActions::showProfile(getAvatarId());
		}
		else if (level == "im")
		{
			LLAvatarActions::startIM(getAvatarId());
		}
		else if (level == "add")
		{
			std::string name;
			name.assign(getFirstName());
			name.append(" ");
			name.append(getLastName());

			LLAvatarActions::requestFriendshipDialog(getAvatarId(), name);
		}
		else if (level == "remove")
		{
			LLAvatarActions::removeFriendDialog(getAvatarId());
		}
	}

	BOOL postBuild()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;

		registrar.add("AvatarIcon.Action", boost::bind(&LLChatHistoryHeader::onAvatarIconContextMenuItemClicked, this, _2));
		registrar.add("ObjectIcon.Action", boost::bind(&LLChatHistoryHeader::onObjectIconContextMenuItemClicked, this, _2));

		LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_avatar_icon.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
		mPopupMenuHandleAvatar = menu->getHandle();

		menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_object_icon.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
		mPopupMenuHandleObject = menu->getHandle();

		setDoubleClickCallback(boost::bind(&LLChatHistoryHeader::onHeaderPanelClick, this, _2, _3, _4));

		return LLPanel::postBuild();
	}

	bool pointInChild(const std::string& name,S32 x,S32 y)
	{
		LLUICtrl* child = findChild<LLUICtrl>(name);
		if(!child)
			return false;
		
		LLView* parent = child->getParent();
		if(parent!=this)
		{
			x-=parent->getRect().mLeft;
			y-=parent->getRect().mBottom;
		}

		S32 local_x = x - child->getRect().mLeft ;
		S32 local_y = y - child->getRect().mBottom ;
		return 	child->pointInView(local_x, local_y);
	}

	BOOL handleRightMouseDown(S32 x, S32 y, MASK mask)
	{
		if(pointInChild("avatar_icon",x,y) || pointInChild("user_name",x,y))
		{
			showContextMenu(x,y);
			return TRUE;
		}

		return LLPanel::handleRightMouseDown(x,y,mask);
	}

	void onHeaderPanelClick(S32 x, S32 y, MASK mask)
	{
		if (mSourceType == CHAT_SOURCE_OBJECT)
		{
			LLFloaterReg::showInstance("inspect_object", LLSD().insert("object_id", mAvatarID));
		}
		else if (mSourceType == CHAT_SOURCE_AGENT)
		{
			LLFloaterReg::showInstance("inspect_avatar", LLSD().insert("avatar_id", mAvatarID));
		}
		//if chat source is system, you may add "else" here to define behaviour.
	}

	const LLUUID&		getAvatarId () const { return mAvatarID;}
	const std::string&	getFirstName() const { return mFirstName; }
	const std::string&	getLastName	() const { return mLastName; }

	void setup(const LLChat& chat,const LLStyle::Params& style_params) 
	{
		mAvatarID = chat.mFromID;
		mSourceType = chat.mSourceType;
		gCacheName->get(mAvatarID, FALSE, boost::bind(&LLChatHistoryHeader::nameUpdatedCallback, this, _1, _2, _3, _4));
		if(chat.mFromID.isNull())
		{
			mSourceType = CHAT_SOURCE_SYSTEM;
		}

		LLTextEditor* userName = getChild<LLTextEditor>("user_name");

		userName->setReadOnlyColor(style_params.readonly_color());
		userName->setColor(style_params.color());
		
		if(!chat.mFromName.empty())
		{
			userName->setValue(chat.mFromName);
			mFrom = chat.mFromName;
		}
		else
		{
			std::string SL = LLTrans::getString("SECOND_LIFE");
			userName->setValue(SL);
		}

		setTimeField(chat.mTimeStr);
		
		LLAvatarIconCtrl* icon = getChild<LLAvatarIconCtrl>("avatar_icon");

		if(mSourceType != CHAT_SOURCE_AGENT)
			icon->setDrawTooltip(false);

		if(!chat.mFromID.isNull())
		{
			icon->setValue(chat.mFromID);
		}

	} 

	void nameUpdatedCallback(const LLUUID& id,const std::string& first,const std::string& last,BOOL is_group)
	{
		if (id != mAvatarID)
			return;
		mFirstName = first;
		mLastName = last;
	}
protected:
	void showContextMenu(S32 x,S32 y)
	{
		if(mSourceType == CHAT_SOURCE_SYSTEM)
			showSystemContextMenu(x,y);
		if(mSourceType == CHAT_SOURCE_AGENT)
			showAvatarContextMenu(x,y);
		if(mSourceType == CHAT_SOURCE_OBJECT)
			showObjectContextMenu(x,y);
	}

	void showSystemContextMenu(S32 x,S32 y)
	{
	}
	
	void showObjectContextMenu(S32 x,S32 y)
	{
		LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandleObject.get();
		if(menu)
			LLMenuGL::showPopup(this, menu, x, y);
	}
	
	void showAvatarContextMenu(S32 x,S32 y)
	{
		LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandleAvatar.get();

		if(menu)
		{
			bool is_friend = LLAvatarTracker::instance().getBuddyInfo(mAvatarID) != NULL;
			
			menu->setItemEnabled("Add Friend", !is_friend);
			menu->setItemEnabled("Remove Friend", is_friend);

			if(gAgentID == mAvatarID)
			{
				menu->setItemEnabled("Add Friend", false);
				menu->setItemEnabled("Send IM", false);
				menu->setItemEnabled("Remove Friend", false);
			}

			menu->buildDrawLabels();
			menu->updateParent(LLMenuGL::sMenuContainer);
			LLMenuGL::showPopup(this, menu, x, y);
		}
	}

private:
	void setTimeField(const std::string& time_value)
	{
		LLTextBox* time_box = getChild<LLTextBox>("time_box");

		LLRect rect_before = time_box->getRect();
		time_box->setValue(time_value);

		// set necessary textbox width to fit all text
		time_box->reshapeToFitText();
		LLRect rect_after = time_box->getRect();

		// move rect to the left to correct position...
		S32 delta_pos_x = rect_before.getWidth() - rect_after.getWidth();
		S32 delta_pos_y = rect_before.getHeight() - rect_after.getHeight();
		time_box->translate(delta_pos_x, delta_pos_y);

		//... & change width of the name control
		LLUICtrl* user_name = getChild<LLUICtrl>("user_name");
		const LLRect& user_rect = user_name->getRect();
		user_name->reshape(user_rect.getWidth() + delta_pos_x, user_rect.getHeight());
	}

protected:
	LLHandle<LLView>	mPopupMenuHandleAvatar;
	LLHandle<LLView>	mPopupMenuHandleObject;

	LLUUID			    mAvatarID;
	EChatSourceType		mSourceType;
	std::string			mFirstName;
	std::string			mLastName;
	std::string			mFrom;

};


LLChatHistory::LLChatHistory(const LLChatHistory::Params& p)
: LLTextEditor(p),
mMessageHeaderFilename(p.message_header),
mMessageSeparatorFilename(p.message_separator),
mLeftTextPad(p.left_text_pad),
mRightTextPad(p.right_text_pad),
mLeftWidgetPad(p.left_widget_pad),
mRightWidgetPad(p.right_widget_pad),
mTopSeparatorPad(p.top_separator_pad),
mBottomSeparatorPad(p.bottom_separator_pad),
mTopHeaderPad(p.top_header_pad),
mBottomHeaderPad(p.bottom_header_pad)
{
}

LLChatHistory::~LLChatHistory()
{
	this->clear();
}

/*void LLChatHistory::updateTextRect()
{
	static LLUICachedControl<S32> texteditor_border ("UITextEditorBorder", 0);

	LLRect old_text_rect = mTextRect;
	mTextRect = mScroller->getContentWindowRect();
	mTextRect.stretch(-texteditor_border);
	mTextRect.mLeft += mLeftTextPad;
	mTextRect.mRight -= mRightTextPad;
	if (mTextRect != old_text_rect)
	{
		needsReflow();
	}
}*/

LLView* LLChatHistory::getSeparator()
{
	LLPanel* separator = LLUICtrlFactory::getInstance()->createFromFile<LLPanel>(mMessageSeparatorFilename, NULL, LLPanel::child_registry_t::instance());
	return separator;
}

LLView* LLChatHistory::getHeader(const LLChat& chat,const LLStyle::Params& style_params)
{
	LLChatHistoryHeader* header = LLChatHistoryHeader::createInstance(mMessageHeaderFilename);
	header->setup(chat,style_params);
	return header;
}

void LLChatHistory::clear()
{
	mLastFromName.clear();
	LLTextEditor::clear();
}

void LLChatHistory::appendMessage(const LLChat& chat, const bool use_plain_text_chat_history, const LLStyle::Params& input_append_params)
{
	LLColor4 txt_color = LLUIColorTable::instance().getColor("White");
	LLViewerChat::getChatColor(chat,txt_color);
	LLFontGL* fontp = LLViewerChat::getChatFont();	
	std::string font_name = LLFontGL::nameFromFont(fontp);
	std::string font_size = LLFontGL::sizeFromFont(fontp);	
	LLStyle::Params style_params;
	style_params.color(txt_color);
	style_params.readonly_color(txt_color);
	style_params.font.name(font_name);
	style_params.font.size(font_size);	
	style_params.font.style(input_append_params.font.style);
	
	std::string header_text = "[" + chat.mTimeStr + "] ";
	if (utf8str_trim(chat.mFromName).size() != 0 && chat.mFromName != SYSTEM_FROM)
		header_text += chat.mFromName + ": ";
	
	if (use_plain_text_chat_history)
	{
		appendText(header_text, getText().size() != 0, style_params);
	}
	else
	{
		LLView* view = NULL;
		LLInlineViewSegment::Params p;
		p.force_newline = true;
		p.left_pad = mLeftWidgetPad;
		p.right_pad = mRightWidgetPad;

		if (mLastFromName == chat.mFromName)
		{
			view = getSeparator();
			p.top_pad = mTopSeparatorPad;
			p.bottom_pad = mBottomSeparatorPad;
		}
		else
		{
			view = getHeader(chat, style_params);
			if (getText().size() == 0)
				p.top_pad = 0;
			else
				p.top_pad = mTopHeaderPad;
			p.bottom_pad = mBottomHeaderPad;
			
		}
		p.view = view;

		//Prepare the rect for the view
		LLRect target_rect = getDocumentView()->getRect();
		// squeeze down the widget by subtracting padding off left and right
		target_rect.mLeft += mLeftWidgetPad + mHPad;
		target_rect.mRight -= mRightWidgetPad;
		view->reshape(target_rect.getWidth(), view->getRect().getHeight());
		view->setOrigin(target_rect.mLeft, view->getRect().mBottom);

		appendWidget(p, header_text, false);
		mLastFromName = chat.mFromName;
	}
	//Handle IRC styled /me messages.
	std::string prefix = chat.mText.substr(0, 4);
	if (prefix == "/me " || prefix == "/me'")
	{
		style_params.font.style = "ITALIC";

		if (chat.mFromName.size() > 0)
			appendText(chat.mFromName + " ", TRUE, style_params);
		appendText(chat.mText.substr(4), FALSE, style_params);
	}
	else
		appendText(chat.mText, FALSE, style_params);
	blockUndo();
}

