'use strict';

// Browser-only presentation controller. No GPIO or credentials are persisted.
// The ESP32 and ESP8266 firmware expose the same bounded JSON API.
(function () {
  var token = '';
  var pins = [];
  var activeTab = 'dashboard';
  var fetchInProgress = false;
  var titles = {
    dashboard: 'Overview', gpio: 'GPIO laboratory', pins: 'Pin reference',
    loader: 'Firmware loader', displays: 'Display diagnostics',
    serial: 'Serial monitor', help: 'Help & recovery'
  };

  function $(id) { return document.getElementById(id); }
  function text(id, value) { $(id).textContent = String(value); }
  function formatBytes(value) {
    return typeof value === 'number' && Number.isFinite(value)
      ? (value / 1024).toFixed(0) + ' KiB' : '—';
  }
  function feedback(id, message, type) {
    var node = $(id);
    node.textContent = message;
    node.classList.remove('error', 'success');
    if (type) node.classList.add(type);
  }
  async function request(path, method, body, credential) {
    var selected = credential === undefined ? token : credential;
    var opts = {method: method || 'GET', cache: 'no-store', headers: {}};
    if (selected) opts.headers['X-Blaze-Token'] = selected;
    if (body !== undefined) {
      opts.headers['Content-Type'] = 'application/json';
      opts.body = JSON.stringify(body);
    }
    var response = await fetch(path, opts);
    var result = await response.json();
    if (!response.ok) throw new Error(result.message || ('HTTP ' + response.status));
    return result;
  }
  function showTab(which) {
    if (!Object.prototype.hasOwnProperty.call(titles, which)) return;
    activeTab = which;
    document.querySelectorAll('[data-tab]').forEach(function (button) {
      var selected = button.dataset.tab === which;
      button.setAttribute('aria-selected', String(selected));
      $(button.dataset.tab).hidden = !selected;
    });
    text('pageTitle', titles[which]);
    if (which === 'gpio') refreshGpio();
  }
  function selectedPin() {
    return pins.find(function (p) { return p.number === Number($('pin').value); });
  }
  function restricted(pin) {
    return !pin.exposed || pin.strap || pin.reserved ||
      pin.category === 'flash' || pin.category === 'input-only' ||
      pin.category === 'analog' || pin.category === 'not-exposed' ||
      pin.category === 'absent' || pin.category === 'special';
  }
  function inspectPin() {
    var pin = selectedPin();
    if (!pin) return;
    text('pinLabel', pin.label || ('GPIO' + pin.number));
    text('pinCategory', pin.category);
    text('pinCapabilities', (pin.input ? 'Input' : 'No digital input') +
      ' · ' + (pin.output ? 'Output' : 'No digital output') +
      ' · ' + (pin.pwm ? 'PWM capable' : 'No PWM') +
      ' · ' + (pin.adc ? 'ADC capable' : 'No ADC'));
    text('pinWarning', pin.warning);
    $('ackWrap').hidden = !restricted(pin);
    $('ack').checked = false;
    updateControls();
  }
  function updateControls() {
    var mode = $('mode').value;
    $('inputControls').hidden = mode !== 'input';
    $('outputControls').hidden = mode !== 'output';
    $('pwmControls').hidden = mode !== 'pwm';
    $('level').disabled = mode !== 'output';
    $('pullup').disabled = mode !== 'input';
    $('frequency').disabled = mode !== 'pwm';
    $('duty').disabled = mode !== 'pwm';
    var pin = selectedPin();
    // Keep input-only, flash and unavailable pins selectable; never imply driving works.
    if (pin && mode === 'output' && !pin.output) {
      text('pinWarning', pin.warning + ' Requested output is unsupported; firmware will reject it.');
    } else if (pin && mode === 'pwm' && !pin.pwm) {
      text('pinWarning', pin.warning + ' PWM on this pin is unsupported; firmware will reject it.');
    } else if (pin) {
      text('pinWarning', pin.warning);
    }
    var hz = Number($('frequency').value);
    var duty = Number($('duty').value);
    var periodMs = 1000 / hz;
    text('frequencyValue', hz + ' Hz');
    text('dutyValue', duty + '%');
    text('pwmEstimate', 'Estimated period: ' + periodMs.toFixed(2) +
      ' ms · HIGH: ' + (periodMs * duty / 100).toFixed(2) + ' ms');
  }
  function fillPins() {
    var select = $('pin');
    select.textContent = '';
    pins.forEach(function (p) {
      var option = document.createElement('option');
      option.value = String(p.number);
      option.textContent = p.label || ('GPIO' + p.number);
      select.appendChild(option);
    });
    var preferred = pins.find(function (p) { return p.number === 32 && p.output; }) ||
      pins.find(function (p) { return p.number === 4 && p.output; }) || pins[0];
    if (preferred) select.value = String(preferred.number);
    inspectPin();
    renderPinReference();
    fillI2cPins();
  }
  function fillI2cPins() {
    var sda = $('i2cSda'), scl = $('i2cScl');
    [sda, scl].forEach(function (select) {
      select.textContent = '';
      pins.forEach(function (pin) {
        var option = document.createElement('option');
        option.value = String(pin.number);
        option.textContent = pin.label || ('GPIO' + pin.number);
        select.appendChild(option);
      });
    });
    var esp32 = pins.some(function (pin) { return pin.number === 32; });
    sda.value = esp32 ? '21' : '4';
    scl.value = esp32 ? '22' : '5';
    inspectI2cPins();
  }
  function inspectI2cPins() {
    var sda = pins.find(function (p) { return p.number === Number($('i2cSda').value); });
    var scl = pins.find(function (p) { return p.number === Number($('i2cScl').value); });
    $('i2cAck').checked = false;
    $('oledConfirm').checked = false;
    if (!sda || !scl) return;
    text('i2cWarnings', 'SDA: ' + sda.warning + ' SCL: ' + scl.warning +
      (sda.number === scl.number ? ' Both pins must be different.' : '') +
      (!sda.input || !sda.output || !scl.input || !scl.output ?
        ' One or both pins cannot run I²C on this board.' : ''));
  }
  async function runOled() {
    if (!token) {
      feedback('oledMessage','Verify admin access on Overview before operating the OLED.','error');
      return;
    }
    if (!$('oledConfirm').checked) {
      feedback('oledMessage','Confirm the actual 128×64 controller model, 3.3 V wiring and address first.','error');
      return;
    }
    $('oledRun').disabled = true;
    feedback('oledMessage','Sending a bounded display write…');
    try {
      var result = await request('/api/oled/test','POST',{
        sda: Number($('i2cSda').value),
        scl: Number($('i2cScl').value),
        address: Number($('oledAddress').value),
        controller: $('oledModel').value,
        pattern: $('oledPattern').value,
        ack: $('i2cAck').checked,
        confirm: $('oledConfirm').checked
      });
      feedback('oledMessage',result.note ||
        'Pattern sent. This is not physical display verification.','success');
      $('oledConfirm').checked = false;
    } catch (error) {
      feedback('oledMessage','OLED test unsuccessful: ' + error.message,'error');
    } finally {
      $('oledRun').disabled = false;
    }
  }

  async function scanI2c() {
    if (!token) {
      feedback('i2cMessage', 'Verify admin access on Overview before scanning.', 'error');
      return;
    }
    $('i2cScan').disabled = true;
    $('i2cResults').textContent = '';
    feedback('i2cMessage', 'Scanning addresses · brief local bus operation…');
    try {
      var result = await request('/api/i2c/scan', 'POST', {
        sda: Number($('i2cSda').value),
        scl: Number($('i2cScl').value),
        ack: $('i2cAck').checked
      });
      if (!Array.isArray(result.addresses)) throw new Error('Invalid scan response');
      var addresses = result.addresses.filter(function (addr) {
        return Number.isInteger(addr) && addr >= 0x08 && addr <= 0x77;
      });
      feedback('i2cMessage', addresses.length ? addresses.length +
        ' address(es) acknowledged. Device type not identified.' :
        'Scan completed · no devices responded. Check 3.3 V power and pull-ups.', 'success');
      addresses.forEach(function (addr) {
        var item = document.createElement('span');
        item.className = 'address-tag';
        item.textContent = '0x' + addr.toString(16).toUpperCase().padStart(2, '0');
        $('i2cResults').appendChild(item);
      });
    } catch (error) {
      feedback('i2cMessage', 'Scan did not complete: ' + error.message, 'error');
    } finally {
      $('i2cScan').disabled = false;
    }
  }
  function renderPinReference() {
    var container = $('pinRows');
    var term = $('pinSearch').value.trim().toLowerCase();
    container.textContent = '';
    var results = pins.filter(function (pin) {
      return !term || [pin.number, 'GPIO' + pin.number, pin.label || '',
        pin.category, pin.warning, pin.input ? 'input' : '',
        pin.output ? 'output' : '', pin.pwm ? 'pwm' : ''].join(' ').toLowerCase().includes(term);
    });
    text('pinCount', results.length + ' of ' + pins.length + ' pins');
    results.forEach(function (pin) {
      var card = document.createElement('div');
      card.className = 'pin-item' + (!pin.exposed ? ' unavailable' : restricted(pin) ? ' restricted' : '');
      var top = document.createElement('div');
      top.className = 'pin-item-top';
      var name = document.createElement('strong');
      name.textContent = pin.label || ('GPIO' + pin.number);
      var type = document.createElement('small');
      type.textContent = pin.category;
      top.appendChild(name);
      top.appendChild(type);
      var abilities = document.createElement('p');
      abilities.className = 'abilities';
      abilities.textContent = (pin.input ? 'IN' : '—') + ' · ' +
        (pin.output ? 'OUT' : '—') + ' · ' + (pin.pwm ? 'PWM' : '—') +
        ' · ' + (pin.adc ? 'ADC' : '—');
      var note = document.createElement('p');
      note.textContent = pin.warning;
      card.appendChild(top);
      card.appendChild(abilities);
      card.appendChild(note);
      container.appendChild(card);
    });
    if (!results.length) {
      var empty = document.createElement('p');
      empty.className = 'pin-empty';
      empty.textContent = 'No pins match the search. Clear the filter to view the full reference.';
      container.appendChild(empty);
    }
  }
  async function refreshDevice() {
    try {
      var result = await request('/api/status');
      text('deviceBoard', result.board || 'Unknown');
      text('deviceVersion', result.version || 'Unknown');
      text('deviceHeap', formatBytes(result.heap_free));
      text('deviceFlash', formatBytes(result.flash_bytes));
      $('connectionState').className = 'connection online';
      text('connectionState', 'Connected · 192.168.4.1');
    } catch (error) {
      $('connectionState').className = 'connection offline';
      text('connectionState', 'Device unreachable');
      text('deviceBoard', 'Connection failed');
      feedback('authstate', 'Device unreachable: ' + error.message, 'error');
    }
  }
  async function verifyToken() {
    var candidate = $('token').value.trim();
    if (!candidate) {
      feedback('authstate', 'Enter the admin token shown on the serial console.', 'error');
      return;
    }
    $('tokenApply').disabled = true;
    try {
      await request('/api/auth/check', 'GET', undefined, candidate);
      token = candidate;
      $('token').value = '';
      feedback('authstate', 'Access verified with this device.', 'success');
    } catch (error) {
      token = '';
      feedback('authstate', 'Token not verified: ' + error.message, 'error');
    } finally {
      $('tokenApply').disabled = false;
    }
  }
  function clearToken() {
    token = '';
    $('token').value = '';
    feedback('authstate', 'Access cleared from this page.');
  }
  async function refreshGpio() {
    if (fetchInProgress || activeTab !== 'gpio') return;
    fetchInProgress = true;
    try {
      var result = await request('/api/gpio');
      var running = result.status === 'running' && result.pin >= 0;
      text('runState', running ? 'Running' : 'Idle');
      $('runState').className = running ? 'state running' : 'state';
      text('livePin', running ? 'GPIO' + result.pin + ' · ' + result.mode : 'No active pin');
      var level = !running ? '—' : result.mode === 'pwm' ? (result.duty_percent + '% PWM') :
        result.level === 1 ? 'HIGH' : 'LOW';
      text('liveLevel', level);
      text('rising', Number(result.rising_edges || 0));
      text('falling', Number(result.falling_edges || 0));
      text('remaining', running ? (result.expires_in_ms / 1000).toFixed(1) + ' s' : '—');
      text('runningMode', running ? result.mode.toUpperCase() : '—');
      $('leaseFill').style.width = (running ? Math.min(100, 100 * result.expires_in_ms / 20000) : 0) + '%';
      text('pwmRunning', running && result.mode === 'pwm' ?
        'Requested ' + result.frequency_hz + ' Hz at ' + result.duty_percent +
        '% duty. This is commanded output, not a measured waveform.' :
        'GPIO counts use polling and may miss fast edges. Outputs are commands, not measured voltages.');
    } catch (error) {
      text('runState', 'Offline');
      $('runState').className = 'state';
      feedback('gpioMessage', 'Cannot read GPIO status: ' + error.message, 'error');
    } finally {
      fetchInProgress = false;
    }
  }
  async function startTest() {
    if (!token) {
      feedback('gpioMessage', 'Verify the device admin token on Overview first.', 'error');
      return;
    }
    var mode = $('mode').value;
    var requestBody = {
      pin: Number($('pin').value), mode: mode, level: Number($('level').value),
      pullup: mode === 'input' && $('pullup').checked, ack: $('ack').checked
    };
    if (mode === 'pwm') {
      requestBody.frequency_hz = Number($('frequency').value);
      requestBody.duty_percent = Number($('duty').value);
    }
    $('start').disabled = true;
    try {
      var result = await request('/api/gpio', 'POST', requestBody);
      feedback('gpioMessage', result.message || 'Experiment started.', 'success');
    } catch (error) {
      feedback('gpioMessage', error.message, 'error');
    } finally {
      $('start').disabled = false;
      await refreshGpio();
    }
  }
  async function stopTest() {
    if (!token) {
      feedback('gpioMessage', 'Verify the device admin token before sending STOP.', 'error');
      return;
    }
    try {
      await request('/api/gpio/stop', 'POST');
      feedback('gpioMessage', 'Pin released. Laboratory idle.', 'success');
    } catch (error) {
      feedback('gpioMessage', 'STOP could not be confirmed: ' + error.message, 'error');
    }
    await refreshGpio();
  }
  document.querySelectorAll('[data-tab]').forEach(function (button) {
    button.addEventListener('click', function () { showTab(button.dataset.tab); });
  });
  $('refreshDevice').addEventListener('click', refreshDevice);
  $('tokenApply').addEventListener('click', verifyToken);
  $('tokenClear').addEventListener('click', clearToken);
  $('token').addEventListener('keydown', function (event) {
    if (event.key === 'Enter') verifyToken();
  });
  $('pin').addEventListener('change', inspectPin);
  $('mode').addEventListener('change', function () { $('ack').checked = false; updateControls(); });
  $('frequency').addEventListener('input', updateControls);
  $('duty').addEventListener('input', updateControls);
  $('pinSearch').addEventListener('input', renderPinReference);
  $('start').addEventListener('click', startTest);
  $('stop').addEventListener('click', stopTest);
  $('i2cSda').addEventListener('change', inspectI2cPins);
  $('i2cScl').addEventListener('change', inspectI2cPins);
  $('i2cScan').addEventListener('click', scanI2c);
  $('oledModel').addEventListener('change', function () { $('oledConfirm').checked = false; });
  $('oledAddress').addEventListener('change', function () { $('oledConfirm').checked = false; });
  $('oledPattern').addEventListener('change', function () { $('oledConfirm').checked = false; });
  $('oledRun').addEventListener('click', runOled);
  updateControls();
  showTab('dashboard');
  (async function init() {
    await refreshDevice();
    try {
      pins = (await request('/api/pins')).pins || [];
      fillPins();
    } catch (error) {
      text('pinLabel', 'Pin reference unavailable');
      text('pinWarning', error.message);
      text('pinCount', 'Cannot load pins');
    }
  }());
  // Network and GPIO status are polled only when their tab is open.
  setInterval(refreshGpio, 1000);
}());
