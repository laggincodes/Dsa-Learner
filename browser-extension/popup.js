(async () => {
  const backendEl = document.getElementById("backendState");
  const lastSubEl = document.getElementById("lastSubBox");

  try {
    const res = await fetch("http://127.0.0.1:8080/api/extension/status");
    const data = await res.json();
    if (res.ok && data.ok) {
      backendEl.innerHTML = `<span class="ok">Backend Online</span><div class="muted">Events captured: ${data.extensionEventsCount || 0}</div>`;
    } else {
      backendEl.innerHTML = `<span class="err">Backend Error</span>`;
    }
  } catch {
    backendEl.innerHTML = `<span class="err">Backend Offline</span><div class="muted">Start dsa_server on port 8080</div>`;
  }

  const stored = await chrome.storage.local.get("lastSubmission");
  if (stored.lastSubmission) {
    const s = stored.lastSubmission;
    lastSubEl.innerHTML = `
      <div><strong>${s.title || s.slug}</strong></div>
      <div class="muted">${s.timestamp} · ${s.syncedOk ? "Synced" : s.error}</div>
    `;
  }
})();
