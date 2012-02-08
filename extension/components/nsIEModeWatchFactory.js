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
const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components; 

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const _IEMODEWATCH_CID = Components.ID('{4A5F2348-6943-4d85-A652-A7F32B68259B}');
const _IEMODEWATCH_CONTRACTID = "@mozilla.com.cn/iemodewatch;1";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/AddonManager.jsm");


["LOG", "WARN", "ERROR"].forEach(function(aName) {
  if(this[aName])
    return
  this.__defineGetter__(aName, function() {
    Cu.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("IEMode", this);
    return this[aName];
  });
}, this);

const IEModeContainerUrl = "chrome://iemode/content/container.html?url=";


function getFileFromURLSpec(spec) {
      var fph = Services.io.getProtocolHandler("file").QueryInterface(Ci.nsIFileProtocolHandler);
      return fph.getFileFromURLSpec(spec).QueryInterface(Ci.nsILocalFile);
}
function getBoolPref(prefName, defval) {
  var result = defval;
  var prefs = Services.prefs.getBranch("");
  if (prefs.getPrefType(prefName) == prefs.PREF_BOOL) {
    try {
      result = prefs.getBoolPref(prefName); 
    }catch(e) {ERROR(e)}
  }
  return(result);
}

function setIntPref(prefName, value) {
  var prefs = Services.prefs.getBranch("");
  try {
    prefs.setIntPref(prefName, value);
  } catch (e) {ERROR(e)}
}

function getIntPref(prefName, defval) {
  var result = defval;
  var prefs = Services.prefs.getBranch("");
  if (prefs.getPrefType(prefName) == prefs.PREF_INT) {
    try {
      result = prefs.getIntPref(prefName);
    } catch (e) {ERROR(e)}
  }
  return (result);
}

function setStrPref (prefName, value) {
  var prefs = Services.prefs.getBranch("");
  var sString = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
  sString.data = value;
  try {
    prefs.setComplexValue(prefName, Ci.nsISupportsString, sString);
  } catch (e) {ERROR(e)}
};

function getStrPref (prefName, defval) {
  var result = defval;
  var prefs = Services.prefs.getBranch("");
  if (prefs.getPrefType(prefName) == prefs.PREF_STRING) {
    try {
      result = prefs.getComplexValue(prefName, Ci.nsISupportsString).data;
    } catch (e) {ERROR(e)}
  }
  return (result);
};

function httpGet (url, onreadystatechange) {
	var xmlHttpRequest = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);
  xmlHttpRequest.QueryInterface(Ci.nsIJSXMLHttpRequest);
	xmlHttpRequest.open('GET', url, true);
	xmlHttpRequest.send(null);
	xmlHttpRequest.onreadystatechange = function() {
		onreadystatechange(xmlHttpRequest);
	};
};

function updateFilter(){
	var allwaysUpdate = getBoolPref("extensions.iemode.official.allwaysupdate", true);
	var last_update = getIntPref("extensions.iemode.official.lastupdatetime", 0);
  var oneDay = 24*60*60;  //24 Hours
  var now = Date.now()/1000;
	if (!allwaysUpdate && (now - last_update) < oneDay) {//24 Hours
		return; 
	}
	var updateUrl = getStrPref("extensions.iemode.official.updateurl", null);
	if(!updateUrl)
	  return;
	httpGet(updateUrl, function(response) {
		if (response.readyState == 4 && 200 == response.status) {
			var filter = response.responseText;
      LOG("update filter : OK");
			if (filter) {
        setStrPref("extensions.iemode.official.filterlist", filter);
				setIntPref("extensions.iemode.official.lastupdatetime", now);
			}
		}
	});  
	var hwindow = Cc["@mozilla.org/appshell/appShellService;1"]
  			   .getService(Ci.nsIAppShellService)
  			   .hiddenDOMWindow;
	hwindow.setTimeout(updateFilter, oneDay*1000);
 
}

var IEModeWatcher = {
   isIEModeURL: function(url) {
      if (!url)
        return false;
      return (url.indexOf(IEModeContainerUrl) == 0);
   },

   getIEModeURL: function(url) {
      if (this.isIEModeURL(url))
        return url;
      if (/^file:\/\/.*/.test(url))
        try {
          url = decodeURI(url).substring(8).replace(/\//g, "\\"); 
        }catch(e) {ERROR(e)}
      return IEModeContainerUrl + encodeURI(url);
   },

   isFilterEnabled: function() {
      return (getBoolPref("extensions.iemode.filter", true));
   },

   isOfficialFilterEnabled: function() {
      return (getBoolPref("extensions.iemode.official.filter", true));
   },

   getAllPrefFilterList: function() {  // add official filter list
      var s = "";
      if(this.isFilterEnabled())
        s =  getStrPref("extensions.iemode.filterlist", "") + " ";
      if(this.isOfficialFilterEnabled())
        s += getStrPref("extensions.iemode.official.filterlist", ""); 
      return ((s == "") ? [] : s.split(" "));
   },

   getPrefOfficialFilterList: function() {  // add official filter list
      var s = "";
      if(this.isOfficialFilterEnabled())
        s = getStrPref("extensions.iemode.official.filterlist", ""); 
      return ((s == "") ? [] : s.split(" "));
   },

   getPrefFilterList: function() {  
      var s = "";
      if(this.isFilterEnabled())
        s =  getStrPref("extensions.iemode.filterlist", "");
      return ((s == "") ? [] : s.split(" "));
   },

   setPrefOfficialFilterList: function(list) {
      setStrPref("extensions.iemode.official.filterlist", list.join(" "));
   },

   setPrefFilterList: function(list) {
      setStrPref("extensions.iemode.filterlist", list.join(" "));
   },

   isMatchURL: function(url, pattern) {
      if ((!pattern) || (pattern.length==0))
        return false;
      var retest = /^\/(.*)\/$/.exec(pattern);
      if (retest) {
         pattern = retest[1];
      } else {
         pattern = pattern.replace(/\\/g, "/");
         var m = pattern.match(/^(.+:\/\/+[^\/]+\/)?(.*)/);
         m[1] = (m[1] ? m[1].replace(/\./g, "\\.").replace(/\?/g, "[^\\/]?").replace(/\*/g, "[^\\/]*") : "");
         m[2] = (m[2] ? m[2].replace(/\./g, "\\.").replace(/\+/g, "\\+").replace(/\?/g, "\\?").replace(/\*/g, ".*") : "");
         pattern = m[1] + m[2];
         pattern = "^" + pattern.replace(/\/$/, "\/.*") + "$";
      }
      var reg = new RegExp(pattern.toLowerCase());
      return (reg.test(url.toLowerCase()));
   },

   isMatchFilterList: function(url) {
      var aList = this.getAllPrefFilterList();
      for (var i=0; i<aList.length; i++) {
         var item = aList[i].split("\b");
         var rule = item[0];
         var enabled = (item.length == 1);
         if (enabled && this.isMatchURL(url, rule))
           return(true);
      }
      return(false);
   },
   

   getTopWinBrowser: function() {
      try {
         var winMgr = Cc['@mozilla.org/appshell/window-mediator;1'].getService();
         var topWin = winMgr.QueryInterface(Ci.nsIWindowMediator).getMostRecentWindow("navigator:browser");
         var mBrowser = topWin.document.getElementById("content");
         return mBrowser;
      } catch(e) {ERROR(e)}
      return null;
   },

   autoSwitchFilter: function(url) {
      if (url == "about:blank")
        return;
      var mBrowser = this.getTopWinBrowser();
      if (!(mBrowser && mBrowser.mIEModeSwitchURL))
        return;
      if (mBrowser.mIEModeSwitchURL == url) {
         var isIE = this.isIEModeURL(url);
         if (isIE)
           url = decodeURI(url.substring(IEModeContainerUrl.length));

         var isMatched = false;
         var aList = this.getPrefFilterList();
         for (var i=0; i<aList.length; i++) {
            var item = aList[i].split("\b");
            var rule = item[0];
            if (this.isMatchURL(url, rule)) {
               aList[i] = rule + (isIE ? "" : "\b");
               isMatched = true;
            }
         }
         if (isMatched)
           this.setPrefFilterList(aList);

         isMatched = false;
         var aList = this.getPrefOfficialFilterList();
         for (var i=0; i<aList.length; i++) {
            var item = aList[i].split("\b");
            var rule = item[0];
            if (this.isMatchURL(url, rule)) {
               aList[i] = rule + (isIE ? "" : "\b");
               isMatched = true;
            }
         }
         if (isMatched)
           this.setPrefOfficialFilterList(aList);
      }
      mBrowser.mIEModeSwitchURL = null;
   }
}

// ContentPolicy class
var IEModeWatchFactoryClass = function() {
  this.wrappedJSObject = this;
}

IEModeWatchFactoryClass.prototype = {
  // this must match whatever is in chrome.manifest!
  classID: _IEMODEWATCH_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIContentPolicy]),
  
  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
    case "profile-after-change":
    	var _interval = 1000 * 5;
			var hwindow = Cc["@mozilla.org/appshell/appShellService;1"]
						   .getService(Ci.nsIAppShellService)
						   .hiddenDOMWindow;

    	hwindow.setTimeout(updateFilter, _interval);
/*///os       getRule();
      AddonManager.getAddonByID("iemode@mozilla.com.cn", function(addon) {
        var uri = addon.getResourceURI();
        var file = getFileFromURLSpec(uri.spec);
        Services.prefs.setComplexValue("extensions.iemode.xpiDir", Ci.nsILocalFile,file);
      });
      */
      break;
    };
  },

  shouldFilter: function(url) {
    return !IEModeWatcher.isIEModeURL(url)
//         && IEModeWatcher.isFilterEnabled()
         && IEModeWatcher.isMatchFilterList(url);
  },

  checkQueueReload: function(contentType, contentLocation, requestOrigin, requestingNode, mimeTypeGuess, extra) {
	// In Firefox 4, when we are called from the content handler, we aren't able to
	// load a chrome:// URL.  If we try again to filter it from a different context, then
	// it will succeed, so we queue those up for a reload.
	var stack = Components.stack;
	while(stack)
	{
		if(stack.filename && stack.filename.indexOf("nsBrowserContentHandler") != -1)
		{
			var browser = requestingNode;
			var me = this;
			var newURI = IEModeWatcher.getIEModeURL(contentLocation.spec);
			var hwindow = Cc["@mozilla.org/appshell/appShellService;1"]
						   .getService(Ci.nsIAppShellService)
						   .hiddenDOMWindow;

			hwindow.setTimeout(function() {
				var wm = Cc["@mozilla.org/appshell/window-mediator;1"]
								   .getService(Ci.nsIWindowMediator);
				var browserWindow = wm.getMostRecentWindow("navigator:browser");
				browserWindow.loadURI(newURI);
			}, 1000);
			break;
		}
		stack = stack.caller;
	}
  },
  // nsIContentPolicy interface implementation
  shouldLoad: function(contentType, contentLocation, requestOrigin, requestingNode, mimeTypeGuess, extra) {
    if (contentType == Ci.nsIContentPolicy.TYPE_DOCUMENT) {
      IEModeWatcher.autoSwitchFilter(contentLocation.spec);
      // check IEMode FilterList
      if (this.shouldFilter(contentLocation.spec)) {
		    this.checkQueueReload(contentType, contentLocation, requestOrigin, requestingNode, mimeTypeGuess, extra);
        contentLocation.spec = IEModeWatcher.getIEModeURL(contentLocation.spec);
      }
    }
    return (Ci.nsIContentPolicy.ACCEPT);
  },
  // this is now for urls that directly load media, and meta-refreshes (before activation)
  shouldProcess: function(contentType, contentLocation, requestOrigin, requestingNode, mimeType, extra) {
    return (Ci.nsIContentPolicy.ACCEPT);
  },
  get wrappedJSObject() {
    return this;
  }
}

var NSGetFactory = XPCOMUtils.generateNSGetFactory([IEModeWatchFactoryClass]);
