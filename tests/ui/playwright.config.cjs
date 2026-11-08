const { defineConfig } = require('@playwright/test');
const path = require('node:path');

module.exports = defineConfig({
  testDir: __dirname,
  testMatch: '**/*.spec.cjs',
  timeout: 20000,
  retries: 0,
  workers: 1,
  reporter: 'list',
  projects: [
    {name: 'desktop-chromium', use: {browserName: 'chromium', viewport: {width: 1280, height: 820}}},
    {name: 'mobile-chromium', use: {browserName: 'chromium', viewport: {width: 390, height: 844}, isMobile: true, hasTouch: true}}
  ],
  use: {baseURL: 'http://127.0.0.1:8765'},
  webServer: {
    command: 'python3 -m http.server 8765 --bind 127.0.0.1 --directory main/web',
    cwd: path.resolve(__dirname, '../..'),
    url: 'http://127.0.0.1:8765/',
    timeout: 30000,
    reuseExistingServer: false
  }
});
