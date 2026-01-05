let isScanning = false;

function scanNetworks() {
    if (isScanning) return;

    isScanning = true;
    const btn = document.getElementById('scanBtn');
    const status = document.getElementById('scanStatus');

    btn.disabled = true;
    btn.textContent = '⏳ Scanning...';
    status.innerHTML = '<span class="spinner"></span>Searching for networks...';

    // Simulate scanning process (3 seconds)
    setTimeout(() => {
        status.innerHTML = '✓ Scan complete! Found networks will appear here soon.';
        setTimeout(() => {
            btn.disabled = false;
            btn.textContent = '📡 Scan WiFi Networks';
            status.innerHTML = '';
            isScanning = false;
        }, 2000);
    }, 3000);
}

function togglePassword() {
    const input = document.getElementById('password');
    const btn = event.target;

    if (input.type === 'password') {
        input.type = 'text';
        btn.textContent = 'HIDE';
    } else {
        input.type = 'password';
        btn.textContent = 'SHOW';
    }
}

function showMessage(text, type) {
    const msg = document.getElementById('message');
    msg.textContent = text;
    msg.className = 'message ' + type;
    msg.style.display = 'block';
}

function submitForm(event) {
    event.preventDefault();

    const ssid = document.getElementById('ssid').value.trim();
    const password = document.getElementById('password').value;

    if (ssid.length === 0) {
        showMessage('Please enter a valid network name', 'error');
        return;
    }

    if (password.length < 8) {
        showMessage('Password must be at least 8 characters', 'error');
        return;
    }

    const submitBtn = document.getElementById('submitBtn');
    submitBtn.disabled = true;
    submitBtn.textContent = '⏳ Connecting...';

    // Send credentials to ESP32
    fetch('/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password)
    })
        .then(response => response.text())
        .then(data => {
            submitBtn.textContent = '✓ Saved!';
            showMessage('✓ Credentials saved! Device will restart and connect...', 'success');
            setTimeout(() => {
                window.location.href = '/success.html';
            }, 2000);
        })
        .catch(error => {
            showMessage('✗ Failed to save credentials. Please try again.', 'error');
            submitBtn.disabled = false;
            submitBtn.textContent = '✓ Connect to WiFi';
        });
}
