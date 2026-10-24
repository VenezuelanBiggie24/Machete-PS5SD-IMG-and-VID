  // ---------- Machete Studio Pro: Creative Dock & Presets ----------
  var STYLE_PRESETS = {
    cinematic: ', cinematic lighting, 8k resolution, photorealistic, dramatic atmosphere, octane render, unreal engine 5, ray tracing',
    photo: ', professional 35mm photograph, soft natural lighting, high dynamic range, f/1.8, bokeh, hyper-detailed skin texture',
    cyberpunk: ', cyberpunk neon aesthetic, volumetric fog, rainy reflections, intricate mechanical details, vibrant glowing palette',
    anime: ', anime visual masterpiece, makoto shinkai aesthetic, luminous color grading, clean linework, atmospheric depth',
    product: ', commercial studio product photography, pristine clean background, balanced softbox illumination, sharp focus, 8k',
    fantasy: ', dark fantasy epic composition, ethereal bioluminescent glow, ornate medieval architecture, mythical atmosphere'
  };

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
            for (var k = 0; k < aspectBtns.length; k++) aspectBtns[k].classList.remove('on');
            btn.classList.add('on');
            markPreset();
            persist();
            if ($('w').dispatchEvent) $('w').dispatchEvent(new Event('change'));
          }
        };
      })(aspectBtns[j]);
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
