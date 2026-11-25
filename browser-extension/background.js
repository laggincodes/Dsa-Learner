const BACKEND_URL = "http://127.0.0.1:8080/api/extension/submission";

chrome.runtime.onMessage.addListener((msg, _sender, sendResponse) => {
  if (!msg || msg.type !== "LEETCODE_ACCEPTED") {
    return false;
  }

  fetch(BACKEND_URL, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(msg.payload),
  })
    .then(async (res) => {
      const data = await res.json();
      const record = {
        ...msg.payload,
        syncedOk: res.ok && data.ok === true,
        error: res.ok ? "" : (data.error || `HTTP ${res.status}`),
      };
      await chrome.storage.local.set({ lastSubmission: record });
      sendResponse({ ok: record.syncedOk, data });
    })
    .catch(async (err) => {
      const record = {
        ...msg.payload,
        syncedOk: false,
        error: err.message || "Local backend unreachable",
      };
      await chrome.storage.local.set({ lastSubmission: record });
      sendResponse({ ok: false, error: record.error });
    });

  return true;
});
