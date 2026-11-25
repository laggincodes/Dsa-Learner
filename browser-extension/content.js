(() => {
  "use strict";

  let lastSentSignature = "";
  let lastSentAt = 0;

  function extractSlug() {
    const match = window.location.pathname.match(/^\/problems\/([^/]+)/);
    return match ? match[1] : "";
  }

  function extractProblemMeta() {
    const slug = extractSlug();
    let problemId = 0;
    let title = "";
    let difficulty = "";

    const headingEl =
      document.querySelector('[data-cy="question-title"]') ||
      document.querySelector('a[href^="/problems/' + slug + '"]') ||
      document.querySelector("div.text-title-large");

    const rawTitle = headingEl ? headingEl.textContent.trim() : document.title.replace(" - LeetCode", "").trim();
    const idTitleMatch = rawTitle.match(/^(\d+)\.\s*(.+)$/);
    if (idTitleMatch) {
      problemId = parseInt(idTitleMatch[1], 10);
      title = idTitleMatch[2].trim();
    } else {
      title = rawTitle;
    }

    const bodyText = document.body ? document.body.innerText : "";
    if (/\bEasy\b/.test(bodyText)) difficulty = "Easy";
    else if (/\bHard\b/.test(bodyText)) difficulty = "Hard";
    else if (/\bMedium\b/.test(bodyText)) difficulty = "Medium";

    return { problemId, title, slug, difficulty };
  }

  function formatLocalTimestamp(date) {
    const pad = (n) => String(n).padStart(2, "0");
    return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())} ` +
           `${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(date.getSeconds())}`;
  }

  function checkForAcceptedVerdict() {
    const resultEl = document.querySelector('[data-e2e-locator="submission-result"]');
    if (!resultEl) return;
    const verdict = (resultEl.textContent || "").trim();
    if (verdict !== "Accepted") return;

    const meta = extractProblemMeta();
    if (!meta.slug) return;

    const now = Date.now();
    const sig = `${meta.slug}`;
    if (sig === lastSentSignature && now - lastSentAt < 15000) {
      return;
    }

    lastSentSignature = sig;
    lastSentAt = now;

    chrome.runtime.sendMessage({
      type: "LEETCODE_ACCEPTED",
      payload: {
        problemId: meta.problemId,
        title: meta.title,
        slug: meta.slug,
        difficulty: meta.difficulty,
        timestamp: formatLocalTimestamp(new Date(now)),
      },
    });
  }

  const observer = new MutationObserver(() => {
    checkForAcceptedVerdict();
  });

  observer.observe(document.documentElement, {
    childList: true,
    subtree: true,
    characterData: true,
  });
})();
