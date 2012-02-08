/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is IETab. Modified In Coral IE Tab.
 *
 * The Initial Developer of the Original Code is yuoo2k <yuoo2k@gmail.com>.
 * Modified by quaful <quaful@msn.com>.
 *
 * Portions created by the Initial Developer are Copyright (C) 2006-2008
 * the Initial Developer. All Rights Reserved.
 *
 * ***** END LICENSE BLOCK ***** */
/** @namespace
 */

var IEMode = IEMode || {};

Components.utils.import("resource://gre/modules/Services.jsm");


/** 将URL转换为IE Tab URL */
IEMode.getIeModeURL = function (url) {
  if (IEMode.startsWith(url, IEMode.containerUrl)) return url;
  if (/^file:\/\/.*/.test(url)) try {
    url = decodeURI(url).replace(/\|/g, ":");
  } catch (e) {ERROR(e)}
  return IEMode.containerUrl + encodeURI(url);
}


/** 从IE Tab URL中提取实际访问的URL */
IEMode.getIeTabTrimURL = function (url) {
  if (url && url.length > 0) {
    url = url.replace(/^\s+/g, "").replace(/\s+$/g, "");
    if (/^file:\/\/.*/.test(url)) url = url.replace(/\|/g, ":");
    if (url.substr(0, IEMode.containerUrl.length) == IEMode.containerUrl) {
      url = decodeURI(url.substring(IEMode.containerUrl.length));

      if (!/^[\w]+:/.test(url)) {
        url = "http://" + url;
      }
    }
  }
  return url;
}

/** 获取Firefox页面内嵌的IE Mode Plugin对象 */
IEMode.getIeTabElmt = function (aTab) {
  var aBrowser = (aTab ? aTab.linkedBrowser : gBrowser);
  if (aBrowser && aBrowser.currentURI && IEMode.startsWith(aBrowser.currentURI.spec, IEMode.containerUrl)) {
    if (aBrowser.contentDocument) {
      var obj = aBrowser.contentDocument.getElementById(IEMode.objectID);
      if (obj) {
        return (obj.wrappedJSObject ? obj.wrappedJSObject : obj); // Ref: Safely accessing content DOM from chrome
      }
    }
  }
  return null;
}

/** 获取IE Tab实际访问的URL*/
IEMode.getIeTabElmtURL = function (aTab) {
  var aBrowser = (aTab ? aTab.linkedBrowser : gBrowser);
  var url = IEMode.getIeTabTrimURL(aBrowser.currentURI.spec);
  var iemodeObject = IEMode.getIeTabElmt(aTab);
  if (iemodeObject && iemodeObject.URL && iemodeObject.URL != "") {
    url = (/^file:\/\/.*/.test(url) ? encodeURI(IEMode.convertToUTF8(iemodeObject.URL)) : iemodeObject.URL);
  }
  return url;
}

/** 获取当前Tab的IE Tab URI
 *  与IEMode.getIeTabElmtURL功能相同
 */
IEMode.getCurrentIeTabURI = function (aBrowser) {
  try {
    var docShell = aBrowser.boxObject.QueryInterface(Components.interfaces.nsIBrowserBoxObject).docShell;
    var wNav = docShell.QueryInterface(Components.interfaces.nsIWebNavigation);
    if (wNav.currentURI && IEMode.startsWith(wNav.currentURI.spec, IEMode.containerUrl)) {
      var iemodeObject = wNav.document.getElementById(IEMode.objectID);
      if (iemodeObject) {
        if (iemodeObject.wrappedJSObject) iemodeObject = iemodeObject.wrappedJSObject;
        var url = iemodeObject.URL;
        if (url) {
          const ios = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
          return ios.newURI(IEMode.containerUrl + encodeURI(url), null, null);
        }
      }
    }
  } catch (e) {ERROR(e)}
  return null;
}

/** 是否是IE内核*/
IEMode.isIEEngine = function (aTab) {
  var tab = aTab || gBrowser.mCurrentTab;
  var aBrowser = (aTab ? aTab.linkedBrowser : gBrowser);
  if (aBrowser && aBrowser.currentURI && IEMode.startsWith(aBrowser.currentURI.spec, IEMode.containerUrl)) {
    return true;
  }
  return false;
}

/** 切换某个Tab的内核
 *  通过设置不同的URL实现切换内核的功能。
 *  使用IE内核时，将URL转换为ie tab URL再访问；
 *  使用Firefox内核时，不需转换直接访问。
 */
IEMode.switchTabEngine = function (aTab) {
  if (aTab && aTab.localName == "tab") {
    // 实际浏览的URL
    var url = IEMode.getIeTabElmtURL(aTab);

    // 用于nsIEModeWatchFactory自动切换
    gBrowser.mIEModeSwitchURL = url;

    var webNav = aTab.linkedBrowser;
    // Now it is IE engine, call me means users want to switch to Firefox engine.
    // We have to tell iewatch component that this is manual switching, do not switch back to IE engine
    if (webNav) {
      var node = aTab.linkedBrowser.QueryInterface(Components.interfaces.nsIDOMNode);
      if (node) node.setAttribute(IEMode.browserAttr, true); // Leave a mark for iewatch component				
    }

    var isIEEngineAfterSwitch = !IEMode.isIEEngine(aTab);
    // firefox特有地址只允许使用Firefox内核
    if (isIEEngineAfterSwitch && !IEMode.isFirefoxOnly(url)) {
      // ie tab URL
      url = IEMode.getIeModeURL(url);
    }
    if (aTab.linkedBrowser && aTab.linkedBrowser.currentURI.spec != url) aTab.linkedBrowser.loadURI(url);

    // 用于nsIEModeWatchFactory自动切换
    gBrowser.mIEModeSwitchURL = null;
  }
}

IEMode.setUrlbarSwitchButtonStatus = function (isIEEngine) {
  // Firefox特有页面禁止内核切换
  var url = IEMode.getIeTabElmtURL();
  var urlbtn = document.getElementById("iemode-urlbar-switch");
  if(urlbtn)
    urlbtn.disabled = IEMode.isFirefoxOnly(url);
  var btn = document.getElementById("iemode-button");
  if(btn){
    btn.disabled = IEMode.isFirefoxOnly(url);
    btn.setAttribute("engine", (isIEEngine ? "ie" : "fx"));
  }

  // 更新内核切换按钮图标
  var image = document.getElementById("iemode-urlbar-switch-image");
  if (image) {
    image.setAttribute("engine", (isIEEngine ? "ie" : "fx"));
  }

  // 更新内核切换按钮文字
  var label = document.getElementById("iemode-urlbar-switch-label");
  if (label) {
    var labelId = isIEEngine ? "iemode.urlbar.switch.label.ie" : "iemode.urlbar.switch.label.fx";
    label.value = IEMode.GetLocalizedString(labelId);
  }
  // 更新内核切换按钮tooltip文字
  var tooltips = document.getElementsByAttribute("function", "iemode-switch-tooltip");
  var tooltipId = isIEEngine ? "iemode.urlbar.switch.tooltip2.ie" : "iemode.urlbar.switch.tooltip2.fx";
  for (var i = 0; i < tooltips.length; i++) {
    tooltips[i].value = IEMode.GetLocalizedString(tooltipId);
  }

  var menus = document.getElementsByAttribute("function", "switchIEMode");
  for (var i = 0; i < menus.length; i++) {
    menus[i].setAttribute("checked", (isIEEngine ? "true" : "false"));
  }

}

/** 切换当前页面内核*/
IEMode.switchEngine = function () {
  IEMode.switchTabEngine(gBrowser.mCurrentTab);
}

/** 打开配置对话框 */
IEMode.openPrefDialog = function (url) {
  if (!url) url = IEMode.getIeTabElmtURL();
  var icon = document.getElementById('ietab-status');
  window.openDialog('chrome://iemode/content/iemodeSetting.xul', "iemodePrefDialog", 'chrome,centerscreen', IEMode.getUrlDomain(url), icon);
}

IEMode.openFilterEditor = function () {
  var url = IEMode.getIeTabElmtURL();
  var icon = document.getElementById('ietab-status');
  window.openDialog('chrome://iemode/content/ietabSetting.xul', "ietabPrefDialog", 'chrome,centerscreen', IEMode.getUrlDomain(url), icon, 1);
}

// @todo 什么时候使用？
IEMode.loadIeTab = function (url) {
  gBrowser.loadURI(IEMode.getIeModeURL(url));
}

/** 新建一个ie标签*/
IEMode.addIeTab = function (url) {
  var newTab = gBrowser.addTab(IEMode.getIeModeURL(url));
  gBrowser.selectedTab = newTab;
  if (gURLBar && (url == 'about:blank')) window.setTimeout(function () {
    gURLBar.focus();
  }, 0);
}

IEMode.getHandledURL = function (url, isModeIE) {
  url = IEMode.trim(url);

  // 访问firefox特有地址时, 只允许使用firefox内核
  if (IEMode.isFirefoxOnly(url)) {
    return url;
  }

  if (isModeIE) return IEMode.getIeModeURL(url);

  // @todo 下面的处理有一些问题
  if (IEMode.isIEEngine() && (!IEMode.startsWith(url, "about:")) && (!IEMode.startsWith(url, "view-source:"))) {
    if (IEMode.isValidURL(url) || IEMode.isValidDomainName(url)) {
      var isBlank = (IEMode.getIeTabTrimURL(gBrowser.currentURI.spec) == "about:blank");
      var handleUrlBar = IEMode.getBoolPref("extensions.iemode.handleUrlBar", false);
      var isSimilar = (IEMode.getUrlDomain(IEMode.getIeTabElmtURL()) == IEMode.getUrlDomain(url));
      if (isBlank || handleUrlBar || isSimilar) return IEMode.getIeModeURL(url);
    }
  }

  return url;
}

/** 检查URL地址是否是火狐浏览器特有
 *  例如 about:config chrome://xxx
 */
IEMode.isFirefoxOnly = function (url) {
  return (url && (url.length > 0) && ((IEMode.startsWith(url, 'about:') && url != "about:blank") || IEMode.startsWith(url, 'chrome://')));
}

/** 更新地址栏显示*/
IEMode.updateUrlBar = function () {
  IEMode.setUrlbarSwitchButtonStatus(IEMode.isIEEngine());

  if (!gURLBar || !IEMode.isIEEngine()) return;
  if (gBrowser.userTypedValue) {
    if (gURLBar.selectionEnd != gURLBar.selectionStart) window.setTimeout(function () {
      gURLBar.focus();
    }, 0);
  } else {
    var url = IEMode.getIeTabElmtURL();
    if (url == "about:blank") url = "";
    if (gURLBar.value != url) gURLBar.value = url;
  }

  // 更新收藏状态(星星按钮黄色时表示该页面已收藏)
  PlacesStarButton.updateState();
}

/** 改变页面元素启用状态*/
IEMode.updateObjectDisabledStatus = function (objId, isEnabled) {
  var obj = (typeof (objId) == "object" ? objId : document.getElementById(objId));
  if (obj) {
    var d = obj.hasAttribute("disabled");
    if (d == isEnabled) {
      if (d) obj.removeAttribute("disabled");
      else obj.setAttribute("disabled", true);
    }
  }
}

/** 更新前进、后退铵钮状态*/
IEMode.updateBackForwardButtons = function () {
  try {
    var iemodeObject = IEMode.getIeTabElmt();
    var canBack = (iemodeObject ? iemodeObject.CanBack : false) || gBrowser.webNavigation.canGoBack;
    var canForward = (iemodeObject ? iemodeObject.CanForward : false) || gBrowser.webNavigation.canGoForward;
    IEMode.updateObjectDisabledStatus("Browser:Back", canBack);
    IEMode.updateObjectDisabledStatus("Browser:Forward", canForward);
  } catch (e) {ERROR(e)}
}

/** 更新停止和刷新按钮状态*/
IEMode.updateStopReloadButtons = function () {
  try {
    var iemodeObject = IEMode.getIeTabElmt();
    var isBlank = (gBrowser.currentURI.spec == "about:blank");
    var isLoading = gBrowser.mIsBusy;
    IEMode.updateObjectDisabledStatus("Browser:Reload", iemodeObject ? iemodeObject.CanRefresh : !isBlank);
    IEMode.updateObjectDisabledStatus("Browser:Stop", iemodeObject ? iemodeObject.CanStop : isLoading);
  } catch (e) {ERROR(e)}
}

// 更新编辑菜单中cmd_cut、cmd_copy、cmd_paste状态
IEMode.updateEditMenuItems = function (e) {
  if (e.originalTarget != document.getElementById("menu_EditPopup")) return;
  var iemodeObject = IEMode.getIeTabElmt();
  if (iemodeObject) {
    IEMode.updateObjectDisabledStatus("cmd_cut", iemodeObject.CanCut);
    IEMode.updateObjectDisabledStatus("cmd_copy", iemodeObject.CanCopy);
    IEMode.updateObjectDisabledStatus("cmd_paste", iemodeObject.CanPaste);
  }
}

// @todo 这是哪个按钮？
IEMode.updateSecureLockIcon = function () {
  var iemodeObject = IEMode.getIeTabElmt();
  if (iemodeObject) {
    var securityButton = document.getElementById("security-button");
    if (securityButton) {
      var url = iemodeObject.URL;
      const wpl = Components.interfaces.nsIWebProgressListener;
      var state = (IEMode.startsWith(url, "https://") ? wpl.STATE_IS_SECURE | wpl.STATE_SECURE_HIGH : wpl.STATE_IS_INSECURE);
      window.XULBrowserWindow.onSecurityChange(null, null, state);
      securityButton.setAttribute("label", IEMode.getUrlHost(iemodeObject.URL));
    }
  }
}

/** 更新IeMode界面显示*/
IEMode.updateInterface = function () {
  IEMode.updateBackForwardButtons();
  IEMode.updateStopReloadButtons();
  IEMode.updateSecureLockIcon();
  IEMode.updateUrlBar();
}

/** 更新iemode相关的界面*/
IEMode.updateAll = function () {
  if (IEMode.updating) return;
  try {
    IEMode.updating = true;
    IEMode.updateInterface();
  } finally {
    IEMode.updating = false;
  }
}

IEMode.updateProgressStatus = function () {
  var mTabs = gBrowser.mTabContainer.childNodes;
  for (var i = 0; i < mTabs.length; i++) {
    if (mTabs[i].localName == "tab") {
      var iemodeObject = IEMode.getIeTabElmt(mTabs[i]);
      if (iemodeObject) {
        var aCurTotalProgress = iemodeObject.Progress;
        if (aCurTotalProgress != mTabs[i].mProgress) {
          const ios = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
          const wpl = Components.interfaces.nsIWebProgressListener;
          var aMaxTotalProgress = (aCurTotalProgress == -1 ? -1 : 100);
          var aTabListener = gBrowser.mTabListeners[mTabs[i]._tPos];
          var aWebProgress = mTabs[i].linkedBrowser.webProgress;
          var aRequest = ios.newChannelFromURI(mTabs[i].linkedBrowser.currentURI);
          var aStateFlags = (aCurTotalProgress == -1 ? wpl.STATE_STOP : wpl.STATE_START) | wpl.STATE_IS_NETWORK;
          aTabListener.onStateChange(aWebProgress, aRequest, aStateFlags, 0);
          aTabListener.onProgressChange(aWebProgress, aRequest, 0, 0, aCurTotalProgress, aMaxTotalProgress);
          mTabs[i].mProgress = aCurTotalProgress;
        }
      }
    }
  }
}

/** 响应页面正在加载的消息*/
IEMode.onProgressChange = function (event) {
  var progress = parseInt(event.detail);
  if (progress == 0) gBrowser.userTypedValue = null;
  IEMode.updateProgressStatus();
  IEMode.updateAll();
}

/** 响应新开IE标签的消息*/
IEMode.onNewIETab = function (event) {
  var id = event.detail;
  IEMode.addIeTab("IEMode.onNewIETab/" + id);
}

IEMode.onSecurityChange = function (security) {
  IEMode.updateSecureLockIcon();
}

/** 异步调用IE Mode plugin的方法*/
IEMode.goDoCommand = function (cmd) {
  try {
    var iemodeObject = IEMode.getIeTabElmt();
    if (iemodeObject == null) {
      return false;
    }
    var param = null;
    switch (cmd) {
    case "Back":
      if (!iemodeObject.CanBack) {
        return false;
      }
      break;
    case "Forward":
      if (!iemodeObject.CanForward) {
        return false;
      }
      break;
    }
    window.setTimeout(function () {
      IEMode.delayedGoDoCommand(cmd);
    }, 100);
    return true;
  } catch (e) {ERROR(e)}
  return false;
}

/** 配合IEMode.goDoCommand完成对IE Mode Plugin方法的调用*/
IEMode.delayedGoDoCommand = function (cmd) {
  try {
    var iemodeObject = IEMode.getIeTabElmt();
    switch (cmd) {
    case "Back":
      iemodeObject.Back();
      break;
    case "Forward":
      iemodeObject.Forward();
      break;
    case "Stop":
      iemodeObject.Stop();
      break;
    case "Refresh":
      iemodeObject.Refresh();
      break;
    case "SaveAs":
      iemodeObject.SaveAs();
      break;
    case "Print":
      iemodeObject.Print();
      break;
    case "PrintSetup":
      iemodeObject.PrintSetup();
      break;
    case "PrintPreview":
      iemodeObject.PrintPreview();
      break;
    case "Find":
      iemodeObject.Find();
      break;
    case "cmd_cut":
      iemodeObject.Cut();
      break;
    case "cmd_copy":
      iemodeObject.Copy();
      break;
    case "cmd_paste":
      iemodeObject.Paste();
      break;
    case "cmd_selectAll":
      iemodeObject.SelectAll();
      break;
    case "Focus":
      iemodeObject.Focus();
      break;
    case "HandOverFocus":
      iemodeObject.HandOverFocus();
      break;
    case "Zoom":
      var zoomLevel = IEMode.getZoomLevel();
      iemodeObject.Zoom(zoomLevel);
      break;
    case "DisplaySecurityInfo":
      iemodeObject.DisplaySecurityInfo();
      break;
    }
  } catch (e) {ERROR(e)} finally {
    window.setTimeout(function () {
      IEMode.updateAll();
    }, 0);
  }
}

/** 关闭无用的IE Tab页*/
IEMode.closeIeTab = function () {
  var mTabs = gBrowser.mTabContainer.childNodes;
  for (var i = mTabs.length - 1; i >= 0; i--) {
    if (mTabs[i].localName == "tab") {
      var iemodeObject = IEMode.getIeTabElmt(mTabs[i]);
      if (iemodeObject && (iemodeObject.CanClose)) {
        window.setTimeout(IEMode.closeTab, 500, i);
        break;
      }
    }
  }
}

/** 关闭Tab页
 * @param {number} i Tab页index
 */
IEMode.closeTab = function (i) {
  var mTabs = gBrowser.mTabContainer.childNodes;
  gBrowser.removeTab(mTabs[i]);
}

/** 获取右键菜单关联的Tab对象*/
IEMode.getContextTab = function () {
  return (gBrowser && gBrowser.mContextTab && (gBrowser.mContextTab.localName == "tab") ? gBrowser.mContextTab : null);
}

// 响应内核切换按钮点击事件
IEMode.clickSwitchButton = function (e) {
  // 左键或中键点击切换内核
  if (e.button <= 1 && !e.target.disabled) {
    var aTab = gBrowser.mCurrentTab;
    if (!aTab) return;
    IEMode.switchTabEngine(aTab);
  }

  // 右键点击显示选项菜单
  else if (e.button == 2) {
    document.getElementById("iemode-urlbar-switch-context-menu").openPopup(e.target, "after_start", 0, 0, true, false);
  }

  e.preventDefault();
}

/** 将焦点设置到IE窗口上*/
IEMode.focusIE = function () {
  IEMode.goDoCommand("Focus");
}

IEMode.onTabSelected = function (e) {
  var aTab = e.originalTarget;
  IEMode.updateAll();
  IEMode.focusIE();
}

/** 获取document对应的Tab对象*/
IEMode.getTabByDocument = function (doc) {
  var mTabs = gBrowser.mTabContainer.childNodes;
  for (var i = 0; i < mTabs.length; i++) {
    var tab = mTabs[i];
    if (tab.linkedBrowser.contentDocument == doc) {
      return tab
    }
  }
  return null;
}

/** 加载或显示页面时更新IE Mode界面*/
IEMode.onPageShowOrLoad1 = function (e) {
  IEMode.onPageShowOrLoad(e)
}
IEMode.onPageShowOrLoad2 = function (e) {
  IEMode.onPageShowOrLoad(e)
}
IEMode.onPageShowOrLoad = function (e) {
  IEMode.updateAll();
  IEMode.focusIE();
}

/** 响应界面大小变化事件
 * @todo 为何要Zoom
 */
IEMode.onResize = function (e) {
  IEMode.goDoCommand("Zoom");
}


IEMode.hookBrowserGetter = function (aBrowser) {
  if (aBrowser.localName != "browser") aBrowser = aBrowser.getElementsByTagNameNS(kXULNS, "browser")[0];
  // hook aBrowser.currentURI, 在IE引擎内部打开URL时, Firefox也能获取改变后的URL
  IEMode.hookProp(aBrowser, "currentURI", function () {
    var uri = IEMode.getCurrentIeTabURI(this);
    if (uri) return uri;
  });
  // hook aBrowser.sessionHistory
  // @todo 有什么用？
  IEMode.hookProp(aBrowser, "sessionHistory", function () {
    var history = this.webNavigation.sessionHistory;
    var uri = IEMode.getCurrentIeTabURI(this);
    if (uri) {
      var entry = history.getEntryAtIndex(history.index, false);
      if (entry.URI.spec != uri.spec) {
        entry.QueryInterface(Components.interfaces.nsISHEntry).setURI(uri);
        if (this.parentNode.__SS_data) delete this.parentNode.__SS_data;
      }
    }
  });
}

IEMode.hookURLBarSetter = function (aURLBar) {
  if (!aURLBar) aURLBar = document.getElementById("urlbar");
  if (!aURLBar) return;
  aURLBar.onclick = function (e) {
    var iemodeObject = IEMode.getIeTabElmt();
    if (iemodeObject) {
      IEMode.goDoCommand("HandOverFocus");
    }
  }
  IEMode.hookProp(aURLBar, "value", null, function () {
    this.isModeIE = arguments[0] && (arguments[0].substr(0, IEMode.containerUrl.length) == IEMode.containerUrl);
    if (this.isModeIE) {
      arguments[0] = IEMode.getIeTabTrimURL(arguments[0]);
      // if (arguments[0] == "about:blank") arguments[0] = "";
    }
  });
}

IEMode.checkFilter = function (aBrowser, aRequest, aLocation) {
  var iemodewatch = Components.classes["@mozilla.com.cn/iemodewatch;1"].getService().wrappedJSObject;
  if (iemodewatch && iemodewatch.shouldFilter(aLocation.spec)) {
    aRequest.cancel(0x804b0002); //NS_BINDING_ABORTED
    aBrowser.loadURI(aLocation.spec);
  }
}

IEMode.hookCodeAll = function () {
  //hook properties
  IEMode.hookBrowserGetter(gBrowser.mTabContainer.firstChild.linkedBrowser);
  IEMode.hookURLBarSetter(gURLBar);

  //hook functions
  IEMode.hookCode("gFindBar._onBrowserKeypress", "this._useTypeAheadFind &&", "$& !IEMode.isIEEngine() &&"); // IE内核时不使用Firefox的查找条, $&指代被替换的代码
  IEMode.hookCode("PlacesCommandHook.bookmarkPage", "aBrowser.currentURI", "makeURI(IEMode.getIeTabTrimURL($&.spec))"); // 添加到收藏夹时获取实际URL
  IEMode.hookCode("PlacesStarButton.updateState", /(gBrowser|getBrowser\(\))\.currentURI/g, "makeURI(IEMode.getIeTabTrimURL($&.spec))"); // 用IE内核浏览网站时，在地址栏中正确显示收藏状态(星星按钮黄色时表示该页面已收藏)
  IEMode.hookCode("gBrowser.addTab", "return t;", "IEMode.hookBrowserGetter(t.linkedBrowser); $&");
  IEMode.hookCode("nsBrowserAccess.prototype.openURI", " loadflags = isExternal ?", " loadflags = false ?"); // @todo 有什么用?
  IEMode.hookCode("gBrowser.setTabTitle", "if (browser.currentURI.spec) {", "$& if (browser.currentURI.spec.indexOf(IEMode.containerUrl) == 0) return;"); // 取消原有的Tab标题文字设置
  IEMode.hookCode("URLBarSetURI", "getWebNavigation()", "getBrowser()"); // @todo 有什么用？
  IEMode.hookCode("getShortcutOrURI", /return (\S+);/g, "return IEMode.getHandledURL($1);"); // 访问新的URL
  IEMode.hookCode('gBrowser.mTabProgressListener', "function (aWebProgress, aRequest, aLocation) {", "$& IEMode.checkFilter(this.mBrowser, aRequest, aLocation);"); //@todo 有什么用？
  for (var i = 0; i < gBrowser.mTabListeners.length; i++) //@todo 有什么用？
  IEMode.hookCode("gBrowser.mTabListeners[" + i + "].onLocationChange", /{/, "$& IEMode.checkFilter(this.mBrowser, aRequest, aLocation);");

  //hook Interface Commands
  IEMode.hookCode("BrowserBack", /{/, "$& if(IEMode.goDoCommand('Back')) return;");
  IEMode.hookCode("BrowserForward", /{/, "$& if(IEMode.goDoCommand('Forward')) return;");
  IEMode.hookCode("BrowserStop", /{/, "$& if(IEMode.goDoCommand('Stop')) return;");
  IEMode.hookCode("BrowserReload", /{/, "$& if(IEMode.goDoCommand('Refresh')) return;");
  IEMode.hookCode("BrowserReloadSkipCache", /{/, "$& if(IEMode.goDoCommand('Refresh')) return;");

  IEMode.hookCode("saveDocument", /{/, "$& if(IEMode.goDoCommand('SaveAs')) return;");
  IEMode.hookCode("MailIntegration.sendMessage", /{/, "$& var iemodeObject = IEMode.getIeTabElmt(); if(iemodeObject){ arguments[0]=iemodeObject.URL; arguments[1]=iemodeObject.Title; }"); // @todo 发送邮件？
  IEMode.hookCode("PrintUtils.print", /{/, "$& if(IEMode.goDoCommand('Print')) return;");
  IEMode.hookCode("PrintUtils.showPageSetup", /{/, "$& if(IEMode.goDoCommand('PrintSetup')) return;");
  IEMode.hookCode("PrintUtils.printPreview", /{/, "$& if(IEMode.goDoCommand('PrintPreview')) return;");

  IEMode.hookCode("goDoCommand", /{/, "$& if(IEMode.goDoCommand(arguments[0])) return;"); // cmd_cut, cmd_copy, cmd_paste, cmd_selectAll
  IEMode.hookAttr("cmd_find", "oncommand", "if(IEMode.goDoCommand('Find')) return;");
  IEMode.hookAttr("cmd_findAgain", "oncommand", "if(IEMode.goDoCommand('Find')) return;");
  IEMode.hookAttr("cmd_findPrevious", "oncommand", "if(IEMode.goDoCommand('Find')) return;");

  IEMode.hookCode("displaySecurityInfo", /{/, "$& if(IEMode.goDoCommand('DisplaySecurityInfo')) return;");
}

IEMode.addEventAll = function () {
  IEMode.addEventListener(window, "DOMContentLoaded", IEMode.onPageShowOrLoad1);
  IEMode.addEventListener(window, "pageshow", IEMode.onPageShowOrLoad2);
  IEMode.addEventListener(window, "resize", IEMode.onResize);

  IEMode.addEventListener(gBrowser.tabContainer, "TabSelect", IEMode.onTabSelected);

  IEMode.addEventListener("menu_EditPopup", "popupshowing", IEMode.updateEditMenuItems);

  IEMode.addEventListener(window, "ProgressChanged", IEMode.onProgressChange);
  IEMode.addEventListener(window, "NewIETab", IEMode.onNewIETab);

  // @todo 去除ContainerMessage消息
  IEMode.addEventListener(window, "ContainerMessage", IEMode.onContainerMessage);

  // Bug #23666
  if (IEMode.isVersionOlderThan("4.0b6")) IEMode.addEventListener(gBrowser.tabContainer, "TabOpen", IEMode.onNewTab);
}

// Bug #23666
// 正常情况下，IContentPolicy::shouldLoad 首先被调用，然后才会有 TabOpen 的消息
// 但是如果是从桌面打开一个链接进入到 Firefox，那么是先有 TabOpen 消息，然后才有 shouldLoad
// 所以下面注册的 EventListener 只对 #23666 的情形有效，不影响原来的逻辑
IEMode.onNewTab = function (e) {
  var aTab = e.originalTarget;
  var browser = aTab.linkedBrowser;
  if (browser) {
    var _window = browser.contentWindow;
    if (_window) {
      _window.addEventListener("IeTabWatchCommand", function (e) {
        var wnd = e.originalTarget;
        var url = e.data;
        if (IEMode.isValidURL(url)) {
          wnd.location.href = e.data;
        }
      }, false);
    }
  }
}

IEMode.removeEventAll = function () {
  IEMode.removeEventListener(window, "DOMContentLoaded", IEMode.onPageShowOrLoad1);
  IEMode.removeEventListener(window, "pageshow", IEMode.onPageShowOrLoad2);
  IEMode.removeEventListener(window, "resize", IEMode.onResize);

  IEMode.removeEventListener(gBrowser.tabContainer, "TabSelect", IEMode.onTabSelected);

  IEMode.removeEventListener("menu_EditPopup", "popupshowing", IEMode.updateEditMenuItems);

  IEMode.removeEventListener(window, "ProgressChanged", IEMode.onProgressChange);

  IEMode.removeEventListener(window, "load", IEMode.init);
  IEMode.removeEventListener(window, "unload", IEMode.destroy);

  IEMode.removeEventListener(gBrowser.tabContainer, "TabOpen", IEMode.onNewTab);
}
IEMode.observe = function (aSubject, aTopic, aData){
  IEMode.switchEngine();
}

IEMode.init = function () {
  IEMode.hookCodeAll();
  IEMode.addEventAll();
  IEMode.updateAll();
  Services.obs.addObserver(IEMode, "iemode-switch-engine", false);
  var btn_identity = document.getElementById("identity-box");
  btn_identity && btn_identity.addEventListener("click", IEMode.clickIdentityBox, false)

}

IEMode.destroy = function () {
  IEMode.removeEventAll();
  Services.obs.removeObserver(IEMode, "iemode-switch-engine");
  delete IEMode;
}


IEMode.browse = function (url, feature) {
  var windowMediator = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService(Components.interfaces.nsIWindowMediator);
  if (windowMediator) {
    var w = windowMediator.getMostRecentWindow('navigator:browser');
    if (w && !w.closed) {
      var browser = w.getBrowser();
      var b = (browser.selectedTab = browser.addTab()).linkedBrowser;
      b.stop();
      b.webNavigation.loadURI(url, Components.interfaces.nsIWebNavigation.FLAGS_NONE, null, null, null);
    } else {
      window.open(url, "_blank", features || null)
    }
  }
}

IEMode.onContainerMessage = function (event) {
  var msg = {};
  try {
    msg = JSON.parse(event.detail);
  } catch (e) {ERROR(e)}
  if (msg) {
    switch (msg.message) {
    case "newTab":
      {
        if (IEMode.isValidURL(msg.url)) {
          IEMode.addIeTab(msg.url);
        }
        break;
      }
    case "newWindow":
      {
        var url = msg.url;
        if (IEMode.isValidURL(msg.url)) {
          url = IEMode.getIeModeURL(url, 0);
          var name = msg.name;
          var features = msg.features;
          // 直接用 window.open 打开的窗口会被盖在后面，没办法，只好先把它弄成 topmost 窗口，然后再取消 topmost
          if (features && features.length > 0) {
            // window.open的features参数alwaysRaised表示将窗口置顶 
            features = features.replace(/\s/g, '').replace(/\bstatus=(false|no|0),/, '').replace(/\blocation=(false|no|0),/, '') + ",status=1,location=1";
            var childwin = window.open(url, name, features);
            if (childwin) {
              childwin.focus();
            }
          } else {
            window.open(url, name);
          }
        }
        break;
      }
    case "onProgressChange":
      {
        IEMode.onProgressChange(msg.progress);
        break;
      }
    case "onSecurityChange":
      {
        IEMode.onSecurityChange(msg.security);
        break;
      }
    case "closeIeTab":
      {
        IEMode.closeIeTab();
        break;
      }
    }
  }
}

// identity-box事件
IEMode.clickIdentityBox = function (e) {
  if (e.button == 0) {
    var location = gBrowser.contentWindow.location;

    if (location.href.indexOf(IEMode.containerUrl) == 0) {
      IEMode.notify();
    }
  }/* else if (e.button == 2) {
    var btn_identity = document.getElementById("identity-box");
    var menu = document.getElementById("iemode-urlbar-switch-context-menu");
    btn_identity && menu && menu.openPopup(btn_identity, "after_start", -1, 0, true, false);
  }*/

  e.preventDefault();
}

IEMode.notify = function () {
  var btn_identity = document.getElementById("identity-box");
  var panel = document.getElementById("iemode-identity-popup");
  panel.openPopup(btn_identity, "after_start", 15, 0, true, false);
}
IEMode.hideNotify = function () {
  var panel = document.getElementById("iemode-identity-popup");
  panel.hidePopup();
}

window.addEventListener("load", IEMode.init, false);
window.addEventListener("unload", IEMode.destroy, false);
IEMode.browserAttr = "iemodeManualSwitch";
IEMode.engineAttr = "iemodeEngine";