/**
 * Appcelerator Titanium - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2008 Appcelerator, Inc. All Rights Reserved.
 */
#ifndef TI_WINDOW_CONFIG_H_
#define TI_WINDOW_CONFIG_H_

#include <string>

#include "app_api.h"
#include "app_config.h" 
namespace ti {

class TITANIUM_APP_API WindowConfig
{
	private:
	std::string winid;
	std::string url;
	std::string urlRegex;
	std::string title;

	int x;
	int y;
	int width;
	int height;
	int minWidth;
	int minHeight;
	int maxWidth;
	int maxHeight;

	float transparency;

	bool visible;
	bool maximizable;
	bool minimizable;
	bool closeable;
	bool resizable;
	bool fullscreen;
	bool maximized;
	bool minimized;
	bool usingChrome;
	bool usingScrollbars;
	bool topMost;
	bool taskbarTab;
#ifdef OS_OSX
	bool texturedBackground;
#endif

	void SetDefaults();

	public:
	static int DEFAULT_POSITION;
	static int windowCount;
	static std::string blankPageURL;

	WindowConfig() { SetDefaults(); }
	WindowConfig(void* data);
	WindowConfig(WindowConfig *config, std::string& url);
	void UseProperties(SharedKObject properties);

	std::string ToString();

	// window accessors
	std::string& GetURL() { return url; }
	void SetURL(std::string& url_) { url = url_; }
	std::string& GetURLRegex() { return urlRegex; }
	void SetURLRegex(std::string& urlRegex_) { urlRegex = urlRegex_; }
	std::string& GetTitle() { return title; }
	void SetTitle(std::string& title_) { title = title_; }
	std::string& GetID() { return winid; }
	void SetID(std::string id_) { winid = id_; }

	int GetX() { return x; }
	void SetX(int x_) { x = x_; }
	int GetY() { return y; }
	void SetY(int y_) { y = y_; }
	int GetWidth() { return width; }
	void SetWidth(int width_) { width = width_; }
	int GetHeight() { return height; }
	void SetHeight(int height_) { height = height_; }
	int GetMinWidth() { return minWidth; }
	void SetMinWidth(int minWidth_) { minWidth = minWidth_; }
	int GetMinHeight() { return minHeight; }
	void SetMinHeight(int minHeight_) { minHeight = minHeight_; }
	int GetMaxWidth() { return maxWidth; }
	void SetMaxWidth(int maxWidth_) { maxWidth = maxWidth_; }
	int GetMaxHeight() { return maxHeight; }
	void SetMaxHeight(int maxHeight_) { maxHeight = maxHeight_; }

	float GetTransparency() { return transparency; }
	void SetTransparency(float transparency_) { transparency = transparency_; }
	bool IsVisible() { return visible; }
	void SetVisible(bool visible_) { visible = visible_; }
	bool IsMaximizable() { return maximizable; }
	void SetMaximizable(bool maximizable_) { maximizable = maximizable_; }
	bool IsMinimizable() { return minimizable; }
	void SetMinimizable(bool minimizable_) { minimizable = minimizable_; }
	bool IsCloseable() { return closeable; }
	void SetCloseable(bool closeable_) { closeable = closeable_; }
	bool IsResizable() { return resizable; }
	void SetResizable(bool resizable_) { resizable = resizable_; }
	bool IsFullscreen() { return fullscreen; }
	void SetFullscreen(bool fullscreen_) { fullscreen = fullscreen_; }
	bool IsMaximized() { return maximized; }
	void SetMaximized(bool maximized_) { maximized = maximized_; }
	bool IsMinimized() { return minimized; }
	void SetMinimized(bool minimized_) { minimized = minimized_; }
	bool IsUsingChrome() { return usingChrome; }
	void SetUsingChrome(bool usingChrome_) { usingChrome = usingChrome_; }
	bool IsUsingScrollbars() { return usingScrollbars; }
	void SetUsingScrollbars(bool usingScrollbars_) { usingScrollbars = usingScrollbars_; }
	bool IsTopMost() { return topMost; }
	void SetTopMost(bool topmost_) { topMost = topmost_; }
<<<<<<< HEAD:modules/ti.App/window_config.h
	bool IsTaskbarTab() { return taskbarTab; }
	void SetTaskbarTab(bool taskbarTab_) { taskbarTab = taskbarTab_; }
	
=======

// probably long term a better way of doing this, but we need a quick way to disable these
#ifdef OS_OSX
	void SetTexturedBackground(bool texturedBackground_) { texturedBackground = texturedBackground_; }
	bool IsTexturedBackground() { return texturedBackground; }
#endif

>>>>>>> 0ce4e9d8570843eca5b150ed8fdc5989037ae7b1:modules/ti.App/window_config.h
};

}
#endif
