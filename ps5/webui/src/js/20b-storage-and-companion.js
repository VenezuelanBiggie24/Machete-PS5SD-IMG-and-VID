  // ---------- Storage Manager & Telegram Companion ----------
  var storageDevices = [];

  function formatBytes(bytes) {
    if (!bytes || bytes <= 0) return '0 GB';
    var gib = bytes / (1024 * 1024 * 1024);
    if (gib >= 1000) {
      return (gib / 1024).toFixed(2) + ' TB';
    }
    return gib.toFixed(1) + ' GB';
  }

  function renderStorageDevices() {
    var list = $('storage-devices-list');
    if (!list) return;
    clear(list);

    if (!storageDevices || storageDevices.length === 0) {
      list.appendChild(el('div', 'hint', 'No storage devices found.'));
      return;
    }

    for (var i = 0; i < storageDevices.length; i++) {
      (function (dev) {
        var card = el('div', 'storage-item' + (dev.is_active ? ' active' : ''));
        var head = el('div', 'storage-head');
        var nameSpan = el('span', 'storage-name', dev.label || dev.mount);
        head.appendChild(nameSpan);

        var badge = el('span', 'storage-badge' + (dev.is_active ? ' cur' : ''), dev.is_active ? 'Active' : (dev.is_usb ? 'USB' : 'Internal'));
        head.appendChild(badge);
        card.appendChild(head);

        var usedBytes = Math.max(0, dev.total_bytes - dev.free_bytes);
        var pct = dev.total_bytes > 0 ? Math.min(100, Math.round((usedBytes / dev.total_bytes) * 100)) : 0;

        var barBox = el('div', 'storage-bar-box');
        var barFill = el('div', 'storage-bar-fill');
        barFill.style.width = pct + '%';
        barBox.appendChild(barFill);
        card.appendChild(barBox);

        var info = el('div', 'storage-info');
        info.appendChild(el('span', '', formatBytes(dev.free_bytes) + ' free of ' + formatBytes(dev.total_bytes)));
        info.appendChild(el('span', '', pct + '% used'));
        card.appendChild(info);

        if (!dev.is_active) {
          var btnRow = el('div', 'storage-btn-row');
          var selBtn = el('button', '', 'Use this storage');
          selBtn.onclick = function () {
            selectStorageMount(dev.mount);
          };
          btnRow.appendChild(selBtn);
          card.appendChild(btnRow);
        }

        list.appendChild(card);
      })(storageDevices[i]);
    }
  }

  function loadStorageDevices() {
    req('GET', APP + '/storage/devices', null, function (code, data) {
      if (code === 200 && Array.isArray(data)) {
        storageDevices = data;
        renderStorageDevices();
        for (var i = 0; i < data.length; i++) {
          if (data[i].is_active) {
            var desc = $('storage-active-desc');
            if (desc) text(desc, 'Active Storage: ' + data[i].label + ' (' + data[i].mount + ')');
            break;
          }
        }
      }
    });
  }

  function selectStorageMount(mount) {
    req('POST', APP + '/storage/select', { mount: mount }, function (code, data) {
      if (code === 200 && data && data.ok) {
        toast('Storage switched to ' + mount + '.');
        loadStorageDevices();
        loadModels();
      } else {
        toast('Could not switch storage: ' + (data && data.error ? data.error : 'error ' + code));
      }
    });
  }

  // ---------- Telegram Companion Settings ----------
  function loadCompanionSettings() {
    req('GET', APP + '/settings', null, function (code, data) {
      if (code !== 200 || !data) return;
      var tgEn = $('opt-tg-enable');
      if (tgEn) {
        tgEn.checked = !!data.telegram_enabled;
        show('tg-config-box', !!data.telegram_enabled);
      }
      var tgToken = $('opt-tg-token');
      if (tgToken && data.telegram_token) tgToken.value = data.telegram_token;
      var tgChat = $('opt-tg-chat');
      if (tgChat && data.telegram_chat_id) tgChat.value = data.telegram_chat_id;

      var usbEn = $('opt-usb-enable');
      if (usbEn) usbEn.checked = !!data.usb_storage_enabled;

      var ssEn = $('opt-screensaver');
      if (ssEn) ssEn.checked = !!data.screensaver_ambient;

      var thInput = $('opt-th-thresh');
      if (thInput && data.thermal_warning_threshold) thInput.value = data.thermal_warning_threshold;

      updateCompanionBadge();
    });
  }

  function updateCompanionBadge() {
    req('GET', APP + '/companion/status', null, function (code, data) {
      var badge = $('bot-badge');
      if (!badge) return;
      if (code === 200 && data && data.running) {
        badge.className = 'badge on';
        text(badge, 'Running');
      } else if (code === 200 && data && data.enabled) {
        badge.className = 'badge warn';
        text(badge, 'Standby');
      } else {
        badge.className = 'badge off';
        text(badge, 'Off');
      }
    });
  }

  function saveCompanionSettings() {
    var en = $('opt-tg-enable').checked;
    var token = ($('opt-tg-token').value || '').trim();
    var chat = ($('opt-tg-chat').value || '').trim();

    req('POST', APP + '/settings', {
      telegram_enabled: en,
      telegram_token: token,
      telegram_chat_id: chat
    }, function (code, data) {
      if (code === 200 && data) {
        toast('Telegram companion settings saved.');
        updateCompanionBadge();
      } else {
        toast('Could not save companion settings.');
      }
    });
  }

  function initStorageAndCompanion() {
    var refBtn = $('storage-refresh-btn');
    if (refBtn) refBtn.onclick = loadStorageDevices;

    var usbBox = $('opt-usb-enable');
    if (usbBox) {
      usbBox.onchange = function () {
        req('POST', APP + '/settings', { usb_storage_enabled: usbBox.checked }, function (code) {
          if (code === 200) {
            toast(usbBox.checked ? 'External USB scanning enabled.' : 'External USB scanning disabled.');
            loadStorageDevices();
          }
        });
      };
    }

    var ssBox = $('opt-screensaver');
    if (ssBox) {
      ssBox.onchange = function () {
        req('POST', APP + '/settings', { screensaver_ambient: ssBox.checked }, function (code) {
          if (code === 200) toast(ssBox.checked ? 'TV screensaver enabled.' : 'TV screensaver disabled.');
        });
      };
    }

    var thIn = $('opt-th-thresh');
    var thMinus = $('th-thresh-');
    var thPlus = $('th-thresh+');
    function saveThresh(val) {
      var n = Math.max(70, Math.min(95, parseInt(val, 10) || 80));
      if (thIn) thIn.value = n;
      req('POST', APP + '/settings', { thermal_warning_threshold: n }, function () {});
    }
    if (thMinus) thMinus.onclick = function () { saveThresh(parseInt(thIn.value, 10) - 1); };
    if (thPlus) thPlus.onclick = function () { saveThresh(parseInt(thIn.value, 10) + 1); };
    if (thIn) thIn.onchange = function () { saveThresh(thIn.value); };

    var tgEn = $('opt-tg-enable');
    if (tgEn) {
      tgEn.onchange = function () {
        show('tg-config-box', tgEn.checked);
        saveCompanionSettings();
      };
    }

    var tgSave = $('tg-save-btn');
    if (tgSave) tgSave.onclick = saveCompanionSettings;

    loadStorageDevices();
    loadCompanionSettings();
  }
