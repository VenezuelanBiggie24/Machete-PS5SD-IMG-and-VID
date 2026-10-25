  // ---------- Machete Studio Pro: Creative Dock & Presets ----------
  var STYLE_PRESETS = {
    cinematic: ', cinematic lighting, 8k resolution, photorealistic, dramatic atmosphere, octane render, unreal engine 5, ray tracing',
    photo: ', professional 35mm photograph, soft natural lighting, high dynamic range, f/1.8, bokeh, hyper-detailed skin texture',
    cyberpunk: ', cyberpunk neon aesthetic, volumetric fog, rainy reflections, intricate mechanical details, vibrant glowing palette',
    anime: ', anime visual masterpiece, makoto shinkai aesthetic, luminous color grading, clean linework, atmospheric depth',
    product: ', commercial studio product photography, pristine clean background, balanced softbox illumination, sharp focus, 8k',
    fantasy: ', dark fantasy epic composition, ethereal bioluminescent glow, ornate medieval architecture, mythical atmosphere',
    retro: ', 90s vintage film photography, 35mm grain, nostalgic warm hues, disposable camera aesthetic, authentic color grading',
    concept: ', digital concept art, matte painting, trending on artstation, masterpiece, intricate composition, cinematic vista'
  };

  function syncAspectButtons(w, h) {
    var aspectBtns = document.querySelectorAll('.aspect-btn');
    for (var i = 0; i < aspectBtns.length; i++) {
      var bw = parseInt(aspectBtns[i].getAttribute('data-w'), 10);
      var bh = parseInt(aspectBtns[i].getAttribute('data-h'), 10);
      if (bw === w && bh === h) {
        aspectBtns[i].classList.add('on');
      } else {
        aspectBtns[i].classList.remove('on');
      }
    }
  }

  function initStudioDock() {
    // Style chips
    var chips = document.querySelectorAll('.style-chip');
    for (var i = 0; i < chips.length; i++) {
      (function (btn) {
        btn.onclick = function () {
          var key = btn.getAttribute('data-style');
          var textToAdd = STYLE_PRESETS[key];
          if (!textToAdd) return;
          var p = $('prompt');
          if (p.value.indexOf(textToAdd.trim()) === -1) {
            p.value = p.value ? (p.value.trim().replace(/,+$/, '') + textToAdd) : textToAdd.replace(/^,\s*/, '');
            btn.classList.add('on');
          } else {
            p.value = p.value.replace(textToAdd, '').replace(textToAdd.trim(), '');
            btn.classList.remove('on');
          }
          if (p.dispatchEvent) p.dispatchEvent(new Event('input'));
          persist();
        };
      })(chips[i]);
    }

    // Magic polish button
    var magicBtn = $('magic-polish');
    if (magicBtn) {
      magicBtn.onclick = function () {
        var p = $('prompt');
        var polish = ', ultra-detailed masterpiece, cinematic illumination, hyper-realistic textures, 8k octane render';
        if (p.value.indexOf('ultra-detailed masterpiece') === -1) {
          p.value = p.value ? (p.value.trim().replace(/,+$/, '') + polish) : polish.replace(/^,\s*/, '');
          toast('Prompt polished with AI descriptors.');
          if (p.dispatchEvent) p.dispatchEvent(new Event('input'));
          persist();
        } else {
          toast('Prompt already enhanced.');
        }
      };
    }

    // Prompt clear button
    var clrBtn = $('prompt-clear');
    var promptEl = $('prompt');
    function updateClearBtn() {
      if (clrBtn && promptEl) {
        clrBtn.style.display = promptEl.value.trim().length > 0 ? 'inline-block' : 'none';
      }
    }
    if (promptEl) {
      promptEl.addEventListener('input', updateClearBtn);
      updateClearBtn();
    }
    if (clrBtn && promptEl) {
      clrBtn.onclick = function () {
        promptEl.value = '';
        updateClearBtn();
        // remove active state from style chips
        var chips = document.querySelectorAll('.style-chip');
        for (var c = 0; c < chips.length; c++) chips[c].classList.remove('on');
        if (promptEl.dispatchEvent) promptEl.dispatchEvent(new Event('input'));
        persist();
      };
    }

    // Quick negative chips
    var negChips = document.querySelectorAll('.neg-chip');
    for (var n = 0; n < negChips.length; n++) {
      (function (nBtn) {
        nBtn.onclick = function () {
          var negText = nBtn.getAttribute('data-neg');
          if (!negText) return;
          var negField = $('neg');
          if (negField.value.indexOf(negText.trim()) === -1) {
            negField.value = negField.value ? (negField.value.trim().replace(/,+$/, '') + ', ' + negText) : negText;
            nBtn.classList.add('on');
          } else {
            negField.value = negField.value.replace(negText, '').replace(negText.trim(), '').replace(/,\s*,/g, ',').replace(/^,\s*|,\s*$/g, '');
            nBtn.classList.remove('on');
          }
          if (negField.dispatchEvent) negField.dispatchEvent(new Event('input'));
          persist();
        };
      })(negChips[n]);
    }

    // Aspect ratio buttons
    var aspectBtns = document.querySelectorAll('.aspect-btn');
    for (var j = 0; j < aspectBtns.length; j++) {
      (function (btn) {
        btn.onclick = function () {
          var w = parseInt(btn.getAttribute('data-w'), 10);
          var h = parseInt(btn.getAttribute('data-h'), 10);
          if (w && h) {
            $('w').value = w;
            $('h').value = h;
            syncAspectButtons(w, h);
            markPreset();
            persist();
            if ($('w').dispatchEvent) $('w').dispatchEvent(new Event('change'));
          }
        };
      })(aspectBtns[j]);
    }

    // Watch for size inputs to sync aspect buttons
    var wInput = $('w');
    var hInput = $('h');
    if (wInput && hInput) {
      var syncAspectFromInputs = function () {
        syncAspectButtons(parseInt(wInput.value, 10), parseInt(hInput.value, 10));
      };
      wInput.addEventListener('change', syncAspectFromInputs);
      hInput.addEventListener('change', syncAspectFromInputs);
      syncAspectFromInputs();
    }

    // Face Studio Scenario Presets
    var faceChips = document.querySelectorAll('.face-preset-chip');
    for (var f = 0; f < faceChips.length; f++) {
      (function (fBtn) {
        fBtn.onclick = function () {
          var scenarioPrompt = fBtn.getAttribute('data-prompt');
          if (scenarioPrompt) {
            $('prompt').value = scenarioPrompt;
            var enBox = $('facestudio-enable');
            if (enBox && !enBox.checked) {
              enBox.checked = true;
              toggleFaceStudio();
            }
            toast('Scenario applied. Face lock engaged.');
            if ($('prompt').dispatchEvent) $('prompt').dispatchEvent(new Event('input'));
            persist();
          }
        };
      })(faceChips[f]);
    }
  }
