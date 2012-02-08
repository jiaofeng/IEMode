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
var Ci = Components.interfaces;
var Cc = Components.classes;
var Cu = Components.utils;

Cu.import("resource://gre/modules/AddonManager.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var IEMode = IEMode ||{};

IEMode.exportSettings = function () {
  var aOld = IEMode._getAllSettings(false);
  IEMode.setOptions(true);
  var aCurrent = IEMode._getAllSettings(false);
  if (aCurrent) IEMode._saveToFile(aCurrent);
  IEMode._setAllSettings(aOld);
}

IEMode.importSettings = function () {
  var aOld = IEMode._getAllSettings(false);
  var [result, aList] = IEMode._loadFromFile();
  if (result) {
    if (aList) {
      IEMode._setAllSettings(aList);
      IEMode.initDialog();
      IEMode._setAllSettings(aOld);
      IEMode.updateApplyButton(true);
    } else {
//      alert(IEMode.GetLocalizedString("iemode.settings.import.error"));
    }
  }
}

IEMode.restoreDefaultSettings = function () {
  var aOld = IEMode._getAllSettings(false);
  var aDefault = IEMode._getAllSettings(true);
  IEMode._setAllSettings(aDefault);
  IEMode.initDialog();
  IEMode._setAllSettings(aOld);
  IEMode.updateApplyButton(true);
}

// 应用设置
IEMode.setOptions = function (quiet) {
  var requiresRestart = false;

  //filter
  var filter = document.getElementById('filtercbx').checked;
  IEMode.setBoolPref("extensions.iemode.filter", filter);
  IEMode.setStrPref("extensions.iemode.filterlist", IEMode.getFilterListString());

  //official filter
  var filter = document.getElementById('filtercbx-official').checked;
  IEMode.setBoolPref("extensions.iemode.official.filter", filter);
  var update = document.getElementById('autoUpdateFilter').checked;
  IEMode.setBoolPref("extensions.iemode.official.filter.update", update);
  IEMode.setStrPref("extensions.iemode.official.filterlist", IEMode.getOfficialFilterListString());

  //general
/*  IEMode.setBoolPref("extensions.iemode.handleUrlBar", document.getElementById('handleurl').checked);
  var runInProcess = document.getElementById('runinprocess').checked;
  if (runInProcess != IEMode.getBoolPref("extensions.iemode.runinprocess")) {
    requiresRestart = true;
    IEMode.setBoolPref("extensions.iemode.runinprocess", runInProcess);
    IEMode.setBoolPref("dom.ipc.plugins.enabled.npiemode.dll", !runInProcess);
  }
*/
  //update UI
  IEMode.updateApplyButton(false);

  // Deal with compatibility mode
  var newMode = "ie7mode";
  var item = document.getElementById("iemode").selectedItem;
  if (item) newMode = item.getAttribute("id");
  if (IEMode.getStrPref("extensions.iemode.compatMode") != newMode) {
    requiresRestart = true;
    IEMode.setStrPref("extensions.iemode.compatMode", newMode);
    IEMode.updateIECompatMode();
  }

  //notify of restart requirement
  if (requiresRestart && !quiet) {
    alert(IEMode.GetLocalizedString("iemode.settings.alert.restart"));
  }
}

IEMode.getPrefOfficialFilterList = function (def) {
  var s = "";
  if (def) 
    s = IEMode.getDefaultStrPref("extensions.iemode.official.filterlist", null);
  else 
    s = IEMode.getStrPref("extensions.iemode.official.filterlist", null);
  return (s ? s.split(" ") : []);
}

IEMode.getPrefFilterList = function (def) {
  var s = "";
  if (def) 
    s = IEMode.getDefaultStrPref("extensions.iemode.filterlist", null);
  else 
    s = IEMode.getStrPref("extensions.iemode.filterlist", null);
  return (s ? s.split(" ") : []);
}

IEMode.addFilterRule = function (rule, enabled) {
  var idx = IEMode.findRule(rule);
  var rules = document.getElementById('filterChilds');
  if (idx == -1) {
    var item = document.createElement('treeitem');
    var row = document.createElement('treerow');
    var c1 = document.createElement('treecell');
    var c2 = document.createElement('treecell');
    c1.setAttribute('label', rule);
    c2.setAttribute('value', enabled);
    row.appendChild(c1);
    row.appendChild(c2);
    item.appendChild(row);
    item.setEnabled = function(e){
      c2.setAttribute('value', e);
    }
    rules.appendChild(item);
    return (rules.childNodes.length - 1);
  }else{
    rules.childNodes[idx].setEnabled(enabled);
    return idx;
  }
}

IEMode.addOfficialFilterRule = function (rule, enabled) {
  var rules = document.getElementById('filterChilds-official');
  var item = document.createElement('treeitem');
  var row = document.createElement('treerow');
  var c1 = document.createElement('treecell');
  var c2 = document.createElement('treecell');
  c1.setAttribute('label', rule);
  c2.setAttribute('value', enabled);
  row.appendChild(c1);
  row.appendChild(c2);
  item.appendChild(row);
  rules.appendChild(item);
  return (rules.childNodes.length - 1);
}
IEMode.initFilterList = function (def) {
  var list = IEMode.getPrefFilterList(def);
  var rules = document.getElementById('filterChilds');
  while (rules.hasChildNodes())
    rules.removeChild(rules.firstChild);
  for (var i = 0; i < list.length; i++) {
    if (list[i] != "") {
      var item = list[i].split("\b");
      var rule = item[0];
      if (!/^\/(.*)\/$/.exec(rule)) rule = rule.replace(/\/$/, "/*");
      var enabled = (item.length == 1);
      IEMode.addFilterRule(rule, enabled);
    }
  }
}

IEMode.initOfficialFilterList = function (def) {
  var list = IEMode.getPrefOfficialFilterList(def);
  var rules = document.getElementById('filterChilds-official');
  while (rules.hasChildNodes())
    rules.removeChild(rules.firstChild);
  for (var i = 0; i < list.length; i++) {
    if (list[i] != "") {
      var item = list[i].split("\b");
      var rule = item[0];
      if (!/^\/(.*)\/$/.exec(rule)) rule = rule.replace(/\/$/, "/*");
      var enabled = (item.length == 1);
      IEMode.addOfficialFilterRule(rule, enabled);
    }
  }
}

IEMode.initDialog = function () {
  //filter tab 网址过滤
  document.getElementById('filtercbx').checked = IEMode.getBoolPref("extensions.iemode.filter", true);
  //
  IEMode.initFilterList(false);
  // add current tab's url 
  var newurl = (window.arguments ? window.arguments[0] : ""); //get CurrentTab's URL
  document.getElementById('urlbox').value = (IEMode.startsWith(newurl, "about:") ? "" : newurl);
  document.getElementById('urlbox').select();

  //official filter tab 网址过滤
  document.getElementById('filtercbx-official').checked = IEMode.getBoolPref("extensions.iemode.official.filter", true);
  //official filter自动更新
  document.getElementById('autoUpdateFilter').checked = IEMode.getBoolPref("extensions.iemode.official.filter.update", true);
  //
  IEMode.initOfficialFilterList(false);


  //general 功能设置
//  document.getElementById('handleurl').checked = IEMode.getBoolPref("extensions.iemode.handleUrlBar", false);
//  document.getElementById('runinprocess').checked = IEMode.getBoolPref("extensions.iemode.runinprocess", false);

  //updateStatus
  IEMode.updateFilterStatus();
  IEMode.updateOfficialFilterStatus();
  IEMode.updateApplyButton(false);

  //compatibility mode IE兼容模式
  IEMode.updateIECompatUI();
  var mode = IEMode.getStrPref("extensions.iemode.compatMode");
  document.getElementById("iemode").selectedItem = document.getElementById(mode);
}

IEMode.updateIECompatUI = function () {
  var wrk = Cc["@mozilla.org/windows-registry-key;1"].createInstance(Ci.nsIWindowsRegKey);
  wrk.create(wrk.ROOT_KEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Internet Explorer", wrk.ACCESS_READ);

  var value = "";
  try{
    value = wrk.readStringValue("version");
  }catch(e) {ERROR(e)}
  value = value.split('.')[0];
  switch(value){
    case "9":
      document.getElementById("ie9mode").hidden = false;
    case "8":
      document.getElementById("ie8mode").hidden = false;
      document.getElementById("ie7mode").hidden = false;
      break;
    default:
      document.getElementById("iecompat").hidden = false;
  }
}

IEMode.updateApplyButton = function (e) {
  document.getElementById("myApply").disabled = !e;
}

IEMode.init = function () {
  IEMode.initDialog();
  IEMode.addEventListenerByTagName("checkbox", "command", IEMode.updateApplyButton);
  IEMode.addEventListenerByTagName("radio", "command", IEMode.updateApplyButton);
  IEMode.addEventListener("filterChilds", "DOMAttrModified", IEMode.updateApplyButton);
  IEMode.addEventListener("filterChilds", "DOMNodeInserted", IEMode.updateApplyButton);
  IEMode.addEventListener("filterChilds", "DOMNodeRemoved", IEMode.updateApplyButton);
  IEMode.addEventListener("filterChilds-official", "DOMAttrModified", IEMode.updateApplyButton);
  IEMode.addEventListener("filterChilds-official", "DOMNodeInserted", IEMode.updateApplyButton);
  IEMode.addEventListener("filterChilds-official", "DOMNodeRemoved", IEMode.updateApplyButton);
  IEMode.addEventListener("parambox", "input", IEMode.updateApplyButton);
}

IEMode.destory = function () {
  IEMode.removeEventListenerByTagName("checkbox", "command", IEMode.updateApplyButton);
  IEMode.removeEventListenerByTagName("radio", "command", IEMode.updateApplyButton);
  IEMode.removeEventListener("filterChilds", "DOMAttrModified", IEMode.updateApplyButton);
  IEMode.removeEventListener("filterChilds", "DOMNodeInserted", IEMode.updateApplyButton);
  IEMode.removeEventListener("filterChilds", "DOMNodeRemoved", IEMode.updateApplyButton);
  IEMode.removeEventListener("filterChilds-official", "DOMAttrModified", IEMode.updateApplyButton);
  IEMode.removeEventListener("filterChilds-official", "DOMNodeInserted", IEMode.updateApplyButton);
  IEMode.removeEventListener("filterChilds-official", "DOMNodeRemoved", IEMode.updateApplyButton);
  IEMode.removeEventListener("parambox", "input", IEMode.updateApplyButton);
}

IEMode.updateFilterStatus = function () {
  var en = document.getElementById('filtercbx').checked;
  document.getElementById('filterList').disabled = (!en);
  document.getElementById('filterList').editable = (en);
  document.getElementById('urllabel').disabled = (!en);
  document.getElementById('urlbox').disabled = (!en);
  IEMode.updateAddButtonStatus();
  IEMode.updateDelButtonStatus();
}

IEMode.updateOfficialFilterStatus = function () {
  var en = document.getElementById('filtercbx-official').checked;
  document.getElementById('filterList-official').disabled = (!en);
  document.getElementById('filterList-official').editable = (en);
}

IEMode.updateIECompatMode = function () {
  var mode = IEMode.getStrPref("extensions.iemode.compatMode");
  var wrk = Cc["@mozilla.org/windows-registry-key;1"].createInstance(Ci.nsIWindowsRegKey);
  wrk.create(wrk.ROOT_KEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION", wrk.ACCESS_ALL);

  var value = 7000;
  if (mode == "ie8mode") 
    value = 8000;
  else if (mode == "ie9mode") 
    value = 9000;

  wrk.writeIntValue("firefox.exe", value);
  wrk.writeIntValue("plugin-container.exe", value);
}

IEMode.getFilterListString = function () {
  var list = [];
  var filter = document.getElementById('filterList');
  var count = filter.view.rowCount;

  for (var i = 0; i < count; i++) {
    var rule = filter.view.getCellText(i, filter.columns['columnRule']);
    var enabled = filter.view.getCellValue(i, filter.columns['columnEnabled']);
    var item = rule + (enabled == "true" ? "" : "\b");
    list.push(item);
  }
  list.sort();
  return list.join(" ");
}

IEMode.getOfficialFilterListString = function () {
  var list = [];
  var filter = document.getElementById('filterList-official');
  var count = filter.view.rowCount;

  for (var i = 0; i < count; i++) {
    var rule = filter.view.getCellText(i, filter.columns['columnRule-official']);
    var enabled = filter.view.getCellValue(i, filter.columns['columnEnabled-official']);
    var item = rule + (enabled == "true" ? "" : "\b");
    list.push(item);
  }
  list.sort();
  return list.join(" ");
}

IEMode.updateDelButtonStatus = function () {
  var en = document.getElementById('filtercbx').checked;
  var delbtn = document.getElementById('delbtn');
  var filter = document.getElementById('filterList');
  delbtn.disabled = (!en) || (filter.view.selection.count < 1);
}

IEMode.updateAddButtonStatus = function () {
  var en = document.getElementById('filtercbx').checked;
  var addbtn = document.getElementById('addbtn');
  var urlbox = document.getElementById('urlbox');
  addbtn.disabled = (!en) || (IEMode.trim(urlbox.value).length < 1);
}

IEMode.findRule = function (value) {
  var filter = document.getElementById('filterList');
  var count = filter.view.rowCount;
  for (var i = 0; i < count; i++) {
    var rule = filter.view.getCellText(i, filter.columns['columnRule']);
    if (rule == value) return i;
  }
  return -1;
}

IEMode.addNewURL = function () {
  var filter = document.getElementById('filterList');
  var urlbox = document.getElementById('urlbox');
  var rule = IEMode.trim(urlbox.value);
  if (rule != "") {
    if ((rule != "about:blank") && (rule.indexOf("://") < 0)) {
      rule = (/^[A-Za-z]:/.test(rule) ? "file:///" + rule.replace(/\\/g, "/") : rule);
      if (/^file:\/\/.*/.test(rule)) rule = encodeURI(rule);
    }
    if (!/^\/(.*)\/$/.exec(rule)) rule = rule.replace(/\/$/, "/*");
    rule = rule.replace(/\s/g, "%20");
    var idx = IEMode.addFilterRule(rule, true);
    urlbox.value = "";
    filter.view.selection.select(idx);
    filter.boxObject.ensureRowIsVisible(idx);
  }
  filter.focus();
  IEMode.updateAddButtonStatus();
}
IEMode.defaultFilter = function () {
  IEMode.initFilterList(true);
}

IEMode.delSelected = function () {
  var filter = document.getElementById('filterList');
  var rules = document.getElementById('filterChilds');
  if (filter.view.selection.count > 0) {
    for (var i = rules.childNodes.length - 1; i >= 0; i--) {
      if (filter.view.selection.isSelected(i)) rules.removeChild(rules.childNodes[i]);
    }
  }
  IEMode.updateDelButtonStatus();
}

IEMode.onClickFilterList = function (e) {
  var filter = document.getElementById('filterList');
  if (!filter.disabled && e.button == 0 && e.detail >= 2) {
    if (filter.view.selection.count == 1) {
      var urlbox = document.getElementById('urlbox');
      urlbox.value = filter.view.getCellText(filter.currentIndex, filter.columns['columnRule']);
      urlbox.select();
      IEMode.updateAddButtonStatus();
    }
  }
}

IEMode.onClickFilterListOfficial = function (e) {
}

IEMode._saveToFile = function (aList) {
  var fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
  var stream = Cc["@mozilla.org/network/file-output-stream;1"].createInstance(Ci.nsIFileOutputStream);
  var converter = Cc["@mozilla.org/intl/converter-output-stream;1"].createInstance(Ci.nsIConverterOutputStream);

  fp.init(window, null, fp.modeSave);
  fp.defaultExtension = "txt";
  fp.defaultString = "IEModePref";
  fp.appendFilters(fp.filterText);

  if (fp.show() != fp.returnCancel) {
    try {
      if (fp.file.exists()) fp.file.remove(true);
      fp.file.create(fp.file.NORMAL_FILE_TYPE, 0666);
      stream.init(fp.file, 0x02, 0x200, null);
      converter.init(stream, "UTF-8", 0, 0x0000);

      for (var i = 0; i < aList.length; i++) {
        aList[i] = aList[i] + "\n";
        converter.writeString(aList[i]);
      }
    } finally {
      converter.close();
      stream.close();
    }
  }
}

IEMode._loadFromFile = function () {
  var fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
  var stream = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(Ci.nsIFileInputStream);
  var converter = Cc["@mozilla.org/intl/converter-input-stream;1"].createInstance(Ci.nsIConverterInputStream);

  fp.init(window, null, fp.modeOpen);
  fp.defaultExtension = "txt";
  fp.appendFilters(fp.filterText);

  if (fp.show() != fp.returnCancel) {
    try {
      var input = {};
      stream.init(fp.file, 0x01, 0444, null);
      converter.init(stream, "UTF-8", 0, 0x0000);
      converter.readString(stream.available(), input);
      var linebreak = input.value.match(/(((\n+)|(\r+))+)/m)[1];
      return [true, input.value.split(linebreak)];
    } finally {
      converter.close();
      stream.close();
    }
  }
  return [false, null];
}

IEMode._getAllSettings = function (isDefault) {
  var prefix = "extensions.iemode.";
  var prefservice = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefService);
  var prefs = (isDefault ? prefservice.getDefaultBranch("") : prefservice.getBranch(""));
  var preflist = prefs.getChildList(prefix, {});

  var aList = ["IEModePref"];
  for (var i = 0; i < preflist.length; i++) {
    try {
      var value = null;
      switch (prefs.getPrefType(preflist[i])) {
      case prefs.PREF_BOOL:
        value = prefs.getBoolPref(preflist[i]);
        break;
      case prefs.PREF_INT:
        value = prefs.getIntPref(preflist[i]);
        break;
      case prefs.PREF_STRING:
        value = prefs.getComplexValue(preflist[i], Ci.nsISupportsString).data;
        break;
      }
      aList.push(preflist[i] + "=" + value);
    } catch (e) {ERROR(e)}
  }
  return aList;
}

IEMode._setAllSettings = function (aList) {
  if (!aList) return;
  if (aList.length == 0) return;
  if (aList[0] != "IEModePref") return;

  var prefservice = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefService);
  var prefs = prefservice.getBranch("");

  var aPrefs = [];
  for (var i = 1; i < aList.length; i++) {
    var index = aList[i].indexOf("=");
    if (index > 0) {
      var name = aList[i].substring(0, index);
      var value = aList[i].substring(index + 1, aList[i].length);
      aPrefs.push([name, value]);
    }
  }
  for (var i = 0; i < aPrefs.length; i++) {
    try {
      var name = aPrefs[i][0];
      var value = aPrefs[i][1];
      switch (prefs.getPrefType(name)) {
      case prefs.PREF_BOOL:
        prefs.setBoolPref(name, /true/i.test(value));
        break;
      case prefs.PREF_INT:
        prefs.setIntPref(name, value);
        break;
      case prefs.PREF_STRING:
        if (value.indexOf('"') == 0) value = value.substring(1, value.length - 1);
        var sString = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
        sString.data = value;
        prefs.setComplexValue(name, Ci.nsISupportsString, sString);
        break;
      }
    } catch (e) {ERROR(e)}
  }
}

IEMode.removeDEP = function () {
  function getFileFromURLSpec(path) {
    var fph = Services.io.getProtocolHandler("file").QueryInterface(Ci.nsIFileProtocolHandler);
    return fph.getFileFromURLSpec(path).QueryInterface(Ci.nsILocalFile);
  }
  AddonManager.getAddonByID("iemode@mozilla.com.cn", function (addon) {
    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      var file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      file.initWithPath(getFileFromURLSpec(addon.getResourceURI("").spec).path + "\\bin\\Dep.exe");
      var process = Cc['@mozilla.org/process/util;1'].createInstance(Ci.nsIProcess);
      process.init(file);
      var path = Services.dirsvc.get("XCurProcD", Ci.nsIFile);
    
      path.append("plugin-container.exe");
      process.run(false, [path.path], 1);
    } catch (e) {
      ERROR(e);
    }
  });
}
