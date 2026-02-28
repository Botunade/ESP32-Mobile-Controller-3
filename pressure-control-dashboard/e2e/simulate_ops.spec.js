const { test, expect } = require('@playwright/test');

test('simulate dashboard operations and check for silent errors', async ({ page }) => {
    const consoleErrors = [];
    const logs = [];

    // Capture console logs and errors
    page.on('console', msg => {
        if (msg.type() === 'error') {
            consoleErrors.push(msg.text());
        }
        logs.push(`[${msg.type()}] ${msg.text()}`);
    });

    // Capture page errors
    page.on('pageerror', error => {
        consoleErrors.push(error.message);
    });

    console.log('--- Navigating to Dashboard ---');
    await page.goto('http://localhost:3000');

    // Wait for initial load
    await page.waitForTimeout(2000);

    // 1. Check Connection Status
    const connectionStatus = await page.locator('#connectionStatus').textContent();
    console.log(`Initial Connection Status: ${connectionStatus.trim()}`);

    // We expect it to at least NOT be "Initializing..." if Firebase is working
    expect(connectionStatus).not.toContain('Initializing');

    // 2. Check for Telemetry Updating
    console.log('--- Checking Telemetry ---');
    const pressureReading = await page.locator('#valPressure').textContent();
    console.log(`Pressure: ${pressureReading}`);

    // 3. Simulate "Start PID"
    console.log('--- Simulating Start PID ---');
    await page.click('#btnStart');
    await page.waitForTimeout(1000);

    // 4. Simulate Config Update
    console.log('--- Simulating Config Update ---');
    await page.fill('#kp', '2.5');
    await page.fill('#ki', '0.5');
    await page.fill('#kd', '0.01');
    await page.fill('#setpoint', '75');

    await page.click('#btnSaveConfig');

    // Check if saving overlay appears
    const overlayVisible = await page.isVisible('#savingOverlay');
    if (overlayVisible) {
        console.log('Saving overlay is visible as expected.');
    }

    // Wait for sync badge to update to "Synced"
    await expect(page.locator('#syncConfigBadge')).toContainText('Synced', { timeout: 10000 });
    console.log('Settings synced successfully.');

    // 5. Final Error Check
    if (consoleErrors.length > 0) {
        console.error('--- SILENT ERRORS DETECTED ---');
        consoleErrors.forEach(err => console.error(`Error: ${err}`));
    } else {
        console.log('--- NO SILENT ERRORS DETECTED ---');
    }

    // Report logs for analysis
    console.log('--- Browser Logs ---');
    logs.forEach(log => console.log(log));

    expect(consoleErrors).toHaveLength(0);
});
