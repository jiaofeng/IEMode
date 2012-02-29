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
 
 /**
 * @namespace
 */
const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components; 
var IEModeContainer = IEModeContainer || {};

Cu.import("resource://gre/modules/Services.jsm");

["LOG", "WARN", "ERROR"].forEach(function(aName) {
  if(this[aName])
    return
  this.__defineGetter__(aName, function() {
    Cu.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("IEMode container", this);
    return this[aName];
  });
}, this);

function init() 
{
  navigator.plugins.refresh(false);
  var c = document.getElementById("container");
  while (c.hasChildNodes())
    c.removeChild(c.firstChild);
  
  if (isWindows()) {
    if (checkOSVersion()) {
      if (isInPrivateBrowsingMode()) {
        $("#container").append('<iframe src="PrivateBrowsingWarning.xhtml" width="100%" height="100%" frameborder="no" border="0" marginwidth="0" marginheight="0" scrolling="no" allowtransparency="yes"></iframe>');
      } else {
        var ts = '<embed id="iemode-object" type="application/iemode" style="width:100%;height:100%;"></embed>';

        setTimeout(function() {
          $("#container").append(ts);
        }, 100);
        registerEventHandler();
      }
    } else {
      $("#container").append('<center><p>The current version of IE Mode can only work on Microsoft Windows XP SP2 or later.</p><center>');
    }
  } else {
    $("#container").append('<p align="center"> IE Mode can only work on 32bit Microsoft Windows.</p>');
  }
}

function isWindows() {
	return navigator.platform.substring(0,3) == 'Win';	// Win32, Win64
}

/**
 * IE Mode no longer works on Windows 2000, Windows XP RTM and Windows XP SP1. So this function will return false
 * if the host OS's version is not Windows XP SP2 or later.
 */
function checkOSVersion() {
	var b = false;
	
	var m = /Windows NT (\d+\.\d+)/.exec(navigator.oscpu);
	if ( m ) {
		var ver = m[1];
		if ( ver >= "6.0" ) return true;		// 6.0+: Vista, Win7 or later, no problem
		if ( ver < "5.1" ) return false;		// 5.1: XP, can not work if is earlier than that
		
		// 5.1: XP, should be SP2+
		// 5.2: Windows Server 2003, should be SP1+
		var regRoot = 3;	// HKEY_LOCAL_MACHINE
		var regPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
		var regName = "CSDVersion";
		var servicePackStr = IEMode.getRegistryEntry(regRoot, regPath, regName);
		if ( servicePackStr && servicePackStr.length > 3 ) {
			var i = 1;
			while ( ((servicePackStr[i] < '0') || (servicePackStr[i] > '9')) && ( i < servicePackStr.length ) ) {
				i++;
			}
    	var ServicePackLevel = parseInt(servicePackStr[i]);
    	switch (ver) {
    		case "5.1":
    			b = ServicePackLevel >= "2";
    			break;
    		case "5.2":
    			b = ServicePackLevel >= "1";
    			break;
    		default:
    			b = ServicePackLevel >= "2";		// Should not come here, but as the last resort, set a default condition
    			break;
    	}
    }
	}
	
	return b;
}

function isInPrivateBrowsingMode() {
	var pbs;
	try { pbs = Components.classes["@mozilla.org/privatebrowsing;1"].getService(Components.interfaces.nsIPrivateBrowsingService); } catch (e) {}
	var privatebrowsingwarning = pbs && pbs.privateBrowsingEnabled && IEMode.getBoolPref("extensions.iemode.privatebrowsingwarning", true);
	
	if ( privatebrowsingwarning ) {
		var cookieService = Components.classes["@mozilla.org/cookieService;1"].getService(Components.interfaces.nsICookieService);
		var ioService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
		var cookieManager = Components.classes["@mozilla.org/cookiemanager;1"].getService(Components.interfaces.nsICookieManager);
		try {
			var pbwFlag = cookieService.getCookieString(ioService.newURI("http://iemode/", null, null), null);
			if (pbwFlag) {
				privatebrowsingwarning = pbwFlag.indexOf("privatebrowsingwarning=no") < 0;
				cookieManager.remove("iemode", "privatebrowsingwarning", "/", false);
			}
		}
		catch (e) {ERROR(e)}
	}
	
	return privatebrowsingwarning;
}

/** This listener is only to repeat messages to XUL chrome */
function IETabNotifyListener(event) {
	var evt = document.createEvent("MessageEvent");
	evt.initMessageEvent("ContainerMessage", true, true, event.data, document.location.href, 0, window);
	document.dispatchEvent(evt);
}

function PluginNotFoundListener(event) {
	alert("Loading plugin failed. Please try restarting Firefox.");
}

/** 响应Plugin标题变化事件 */
IEModeContainer.onTitleChanged = function(event) {
	var title = event.detail;
	document.title = title;
}

/** 响应关闭IE标签窗口事件 */
IEModeContainer.onCloseIETab = function(event) {
	window.close();
}

function registerEventHandler() {
	window.addEventListener("IETabNotify", IETabNotifyListener, false);
	window.addEventListener("PluginNotFound", PluginNotFoundListener, false);
	window.addEventListener("TitleChanged", IEModeContainer.onTitleChanged, false);
	window.addEventListener("CloseIETab", IEModeContainer.onCloseIETab, false);
	
	$(document).focus(function() {
		var iemodeObject = document.getElementById("iemode-object");
		if (iemodeObject) {
			iemodeObject.Focus();
		}
	});
}


