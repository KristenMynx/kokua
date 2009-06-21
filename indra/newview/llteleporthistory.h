/** 
 * @file llteleporthistory.h
 * @brief Teleport history
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLTELEPORTHISTORY_H
#define LL_LLTELEPORTHISTORY_H

#include "llsingleton.h" // for LLSingleton

#include <vector>
#include <string>
#include <boost/function.hpp>
#include <boost/signal.hpp>
#include <boost/signals/connection.hpp>


/**
 * An item of the teleport history.
 * 
 * Contains the location's global coordinates and its title.
 */
class LLTeleportHistoryItem
{
public:
	LLTeleportHistoryItem()
	{}

	LLTeleportHistoryItem(std::string title, LLVector3d global_pos)
		: mTitle(title), mGlobalPos(global_pos)
	{}
	
	LLTeleportHistoryItem(const LLSD& val);
	LLSD toLLSD() const;

	std::string	mTitle;		// human-readable location title
	LLVector3d	mGlobalPos; // global position
	LLUUID		mRegionID;	// region ID for getting the region info 
};

/**
 * Teleport history.
 * 
 * Along with the navigation bar "Back" and "Forward" buttons
 * implements web browser-like navigation functionality.
 * 
 * @see LLNavigationBar
 */
class LLTeleportHistory: public LLSingleton<LLTeleportHistory>
{
	LOG_CLASS(LLTeleportHistory);

public:
	
	typedef std::vector<LLTeleportHistoryItem>	slurl_list_t;
	typedef boost::function<void()>				history_callback_t;
	typedef boost::signal  <void()>				history_signal_t;
	
	LLTeleportHistory();
	~LLTeleportHistory();

	/**
	 * Go back in the history.
	 */
	void					goBack() { goToItem(getCurrentItemIndex() - 1); }
	
	/**
	 * Go forward in the history.
	 */
	void					goForward() { goToItem(getCurrentItemIndex() + 1); }
	
	/**
	 * Go to specific item in the history.
	 * 
	 * The item is specified by its index (starting from 0).
	 */ 
	void					goToItem(int idx);
	
	/**
	 * @return history items.
	 */
	const slurl_list_t&		getItems() const { return mItems; }
	
	/**
	 * Is the history empty?
	 * 
	 * History containing single item is treated as empty
	 * because the item points to the current location.
	 */ 
	bool					isEmpty() const { return mItems.size() <= 1; }
	
	/**
	 * Get index of the current location in the history.
	 */
	int						getCurrentItemIndex() const { return mCurrentItem; }
	
	/**
	 * Set a callback to be called upon history changes.
	 * 
	 * Multiple callbacks can be set.
	 */
	boost::signals::connection	setHistoryChangedCallback(history_callback_t cb);
	
	/**
	 * Save history to a file so that we can restore it on startup.
	 * 
	 * @see load()
	 */
	void					dump() const;

private:
	
	/**
	 * Called by when a teleport fails.
	 * 
	 * Called via callback set on the LLViewerParcelMgr "teleport failed" signal.
	 * 
	 * @see mTeleportFailedConn
	 */
	void onTeleportFailed();

	/**
	 * Update current location.
	 * 
	 * Called when a teleport finishes.
	 * Called via callback set on the LLViewerParcelMgr "teleport finished" signal.
	 *
	 * Takes mRequestedItem into consideration: if it's not -1
	 * (i.e. user is teleporting to an arbitrary location, not to a history item)
	 * we purge forward items and append a new one, making it current. Otherwise
	 * we just modify mCurrentItem.
	 * 
	 * @see mRequestedItem
	 * @see mGotInitialUpdate
	 */
	void					updateCurrentLocation();
	
	/**
	 * Invokes the "history changed" callback(s).
	 */
	void					onHistoryChanged();
	
	static std::string		getCurrentLocationTitle();
	
	/**
	 * Actually, the teleport history.
	 */
	slurl_list_t			mItems;
	
	/**
	 * Current position within the history.
	 */
	int						mCurrentItem;
	
	/**
	 * Requested position within the history.
	 * 
	 * When a teleport succeeds, this is checked by updateCurrentLocation() to tell
	 * if this is a teleport within the history (mRequestedItem >=0) or not (-1).
	 * 
	 * Set by goToItem(); reset by onTeleportFailed() (if teleport fails).
	 * 
	 * @see goToItem()
	 * @see updateCurrentLocation()
	 */
	int						mRequestedItem;
	
	/**
	 * Have we received the initial location update?
	 * 
	 * @see updateCurrentLocation()
	 */
	bool					mGotInitialUpdate;
	
	/**
	 * Signal emitted when the history gets changed.
	 * 
	 * Invokes callbacks set with setHistoryChangedCallback().
	 */
	history_signal_t		mHistoryChangedSignal;
	
	/**
	 * Teleport success notification connection.
	 * 
	 * Using this connection we get notified when a teleport finishes
	 * or initial location update occurs.
	 */
	boost::signals::connection	mTeleportFinishedConn;
	
	/**
	 * Teleport failure notification connection.
	 * 
	 * Using this connection we get notified when a teleport fails.
	 */
	boost::signals::connection	mTeleportFailedConn;
};

#endif