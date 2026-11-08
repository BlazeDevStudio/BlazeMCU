const { test, expect } = require('@playwright/test');

const ADMIN = 'TEST-ONLY-ADMIN-SECRET-NOT-IN-FIRMWARE';
const pins = Array.from({length: 40}, (_, number) => {
  const absent = [20,24,28,29,30,31,37,38].includes(number);
  const flash = number >= 6 && number <= 11;
  const inputOnly = [34,35,36,39].includes(number);
  const strap = [0,2,4,5,12,15].includes(number);
  return {
    number, label: 'GPIO' + number,
    exposed: !absent && !flash,
    input: !absent && !flash, output: !absent && !flash && !inputOnly,
    pwm: !absent && !flash && !inputOnly, adc: [32,33,34].includes(number),
    strap, reserved: false,
    category: flash ? 'flash' : absent ? 'absent' : inputOnly ? 'input-only' : strap ? 'strap' : 'gpio',
    warning: inputOnly ? 'Input only; output driving unsupported.' :
      flash ? 'Internal SPI flash. Selection only; never drive.' :
      strap ? 'Boot strapping, external loads may affect reset.' : 'Check board wiring.'
  };
});

async function mockDevice(page) {
  const state = {status: 'idle', pin: -1, mode: 'input', level: 0,
    rising_edges: 0, falling_edges: 0, expires_in_ms: 0};
  const serial = {status:'idle',pin:-1,baud:0,expires_in_ms:0,bytes:0,hex:'',text:''};
  const calls = [];
  await page.route('**/api/**', async route => {
    const req = route.request();
    const path = new URL(req.url()).pathname;
    const method = req.method();
    const authorized = req.headers()['x-blaze-token'] === ADMIN;
    calls.push({path, method, authorized});
    let status = 200, body;
    if (path === '/api/status') {
      body = {board: 'esp32-wroom-32-devkit-reference', version: '0.4.0-preview',
        heap_free: 153600, flash_bytes: 4194304, loader: 'disabled'};
    } else if (path === '/api/pins') {
      body = {pins};
    } else if (path === '/api/gpio' && method === 'GET') {
      body = state;
    } else if (path === '/api/auth/check') {
      status = authorized ? 200 : 401;
      body = authorized ? {authenticated: true} : {message: 'Admin token required'};
    } else if (!authorized) {
      status = 401; body = {message: 'Admin token required'};
    } else if (path === '/api/gpio' && method === 'POST') {
      const input = req.postDataJSON();
      if (input.pin === 34 && input.mode !== 'input') {
        status = 422; body = {message: 'Unsupported input-only output'};
      } else {
        Object.assign(state, {status:'running', pin:input.pin, mode:input.mode,
          level:input.level, expires_in_ms:20000});
        body = {message:'Test active for up to 20 seconds'};
      }
    } else if (path === '/api/gpio/stop') {
      Object.assign(state, {status:'idle', pin:-1, expires_in_ms:0});
      body = {status:'idle'};
    } else if (path === '/api/uart/start' && method === 'POST') {
      const input = req.postDataJSON();
      if ([1,3,6,7,8,9,10,11,16].includes(input.pin)) {
        status = 422; body = {message:'Reserved UART pin'};
      } else {
        Object.assign(serial,{status:'running',pin:input.pin,baud:input.baud,
          expires_in_ms:18000,bytes:2,hex:'4849',text:'HI'});
        body = {status:'running',message:'RX-only UART listening for at most 20 seconds'};
      }
    } else if (path === '/api/uart/stop' && method === 'POST') {
      Object.assign(serial,{status:'idle',pin:-1,baud:0,expires_in_ms:0,bytes:0,hex:'',text:''});
      body = {status:'idle'};
    } else if (path === '/api/uart' && method === 'GET') {
      body = {...serial};
      serial.bytes = 0; serial.hex = ''; serial.text = '';
    } else if (path === '/api/i2c/scan' && method === 'POST') {
      const input = req.postDataJSON();
      if (input.sda === input.scl) {
        status = 422; body = {message:'Unsupported or identical I2C pins'};
      } else {
        body = {status:'completed', sda:input.sda, scl:input.scl,
          addresses:[0x3C,0x68], note:'Address ACK only'};
      }
    } else if (path === '/api/oled/test' && method === 'POST') {
      const input = req.postDataJSON();
      if (!input.confirm) {
        status = 409; body = {message:'Confirm model and wiring'};
      } else if (![60,61].includes(input.address) ||
                 !['ssd1306','sh1106'].includes(input.controller) ||
                 !['checker','border','blank'].includes(input.pattern)) {
        status = 422; body = {message:'Unsupported OLED'};
      } else {
        body = {status:'completed',
          note:'Test pattern sent; unverified on actual display. Use Blank to clear display RAM.'};
      }
    } else {
      status = 404; body = {message:'Unknown route'};
    }
    await route.fulfill({status, contentType:'application/json', body:JSON.stringify(body)});
  });
  return calls;
}
async function connect(page) {
  const calls = await mockDevice(page);
  await page.goto('/');
  await expect(page.locator('#connectionState')).toContainText('Connected');
  return calls;
}
async function authenticate(page, value=ADMIN) {
  await page.locator('#token').fill(value);
  await page.locator('#tokenApply').click();
}
test('dashboard, mobile navigation and complete restricted-pin discovery', async ({page}) => {
  await connect(page);
  await expect(page.locator('#deviceBoard')).toContainText('esp32-wroom');
  await page.locator('#tab-pins').click();
  await expect(page.locator('#pinCount')).toContainText('40 of 40 pins');
  await page.locator('#pinSearch').fill('flash');
  await expect(page.locator('#pinRows')).toContainText('GPIO6');
  await page.locator('#pinSearch').fill('input-only');
  await expect(page.locator('#pinRows')).toContainText('GPIO34');
  await page.locator('#tab-loader').click();
  await expect(page.getByText('FLASH · unavailable')).toBeDisabled();
  await expect(page.getByText('PREBOOT · unavailable')).toBeDisabled();
});
test('incorrect token never yields verified state and blocks mutations', async ({page}) => {
  const calls = await connect(page);
  await authenticate(page, 'not-the-token');
  await expect(page.locator('#authstate')).toContainText('not verified');
  await page.locator('#tab-gpio').click();
  await page.locator('#start').click();
  await expect(page.locator('#gpioMessage')).toContainText('Verify the device admin token');
  expect(calls.filter(x => x.path === '/api/gpio' && x.method === 'POST')).toHaveLength(0);
});
test('verified GPIO behavior, warnings and explicit STOP', async ({page}) => {
  const calls = await connect(page);
  await authenticate(page);
  await expect(page.locator('#authstate')).toContainText('Access verified');
  await page.locator('#tab-gpio').click();
  await page.locator('#pin').selectOption('34');
  await page.locator('#mode').selectOption('output');
  await expect(page.locator('#pinWarning')).toContainText('output is unsupported');
  await page.locator('#pin').selectOption('21');
  await page.locator('#mode').selectOption('pwm');
  await page.locator('#frequency').evaluate(el => { el.value = '500'; el.dispatchEvent(new Event('input', {bubbles: true})); });
  await page.locator('#duty').evaluate(el => { el.value = '30'; el.dispatchEvent(new Event('input', {bubbles: true})); });
  await page.locator('#start').click();
  await expect(page.locator('#gpioMessage')).toContainText('Test active');
  await expect(page.locator('#liveLevel')).toContainText('PWM');
  await page.locator('#stop').click();
  await expect(page.locator('#gpioMessage')).toContainText('Pin released');
  expect(calls.some(x => x.path === '/api/gpio' && x.method === 'POST' && x.authorized)).toBe(true);
});
test('I2C discovery displays ACK addresses only and never guesses display', async ({page}) => {
  const calls = await connect(page);
  await authenticate(page);
  await page.locator('#tab-displays').click();
  await expect(page.locator('#i2cSda')).toHaveValue('21');
  await expect(page.locator('#i2cScl')).toHaveValue('22');
  await page.locator('#i2cScan').click();
  await expect(page.locator('#i2cMessage')).toContainText('2 address(es)');
  await expect(page.locator('#i2cResults')).toContainText('0x3C');
  await expect(page.locator('#i2cResults')).toContainText('0x68');
  await expect(page.locator('#i2cMessage')).toContainText('Device type not identified');
  expect(calls.some(x => x.path === '/api/i2c/scan' && x.authorized)).toBe(true);
});

test('OLED requires model confirmation and never silently guesses display type', async ({page}) => {
  const calls = await connect(page);
  await authenticate(page);
  await page.locator('#tab-displays').click();
  await expect(page.locator('#oledModel')).toHaveValue('ssd1306');
  await expect(page.locator('#oledAddress')).toHaveValue('60');
  await page.locator('#oledRun').click();
  await expect(page.locator('#oledMessage')).toContainText('Confirm the actual');
  expect(calls.filter(x => x.path === '/api/oled/test')).toHaveLength(0);
  await page.locator('#oledModel').selectOption('sh1106');
  await page.locator('#oledAddress').selectOption('61');
  await page.locator('#oledPattern').selectOption('border');
  await page.locator('#oledConfirm').check();
  await page.locator('#oledRun').click();
  await expect(page.locator('#oledMessage')).toContainText('Test pattern sent');
  await expect(page.locator('#oledConfirm')).not.toBeChecked();
  expect(calls.filter(x => x.path === '/api/oled/test' && x.authorized)).toHaveLength(1);
  await page.locator('#oledPattern').selectOption('blank');
  await page.locator('#oledRun').click();
  await expect(page.locator('#oledMessage')).toContainText('Confirm the actual');
  expect(calls.filter(x => x.path === '/api/oled/test')).toHaveLength(1);
});

test('UART RX-only session: auth, pin selection, bounded capture, STOP', async ({page}) => {
  const calls = await connect(page);
  await page.locator('#tab-serial').click();
  await page.locator('#uartStart').click();
  await expect(page.locator('#uartMessage')).toContainText('Verify device admin access');
  expect(calls.filter(x => x.path === '/api/uart/start')).toHaveLength(0);
  await page.locator('#tab-dashboard').click();
  await authenticate(page);
  await page.locator('#tab-serial').click();
  await expect(page.locator('#uartPin')).toHaveValue('16');
  await page.locator('#uartPin').selectOption('34');
  await page.locator('#uartBaud').selectOption('9600');
  await page.locator('#uartStart').click();
  await expect(page.locator('#uartMessage')).toContainText('RX-only UART listening');
  await expect(page.locator('#uartText')).toContainText('HI');
  await expect(page.locator('#uartHex')).toContainText('4849');
  await page.locator('#uartStop').click();
  await expect(page.locator('#uartMessage')).toContainText('UART RX session stopped');
  await expect(page.locator('#uartState')).toHaveText('Idle');
  expect(calls.some(x=>x.path==='/api/uart/start' && x.authorized)).toBe(true);
  await page.locator('#uartClear').click();
  await expect(page.locator('#uartText')).toHaveText('No captured data.');
});
