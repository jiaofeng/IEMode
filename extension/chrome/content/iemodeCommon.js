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
["LOG", "WARN", "ERROR"].forEach(function(aName) {
  if(this[aName])
    return
  this.__defineGetter__(aName, function() {
    Components.utils.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("IEMode.UI", this);
    return this[aName];
  });
}, this);

var IEMode = IEMode || {};

IEMode.containerUrl = "chrome://iemode/content/container.html?url=";
IEMode.objectID = "iemode-object";

IEMode.GetLocalizedString = function (name) {
  var s = "";
  try {
    var stringService = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var strings = stringService.createBundle("chrome://iemode/locale/global.properties");
    s = strings.GetStringFromName(name);
  } catch (e) {ERROR(e)}
  return s;
};

IEMode.isValidURL = function (url) {
  var b = false;
  try {
    const ios = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
    var uri = ios.newURI(url, null, null);
    b = true;
  } catch (e) {ERROR(e)}
  return b;
}

IEMode.isValidDomainName = function (domainName) {
  return /^[0-9a-zA-Z]+[0-9a-zA-Z\.\_\-]*\.[0-9a-zA-Z\_\-]+$/.test(domainName);
}

IEMode.mlog = function (text) {
  Components.utils.reportError("[IE Mode] " + text);
}

IEMode.getRegistryEntry = function (regRoot, regPath, regName) {
  var result = null;
  try {
    if ("@mozilla.org/windows-registry-key;1" in Components.classes) {
      var nsIWindowsRegKey = Components.classes["@mozilla.org/windows-registry-key;1"].getService(Components.interfaces.nsIWindowsRegKey);
      var regRootKey = new Array(0x80000000, 0x80000005, 0x80000001, 0x80000002, 0x80000003);
      nsIWindowsRegKey.open(regRootKey[regRoot], regPath, Components.interfaces.nsIWindowsRegKey.ACCESS_READ);
      if (nsIWindowsRegKey.valueCount) {
        result = nsIWindowsRegKey.readStringValue(regName);
      }
      nsIWindowsRegKey.close();
    }
  } catch (e) {ERROR(e)}

  return result;
}

IEMode.getChromeWindow = function () {
  return QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIWebNavigation).QueryInterface(Components.interfaces.nsIDocShellTreeItem).rootTreeItem.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIDOMWindow);
}

IEMode.getZoomLevel = function () {
  var aBrowser = (typeof (gBrowser) == "undefined") ? IEMode.getChromeWindow().gBrowser : gBrowser;
  var fullZoom = IEMode.getBoolPref("browser.zoom.full", false);
  var docViewer = aBrowser.selectedBrowser.markupDocumentViewer;
  var zoomLevel = fullZoom ? docViewer.fullZoom : docViewer.textZoom;
  return zoomLevel;
}

//-----------------------------------------------------------------------------
IEMode.addEventListener = function (obj, type, listener) {
  if (typeof (obj) == "string") obj = document.getElementById(obj);
  if (obj) obj.addEventListener(type, listener, false);
}
IEMode.removeEventListener = function (obj, type, listener) {
  if (typeof (obj) == "string") obj = document.getElementById(obj);
  if (obj) obj.removeEventListener(type, listener, false);
}

IEMode.addEventListenerByTagName = function (tag, type, listener) {
  var objs = document.getElementsByTagName(tag);
  for (var i = 0; i < objs.length; i++) {
    objs[i].addEventListener(type, listener, false);
  }
}
IEMode.removeEventListenerByTagName = function (tag, type, listener) {
  var objs = document.getElementsByTagName(tag);
  for (var i = 0; i < objs.length; i++) {
    objs[i].removeEventListener(type, listener, false);
  }
}

//-----------------------------------------------------------------------------
/** 替换函数部分源码 */
IEMode.hookCode = function (orgFunc, orgCode, myCode) {
  try {
    if (eval(orgFunc).toString() == eval(orgFunc + "=" + eval(orgFunc).toString().replace(orgCode, myCode))) throw orgFunc;
  } catch (e) {
    ERROR("Failed to hook function: " + orgFunc);
  }
}

/** 将attribute值V替换为myFunc+V*/
IEMode.hookAttr = function (parentNode, attrName, myFunc) {
  if (typeof (parentNode) == "string") parentNode = document.getElementById(parentNode);
  try {
    parentNode.setAttribute(attrName, myFunc + parentNode.getAttribute(attrName));
  } catch (e) {
    ERROR("Failed to hook attribute: " + attrName);
  }
}

/** 在Property的getter和setter代码头部增加一段代码*/
IEMode.hookProp = function (parentNode, propName, myGetter, mySetter) {
  var oGetter = parentNode.__lookupGetter__(propName);
  var oSetter = parentNode.__lookupSetter__(propName);
  if (oGetter && myGetter) myGetter = oGetter.toString().replace(/{/, "{" + myGetter.toString().replace(/^.*{/, "").replace(/.*}$/, ""));
  if (oSetter && mySetter) mySetter = oSetter.toString().replace(/{/, "{" + mySetter.toString().replace(/^.*{/, "").replace(/.*}$/, ""));
  if (!myGetter) myGetter = oGetter;
  if (!mySetter) mySetter = oSetter;
  if (myGetter) try {
    eval('parentNode.__defineGetter__(propName, ' + myGetter.toString() + ');');
  } catch (e) {
    ERROR("Failed to hook property Getter: " + propName);
  }
  if (mySetter) try {
    eval('parentNode.__defineSetter__(propName, ' + mySetter.toString() + ');');
  } catch (e) {
    ERROR("Failed to hook property Setter: " + propName);
  }
}

//-----------------------------------------------------------------------------
IEMode.trim = function (s) {
  if (s) return s.replace(/^\s+/g, "").replace(/\s+$/g, "");
  else return "";
}

IEMode.startsWith = function (s, prefix) {
  if (s) return ((s.substring(0, prefix.length) == prefix));
  else return false;
}

IEMode.endsWith = function (s, suffix) {
  if (s && (s.length > suffix.length)) {
    return (s.substring(s.length - suffix.length) == suffix);
  } else return false;
}

//-----------------------------------------------------------------------------
IEMode.getBoolPref = function (prefName, defval) {
  var result = defval;
  var prefs = Services.prefs.getBranch("");
  if (prefs.getPrefType(prefName) == prefs.PREF_BOOL) {
    try {
      result = prefs.getBoolPref(prefName);
    } catch (e) {ERROR(e)}
  }
  return (result);
}

IEMode.getIntPref = function (prefName, defval) {
  var result = defval;
  var prefs = Services.prefs.getBranch("");
  if (prefs.getPrefType(prefName) == prefs.PREF_INT) {
    try {
      result = prefs.getIntPref(prefName);
    } catch (e) {ERROR(e)}
  }
  return (result);
}

IEMode.getStrPref = function (prefName, defval) {
  var result = defval;
  var prefs = Services.prefs.getBranch("");
  if (prefs.getPrefType(prefName) == prefs.PREF_STRING) {
    try {
      result = prefs.getComplexValue(prefName, Components.interfaces.nsISupportsString).data;
    } catch (e) {ERROR(e)}
  }
  return (result);
}

IEMode.getDefaultStrPref = function (prefName, defval) {
  var result = defval;
  var defaults = Services.prefs.getDefaultBranch("");
  if (defaults.getPrefType(prefName) == defaults.PREF_STRING) {
    try {
      result = defaults.getCharPref(prefName);
    } catch (e) {ERROR(e)}
  }
  return (result);
}
//-----------------------------------------------------------------------------
IEMode.setBoolPref = function (prefName, value) {
  var prefs = Services.prefs.getBranch("");
  try {
    prefs.setBoolPref(prefName, value);
  } catch (e) {ERROR(e)}
}

IEMode.setIntPref = function (prefName, value) {
  var prefs = Services.prefs.getBranch("");
  try {
    prefs.setIntPref(prefName, value);
  } catch (e) {ERROR(e)}
}

IEMode.setStrPref = function (prefName, value) {
  var prefs = Services.prefs.getBranch("");
  var sString = Components.classes["@mozilla.org/supports-string;1"].createInstance(Components.interfaces.nsISupportsString);
  sString.data = value;
  try {
    prefs.setComplexValue(prefName, Components.interfaces.nsISupportsString, sString);
  } catch (e) {ERROR(e)}
}

//-----------------------------------------------------------------------------
IEMode.getDefaultCharset = function (defval) {
  var charset = this.getStrPref("extensions.iemode.intl.charset.default", "");
  if (charset.length) return charset;
  var gPrefs = Components.classes['@mozilla.org/preferences-service;1'].getService(Components.interfaces.nsIPrefBranch);
  if (gPrefs.prefHasUserValue("intl.charset.default")) {
    return gPrefs.getCharPref("intl.charset.default");
  } else {
    var strBundle = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var intlMess = strBundle.createBundle("chrome://global-platform/locale/intl.properties");
    try {
      return intlMess.GetStringFromName("intl.charset.default");
    } catch (e) {
      {WARN(e)}
      return defval;
    }
  }
}

IEMode.queryDirectoryService = function (aPropName) {
  try {
    var dirService = Components.classes["@mozilla.org/file/directory_service;1"].getService(Components.interfaces.nsIProperties);
    var file = dirService.get(aPropName, Components.interfaces.nsIFile);
    return file.path;
  } catch (e) {ERROR(e)}

  return null;
}

IEMode.convertToUTF8 = function (data, charset) {
  try {
    data = decodeURI(data);
  } catch (e) {
    WARN("convertToUTF8 faild");
    if (!charset) charset = IEMode.getDefaultCharset();
    if (charset) {
      var uc = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"].createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
      try {
        uc.charset = charset;
        data = uc.ConvertToUnicode(unescape(data));
        data = decodeURI(data);
      } catch (e) {ERROR(e)}
      uc.Finish();
    }
  }
  return data;
}

IEMode.convertToASCII = function (data, charset) {
  if (!charset) charset = IEMode.getDefaultCharset();
  if (charset) {
    var uc = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"].createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
    uc.charset = charset;
    try {
      data = uc.ConvertFromUnicode(data);
    } catch (e) {
      WARN("ConvertFromUnicode faild");
      data = uc.ConvertToUnicode(unescape(data));
      data = decodeURI(data);
      data = uc.ConvertFromUnicode(data);
    }
    uc.Finish();
  }
  return data;
}

//-----------------------------------------------------------------------------
IEMode.getUrlDomain = function (url) {
  var r = "";
  if (url && !IEMode.startsWith(url, "about:")) {
    if (/^file:\/\/.*/.test(url)) r = url;
    else {
      try {
        const ios = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
        var uri = ios.newURI(url, null, null);
        uri.path = "";
        r = uri.spec;
      } catch (e) {ERROR(e)}
    }
  }
  return r;
}

IEMode.getUrlHost = function (url) {
  if (url && !IEMode.startsWith(url, "about:")) {
    if (/^file:\/\/.*/.test(url)) return url;
    var matches = url.match(/^([A-Za-z]+:\/+)*([^\:^\/]+):?(\d*)(\/.*)*/);
    if (matches) url = matches[2];
  }
  return url;
}

//-----------------------------------------------------------------------------
IEMode.isVersionOlderThan = function (v) {
  var appInfo = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo);
  var versionChecker = Cc["@mozilla.org/xpcom/version-comparator;1"].getService(Ci.nsIVersionComparator);
  return (versionChecker.compare(appInfo.version, v) >= 0);
}
