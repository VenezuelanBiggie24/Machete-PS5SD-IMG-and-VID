  // ---------- Face Studio (Identity Lock) ----------
  var faceState = {
    enabled: false,
    hasFace: false,
    photoMakerAvailable: false
  };

  function updateFaceUi() {
    var enBox = $('facestudio-enable');
    if (enBox) enBox.checked = faceState.enabled;
    show('facestudio-content', faceState.enabled);

    var badge = $('facestudio-badge');
    if (badge) {
      if (faceState.enabled && faceState.hasFace) {
        badge.className = 'face-badge active';
        text(badge, 'Locked');
      } else if (faceState.enabled) {
        badge.className = 'face-badge';
        text(badge, 'No Face');
      } else {
        badge.className = 'face-badge';
        text(badge, 'Standby');
      }
    }

    var img = $('face-img');
    var placeholder = $('face-placeholder');
    var clearBtn = $('face-clear-btn');
    if (faceState.hasFace) {
      if (img) {
        img.src = APP + '/face/image?t=' + Date.now();
        img.style.display = 'block';
      }
      if (placeholder) placeholder.style.display = 'none';
      if (clearBtn) clearBtn.style.display = 'inline-block';
    } else {
      if (img) img.style.display = 'none';
      if (placeholder) placeholder.style.display = 'block';
      if (clearBtn) clearBtn.style.display = 'none';
    }

    var hint = $('face-method-hint');
    if (hint) {
      if (faceState.photoMakerAvailable) {
        text(hint, 'PhotoMaker adapter detected: Native identity transfer active.');
      } else {
        text(hint, 'Facial guidance active: Auto-injects identity reference.');
      }
    }
  }

  function loadFaceStudio() {
    req('GET', APP + '/face/status', null, function (code, data) {
      if (code === 200 && data) {
        faceState.hasFace = !!data.has_face;
        faceState.photoMakerAvailable = !!(data.photomaker_ready || data.photomaker_available);
      }
      req('GET', APP + '/settings', null, function (sCode, sData) {
        if (sCode === 200 && sData) {
          faceState.enabled = !!sData.face_studio_enabled;
        }
        updateFaceUi();
      });
    });
  }

  function toggleFaceStudio() {
    var want = $('facestudio-enable').checked;
    req('POST', APP + '/settings', { face_studio_enabled: want }, function (code, data) {
      if (code === 200 && data) {
        faceState.enabled = !!data.face_studio_enabled;
        toast(faceState.enabled ? 'Face Studio enabled.' : 'Face Studio disabled.');
      } else {
        $('facestudio-enable').checked = !want;
        toast('Could not change Face Studio setting.');
      }
      updateFaceUi();
    });
  }

  function uploadFaceBuffer(buf) {
    var x = new XMLHttpRequest();
    x.open('POST', APP + '/face/upload', true);
    x.setRequestHeader('X-PS5SD-Request', '1');
    x.setRequestHeader('Content-Type', 'image/png');
    x.onreadystatechange = function () {
      if (x.readyState !== 4) return;
      var res = null;
      try { res = JSON.parse(x.responseText); } catch (e) {}
      if (x.status === 200 && res && res.ok) {
        toast('Reference face updated.');
        loadFaceStudio();
      } else {
        toast('Failed to upload face reference: ' + (res && res.error ? res.error : 'error ' + x.status));
      }
    };
    x.send(buf);
  }

  function handleFaceFile(file) {
    if (!file || !file.type.match(/^image\//)) {
      toast('Please choose a valid image file (PNG/JPEG).');
      return;
    }
    var reader = new FileReader();
    reader.onload = function (e) {
      uploadFaceBuffer(e.target.result);
    };
    reader.readAsArrayBuffer(file);
  }

  function clearFaceReference() {
    req('DELETE', APP + '/face', null, function (code, data) {
      if (code === 200 && data && data.ok) {
        toast('Reference face cleared.');
        loadFaceStudio();
      } else {
        toast('Could not clear face reference.');
      }
    });
  }

  function initFaceStudio() {
    var en = $('facestudio-enable');
    if (en) en.onchange = toggleFaceStudio;

    var upBtn = $('face-upload-btn');
    var fileInput = $('face-file-input');
    if (upBtn && fileInput) {
      upBtn.onclick = function () { fileInput.click(); };
      fileInput.onchange = function () {
        if (fileInput.files && fileInput.files[0]) {
          handleFaceFile(fileInput.files[0]);
          fileInput.value = '';
        }
      };
    }

    var clrBtn = $('face-clear-btn');
    if (clrBtn) clrBtn.onclick = clearFaceReference;

    var avatar = $('face-avatar');
    if (avatar) {
      avatar.ondragover = function (e) { e.preventDefault(); return false; };
      avatar.ondrop = function (e) {
        e.preventDefault();
        if (e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files[0]) {
          handleFaceFile(e.dataTransfer.files[0]);
        }
        return false;
      };
    }

    loadFaceStudio();
  }
