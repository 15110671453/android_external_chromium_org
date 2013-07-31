// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('apps_dev_tool', function() {
  'use strict';

  // The list of all packed/unpacked apps and extensions.
  var completeList = [];

  // The list of all packed apps.
  var packedAppList = [];

  // The list of all unpacked apps.
  var unpackedAppList = [];

  // The list of all packed extensions.
  var packedExtensionList = [];

  // The list of all unpacked extensions.
  var unpackedExtensionList = [];

  /** const*/ var AppsDevTool = apps_dev_tool.AppsDevTool;

  /**
   * @param {string} a first string.
   * @param {string} b second string.
   * @return {number} 1, 0, -1 if |a| is lexicographically greater, equal or
   *     lesser than |b| respectively.
   */
  function compare(a, b) {
    return a > b ? 1 : (a == b ? 0 : -1);
  }

  /**
   * Returns a translated string.
   *
   * Wrapper function to make dealing with translated strings more concise.
   * Equivalent to localStrings.getString(id).
   *
   * @param {string} id The id of the string to return.
   * @return {string} The translated string.
   */
  function str(id) {
    return loadTimeData.getString(id);
  }

  /**
   * compares strings |app1| and |app2| (case insensitive).
   * @param {string} app1 first app_name.
   * @param {string} app2 second app_name.
   */
  function compareByName(app1, app2) {
    return compare(app1.name.toLowerCase(), app2.name.toLowerCase());
  }

  /**
   * Refreshes the app.
   */
  function reloadAppDisplay() {
    var extensions = new ItemsList($('extensions-tab'), packedExtensionList,
                                   unpackedExtensionList);
    var apps = new ItemsList($('apps-tab'), packedAppList, unpackedAppList);
    extensions.showItemNodes();
    apps.showItemNodes();
  }

  /**
   * Applies the given |filter| to the items list.
   * @param {string} filter Curent string in the search box.
   */
  function rebuildAppList(filter) {
    packedAppList = [];
    unpackedAppList = [];
    packedExtensionList = [];
    unpackedExtensionList = [];

    for (var i = 0; i < completeList.length; i++) {
      var item = completeList[i];
      if (filter && item.name.toLowerCase().search(filter.toLowerCase()) < 0)
        continue;
      if (item.isApp) {
        if (item.is_unpacked)
          unpackedAppList.push(item);
        else
          packedAppList.push(item);
      } else {
        if (item.is_unpacked)
          unpackedExtensionList.push(item);
        else
          packedExtensionList.push(item);
      }
    }
  }

  /**
   * Create item nodes from the metadata.
   * @constructor
   */
  function ItemsList(tabNode, packedItems, unpackedItems) {
    this.packedItems_ = packedItems;
    this.unpackedItems_ = unpackedItems;
    this.tabNode_ = tabNode;
    assert(this.tabNode_);
  }

  ItemsList.prototype = {

    /**
     * |packedItems_| holds the metadata of packed apps or extensions.
     * @type {!Array.<!Object>}
     * @private
     */
    packedItems_: [],

    /**
     * |unpackedItems_| holds the metadata of unpacked apps or extensions.
     * @type {!Array.<!Object>}
     * @private
     */
    unpackedItems_: [],

    /**
     * |tabNode_| html element holding the tab containing the list of packed
     * and unpacked items.
     * @type {!HTMLElement}
     * @private
     */
    tabNode_: null,

    /**
     * Creates all items from scratch.
     */
    showItemNodes: function() {
      var packedItemsList = this.tabNode_.querySelector('#packed-list .items');
      var unpackedItemsList = this.tabNode_.querySelector(
          '#unpacked-list .items');
      packedItemsList.innerHTML = '';
      unpackedItemsList.innerHTML = '';

      // Iterate over the items and add each item to the list.
      this.tabNode_.classList.toggle('empty-item-list',
                                     this.packedItems_.length == 0 &&
                                     this.unpackedItems_.length == 0);

      // Iterate over the items in the packed items and add each item to the
      // list.
      this.tabNode_.querySelector('#packed-list').classList.toggle(
          'empty-item-list', this.packedItems_.length == 0);
      for (var i = 0; i < this.packedItems_.length; ++i) {
        packedItemsList.appendChild(this.createNode_(this.packedItems_[i]));
      }

      // Iterate over the items in the unpacked items and add each item to the
      // list.
      this.tabNode_.querySelector('#unpacked-list').classList.toggle(
          'empty-item-list', this.unpackedItems_.length == 0);
      for (var i = 0; i < this.unpackedItems_.length; ++i) {
        unpackedItemsList.appendChild(this.createNode_(this.unpackedItems_[i]));
      }
    },

    /**
     * Synthesizes and initializes an HTML element for the item metadata
     * given in |item|.
     * @param {!Object} item A dictionary of item metadata.
     * @return {!Node} The newly created node.
     * @private
     */
    createNode_: function(item) {
      var template = $('template-collection').querySelector(
          '.extension-list-item-wrapper');
      var node = template.cloneNode(true);
      node.id = item.id;

      if (!item.enabled)
        node.classList.add('inactive-extension');

      node.querySelector('.extension-disabled').hidden = item.enabled;

      if (!item.may_disable)
        node.classList.add('may-not-disable');

      var itemNode = node.querySelector('.extension-list-item');
      itemNode.style.backgroundImage = 'url(' + item.icon_url + ')';

      var title = node.querySelector('.extension-title');
      title.textContent = item.name;
      title.onclick = function() {
        if (item.isApp)
          ItemsList.launchApp(item.id);
      };

      var version = node.querySelector('.extension-version');
      version.textContent = item.version;

      var description = node.querySelector('.extension-description span');
      description.textContent = item.description;

      // The 'allow in incognito' checkbox.
      this.setAllowIncognitoCheckbox_(item, node);

      // The 'allow file:// access' checkbox.
      if (item.wants_file_access)
        this.setAllowFileAccessCheckbox_(item, node);

      // The 'Options' checkbox.
      if (item.enabled && item.options_url) {
        var options = node.querySelector('.options-link');
        options.href = item.options_url;
        options.hidden = false;
      }

      // The 'Permissions' link.
      this.setPermissionsLink_(item, node);

      // The 'View in Web Store/View Web Site' link.
      if (item.homepage_url)
        this.setWebstoreLink_(item, node);

      // The 'Reload' checkbox.
      if (item.allow_reload)
        this.setReloadLink_(item, node);

      if (item.type == 'packaged_app') {
        // The 'Launch' link.
        var launch = node.querySelector('.launch-link');
        launch.addEventListener('click', function(e) {
          ItemsList.launchApp(item.id);
        });
        launch.hidden = false;

        // The 'Restart' link.
        var restart = node.querySelector('.restart-link');
        restart.addEventListener('click', function(e) {
          chrome.developerPrivate.restart(item.id, function() {
            ItemsList.loadItemsInfo();
          });
        });
        restart.hidden = false;
      }
      // The terminated reload link.
      if (!item.terminated)
        this.setEnabledCheckbox_(item, node);
      else
        this.setTerminatedReloadLink_(item, node);

      // Set remove button handler.
      this.setRemoveButton_(item, node);

      // First get the item id.
      var idLabel = node.querySelector('.extension-id');
      idLabel.textContent = ' ' + item.id;

      // Set the path and show the pack button, if provided by unpacked
      // app / extension.
      if (item.is_unpacked) {
        var loadPath = node.querySelector('.load-path');
        loadPath.hidden = false;
        loadPath.querySelector('span:nth-of-type(2)').textContent =
            ' ' + item.path;
        this.setPackButton_(item, node);
      }

      // Then the 'managed, cannot uninstall/disable' message.
      if (!item.may_disable)
        node.querySelector('.managed-message').hidden = false;

      this.setActiveViews_(item, node);

      return node;
    },

    /**
     * Sets the webstore link.
     * @param {!Object} item A dictionary of item metadata.
     * @param {!HTMLElement} el HTML element containing all items.
     * @private
     */
    setWebstoreLink_: function(item, el) {
      var siteLink = el.querySelector('.site-link');
      siteLink.href = item.homepage_url;
      siteLink.textContent = str(item.homepageProvided ?
         'extensionSettingsVisitWebsite' : 'extensionSettingsVisitWebStore');
      siteLink.hidden = false;
      siteLink.target = '_blank';
    },

    /**
     * Sets the reload link handler.
     * @param {!Object} item A dictionary of item metadata.
     * @param {!HTMLElement} el HTML element containing all items.
     * @private
     */
    setReloadLink_: function(item, el) {
      var reload = el.querySelector('.reload-link');
      reload.addEventListener('click', function(e) {
        chrome.developerPrivate.reload(item.id, function() {
          ItemsList.loadItemsInfo();
        });
      });
      reload.hidden = false;
    },

    /**
     * Sets the terminated reload link handler.
     * @param {!Object} item A dictionary of item metadata.
     * @param {!HTMLElement} el HTML element containing all items.
     * @private
     */
    setTerminatedReloadLink_: function(item, el) {
      var terminatedReload = el.querySelector('.terminated-reload-link');
      terminatedReload.addEventListener('click', function(e) {
        chrome.developerPrivate.reload(item.id, function() {
          ItemsList.loadItemsInfo();
        });
      });
      terminatedReload.hidden = false;
    },

    /**
     * Sets the permissions link handler.
     * @param {!Object} item A dictionary of item metadata.
     * @param {!HTMLElement} el HTML element containing all items.
     * @private
     */
    setPermissionsLink_: function(item, el) {
      var permissions = el.querySelector('.permissions-link');
      permissions.addEventListener('click', function(e) {
        chrome.developerPrivate.showPermissionsDialog(item.id);
      });
    },

    /**
     * Sets the pack button handler.
     * @param {!Object} item A dictionary of item metadata.
     * @param {!HTMLElement} el HTML element containing all items.
     * @private
     */
    setPackButton_: function(item, el) {
      var packButton = el.querySelector('.pack-link');
      packButton.addEventListener('click', function(e) {
        $('item-root-dir').value = item.path;
        AppsDevTool.showOverlay($('packItemOverlay'));
      });
      packButton.hidden = false;
    },

    /**
     * Sets the remove button handler.
     * @param {!Object} item A dictionary of item metadata.
     * @param {!HTMLElement} el HTML element containing all items.
     * @private
     */
    setRemoveButton_: function(item, el) {
      var deleteLink = el.querySelector('.delete-link');
      deleteLink.addEventListener('click', function(e) {
        var options = {showConfirmDialog: false};
        chrome.management.uninstall(item.id, options, function() {
          ItemsList.loadItemsInfo();
        });
      });
    },

    /**
     * Sets the handler for enable checkbox.
     * @param {!Object} item A dictionary of item metadata.
     * @param {!HTMLElement} el HTML element containing all items.
     * @private
     */
    setEnabledCheckbox_: function(item, el) {
      var enable = el.querySelector('.enable-checkbox');
      enable.hidden = false;
      enable.querySelector('input').disabled = !item.may_disable;

      if (item.may_disable) {
        enable.addEventListener('click', function(e) {
          chrome.developerPrivate.enable(
              item.id, !!e.target.checked, function() {
                ItemsList.loadItemsInfo();
              });
        });
      }

      enable.querySelector('input').checked = item.enabled;
    },

    /**
     * Sets the handler for the allow_file_access checkbox.
     * @param {!Object} item A dictionary of item metadata.
     * @param {!HTMLElement} el HTML element containing all items.
     * @private
     */
    setAllowFileAccessCheckbox_: function(item, el) {
      var fileAccess = el.querySelector('.file-access-control');
      fileAccess.addEventListener('click', function(e) {
        chrome.developerPrivate.allowFileAccess(item.id, !!e.target.checked);
      });
      fileAccess.querySelector('input').checked = item.allow_file_access;
      fileAccess.hidden = false;
    },

    /**
     * Sets the handler for the allow_incognito checkbox.
     * @param {!Object} item A dictionary of item metadata.
     * @param {!HTMLElement} el HTML element containing all items.
     * @private
     */
    setAllowIncognitoCheckbox_: function(item, el) {
      if (item.allow_incognito) {
        var incognito = el.querySelector('.incognito-control');
        incognito.addEventListener('change', function(e) {
          chrome.developerPrivate.allowIncognito(
              item.id, !!e.target.checked, function() {
            ItemsList.loadItemsInfo();
          });
        });
        incognito.querySelector('input').checked = item.incognito_enabled;
        incognito.hidden = false;
      }
    },

    /**
     * Sets the active views link of an item. Clicking on the link
     * opens devtools window to inspect.
     * @param {!Object} item A dictionary of item metadata.
     * @param {!HTMLElement} el HTML element containing all items.
     * @private
     */
    setActiveViews_: function(item, el) {
      if (!item.views.length)
        return;

      var activeViews = el.querySelector('.active-views');
      activeViews.hidden = false;
      var link = activeViews.querySelector('a');

      item.views.forEach(function(view, i) {
        var label = view.path +
            (view.incognito ? ' ' + str('viewIncognito') : '') +
            (view.render_process_id == -1 ? ' ' + str('viewInactive') : '');
        link.textContent = label;
        link.addEventListener('click', function(e) {
          // Opens the devtools inspect window for the page.
          chrome.developerPrivate.inspect({
            extension_id: String(item.id),
            render_process_id: String(view.render_process_id),
            render_view_id: String(view.render_view_id),
            incognito: view.incognito,
          });
        });

        if (i < item.views.length - 1) {
          link = link.cloneNode(true);
          activeViews.appendChild(link);
        }
      });
    },
  };

  /**
   * Rebuilds the item list and reloads the app on every search input.
   */
  ItemsList.onSearchInput = function() {
    rebuildAppList($('search').value);
    reloadAppDisplay();
  };

  /**
   * Fetches items info and reloads the app.
   * @param {Function=} opt_callback An optional callback to be run when
   *     reloading is finished.
   */
  ItemsList.loadItemsInfo = function(callback) {
    chrome.developerPrivate.getItemsInfo(true, true, function(info) {
      completeList = info.sort(compareByName);
      ItemsList.onSearchInput();
      assert(/undefined|function/.test(typeof callback));
      if (callback)
        callback();
    });
  };

  /**
   * Launches the item with id |id|.
   * @param {string} id Item ID.
   */
  ItemsList.launchApp = function(id) {
    chrome.management.launchApp(id, function() {
      ItemsList.loadItemsInfo();
    });
  };

  /**
   * Selects the unpacked apps / extensions tab, scrolls to the app /extension
   * with the given |id| and expand its details.
   * @param {string} id Identifier of the app / extension.
   */
  ItemsList.makeUnpackedExtensionVisible = function(id) {
    var tabbox = document.querySelector('tabbox');
    // Unpacked tab is the first tab.
    tabbox.selectedIndex = 0;

    var firstItem =
        document.querySelector('#unpacked-list .extension-list-item-wrapper');
    if (!firstItem)
      return;
    // Scroll relatively to the position of the first item.
    var node = $(id);
    document.body.scrollTop = node.offsetTop - firstItem.offsetTop;
  };

  return {
    ItemsList: ItemsList,
  };
});
