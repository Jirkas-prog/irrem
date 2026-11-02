const { test, expect } = require('@playwright/test');

test('both languages preserve draft settings and user names without sending configuration writes', async ({ page }) => {
  const errors = [];
  page.on('pageerror', error => errors.push(error.message));
  await page.goto('/');
  await expect(page.locator('#connection')).toHaveText('Device connected');
  await expect(page.locator('#virtual-grid .virtual-key')).toHaveCount(20);
  await page.locator('a[href="#wifi"]').click();
  await page.locator('[name="ssid"]').fill('My unchanged network');
  await page.locator('[name="wifi_password"]').fill('example-test-password');
  const writes = [];
  page.on('request', request => { if (request.method() === 'POST') writes.push(request.url()); });
  await page.locator('#ui-language').selectOption('cs');
  await expect(page.locator('html')).toHaveAttribute('lang', 'cs');
  await expect(page.locator('#connection')).toHaveText('Zařízení připojeno');
  await expect(page.locator('#page-wifi h1')).toHaveText('Doma ve vaší síti.');
  await expect(page.locator('[name="ssid"]')).toHaveValue('My unchanged network');
  await expect(page.locator('[name="wifi_password"]')).toHaveValue('example-test-password');
  await page.locator('#ui-language').selectOption('en');
  await expect(page.locator('#page-wifi h1')).toHaveText('At home on your network.');
  await expect(page.locator('[name="wifi_password"]')).toHaveValue('example-test-password');
  await page.locator('a[href="#buttons"]').click();
  await expect(page.locator('#button-list strong').first()).toHaveText('retez_on');
  await expect(page.locator('#virtual-grid .virtual-key')).toHaveCount(20);
  expect(writes).toEqual([]);
  expect(errors).toEqual([]);
});

test('an explicit language survives a reload and translates backend status messages', async ({ page }) => {
  await page.goto('/');
  await expect(page.locator('#connection')).toHaveText('Device connected');
  await page.locator('#ui-language').selectOption('cs');
  await page.reload();
  await expect(page.locator('#ui-language')).toHaveValue('cs');
  await expect(page.locator('html')).toHaveAttribute('lang', 'cs');
  await page.locator('#ui-language').selectOption('en');
  await page.locator('a[href="#mqtt"]').click();
  await expect(page.locator('#mqtt-detail')).toHaveText('Connected');
  await expect(page.locator('#mqtt-secret')).toHaveText('A password is saved.');
});

test('Czech browser preference and unavailable storage both work', async ({ browser }) => {
  const context = await browser.newContext({ locale: 'cs-CZ' });
  const page = await context.newPage();
  await page.addInitScript(() => {
    Object.defineProperty(Storage.prototype, 'getItem', { value() { throw new Error('storage disabled'); } });
    Object.defineProperty(Storage.prototype, 'setItem', { value() { throw new Error('storage disabled'); } });
  });
  await page.goto('http://127.0.0.1:18739');
  await expect(page.locator('#connection')).toHaveText('Zařízení připojeno');
  await page.locator('#ui-language').selectOption('en');
  await expect(page.locator('#connection')).toHaveText('Device connected');
  await expect(page.locator('html')).toHaveAttribute('lang', 'en');
  await context.close();
});

test('API errors and placeholders are localized without changing inserted values', async ({ page }) => {
  await page.goto('/');
  await expect(page.locator('#connection')).toHaveText('Device connected');
  await page.route('**/api/mqtt/reconnect', route => route.fulfill({ status: 400, contentType: 'application/json', body: JSON.stringify({ error: 'Neplatný požadavek.' }) }));
  await page.locator('a[href="#mqtt"]').click();
  await page.locator('#mqtt-reconnect').click();
  await expect(page.locator('#notice')).toHaveText('Invalid request.');
  await page.locator('#ui-language').selectOption('cs');
  await expect(page.locator('#notice')).toHaveText('Neplatný požadavek.');
  const values = await page.evaluate(() => [IRremI18n.t(undefined), IRremI18n.t('Chyba {0}', 'x{0}')]);
  expect(values).toEqual(['', 'Chyba x{0}']);
});

test('mobile header keeps the language selector reachable', async ({ page }) => {
  await page.setViewportSize({ width: 360, height: 800 });
  await page.goto('/');
  const selector = page.locator('#ui-language');
  await expect(selector).toBeVisible();
  const bounds = await selector.boundingBox();
  expect(bounds.x).toBeGreaterThanOrEqual(0);
  expect(bounds.x + bounds.width).toBeLessThanOrEqual(360);
  await selector.selectOption('cs');
  await expect(page.locator('html')).toHaveAttribute('lang', 'cs');
});

test('switching language does not falsely report reconnection after a network failure', async ({ page }) => {
  await page.goto('/');
  await expect(page.locator('#connection')).toHaveText('Device connected');
  await page.route('**/api/state', route => route.abort());
  await expect(page.locator('#connection')).toHaveText('Connection lost');
  await page.locator('#ui-language').selectOption('cs');
  await expect(page.locator('#connection')).toHaveText('Spojení přerušeno');
  await expect(page.locator('.virtual-key').first()).toBeDisabled();
  await page.locator('#ui-language').selectOption('en');
  await expect(page.locator('#connection')).toHaveText('Connection lost');
});
