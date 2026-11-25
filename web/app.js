(() => {
  "use strict";

  const state = {
    algorithms: [],
    progress: null,
    sort: {
      array: [42, 18, 73, 9, 55, 31, 64, 14, 88, 27, 49, 12],
      steps: [],
      stepIndex: 0,
      playing: false,
      timerId: null,
    },
    search: {
      array: [8, 14, 19, 27, 35, 42, 56, 63, 74, 88, 95],
      steps: [],
      stepIndex: 0,
      playing: false,
      timerId: null,
    },
    bst: {
      nodes: [],
      timerId: null,
    },
    graph: {
      snapshot: { directed: false, vertices: [], edges: [] },
      steps: [],
      stepIndex: 0,
      playing: false,
      timerId: null,
    },
    comparator: {
      results: [],
      metric: "time",
    },
    quiz: {
      sessionId: null,
      questions: [],
      index: 0,
      score: 0,
      answered: false,
    },
    codeEntries: [],
    activeCodeId: "bubble",
    ecosystem: null,
    roadmapStageFilter: "all",
    selectedPracticeId: null,
    flashcardCategory: "all",
    flashcardIndex: 0,
    flashcardRevealed: false,
    library: {
      page: 1,
      pageSize: 50,
      totalPages: 1,
      items: [],
      topics: [],
    },
    activeProblemSlug: null,
    notesPreview: false,
    notesTimer: null,
  };

  const VIEW_META = {
    dashboard: ["Overview", "C++17 Learning Workspace"],
    library: ["Question Library", "Complete LeetCode Catalogue, Filters & Status Tracker"],
    "problem-detail": ["Problem Detail", "Study Notes, Solutions & Submission History"],
    topics: ["Topic Progress", "Automatic Per-Topic Completion & Difficulty Breakdown"],
    solutions: ["Solution Library", "Searchable Multi-Approach C++ Solutions"],
    roadmap: ["DSA Roadmap", "Structured Curriculum across Foundations, Structures & Algorithms"],
    practice: ["Practice Center", "Curated Problem Library, Hints, Approaches & Notes"],
    leetcode: ["LeetCode Connect", "Public Profile Sync, Extension & Submission History"],
    revision: ["Revision Queue", "Spaced Repetition, Needs Revision & Saved Mistakes"],
    resources: ["Resources", "External DSA Platforms & C++ Documentation"],
    analytics: ["Learning Analytics", "Real Statistics Derived from Saved Activity"],
    sorting: ["Sorting Lab", "Interactive Bar Trace & Telemetry"],
    searching: ["Searching Lab", "Linear & Binary Interval Inspection"],
    datastructures: ["Data Structures", "Stack, Queue, Linked List & Binary Search Tree"],
    graph: ["Graph Lab", "Adjacency Lists with C++ BFS & DFS"],
    comparator: ["Complexity Lab", "Measured Microsecond Benchmarks vs Big-O"],
    quiz: ["Quiz Arena", "Randomized DSA Concept Assessment"],
    progress: ["Learning Progress", "Persistent Local Mastery Tracker"],
    codeviewer: ["Code Library", "C++17 Source, Pseudocode & Complexity Matrix"],
    about: ["About", "Project Overview, Architecture & Release Information"],
  };

  const MODULE_DESTINATIONS = {
    sorting: { view: "sorting", desc: "Animate Bubble, Selection, and Insertion Sort step by step." },
    searching: { view: "searching", desc: "Trace Linear and Binary Search with L/M/H pointers." },
    datastructures: { view: "datastructures", desc: "Experiment with Stack, Queue, Linked List, and BST." },
    bst: { view: "datastructures", dsTab: "bst", desc: "Insert, search, delete, and traverse Binary Search Trees." },
    graph: { view: "graph", desc: "Build unweighted graphs and step through BFS and DFS." },
    comparator: { view: "comparator", desc: "Benchmark sorting runtime, comparisons, and swaps in C++." },
    quiz: { view: "quiz", desc: "Test your DSA knowledge with instant C++ feedback." },
    codeviewer: { view: "codeviewer", desc: "Review C++17 implementations, pseudocode, and Big-O." },
  };

  // =========================================================================
  // Core Helpers
  // =========================================================================

  async function api(path, options = {}) {
    const res = await fetch(path, {
      headers: { "Content-Type": "application/json" },
      ...options,
    });
    const data = await res.json();
    if (!res.ok || data.ok === false) {
      const err = new Error(data.error || `HTTP ${res.status}`);
      err.code = data.code || "ERROR";
      throw err;
    }
    return data;
  }

  function toast(message, isError = false) {
    const box = document.getElementById("toastContainer");
    const el = document.createElement("div");
    el.className = `toast${isError ? " error" : ""}`;
    el.textContent = message;
    box.appendChild(el);
    setTimeout(() => el.remove(), 3000);
  }

  function confirmAction(title, message) {
    return new Promise((resolve) => {
      const modal = document.getElementById("confirmModal");
      document.getElementById("confirmModalTitle").textContent = title;
      document.getElementById("confirmModalMessage").textContent = message;
      modal.style.display = "grid";

      const okBtn = document.getElementById("confirmOkBtn");
      const cancelBtn = document.getElementById("confirmCancelBtn");
      const cleanup = (result) => {
        modal.style.display = "none";
        okBtn.removeEventListener("click", onOk);
        cancelBtn.removeEventListener("click", onCancel);
        resolve(result);
      };
      const onOk = () => cleanup(true);
      const onCancel = () => cleanup(false);
      okBtn.addEventListener("click", onOk);
      cancelBtn.addEventListener("click", onCancel);
    });
  }

  function highlightCpp(raw) {
    const src = String(raw || "");
    const tokenRe = /(\/\/[^\n]*|"(?:\\.|[^"\\])*"|\b(?:void|int|bool|char|double|float|long|const|auto|for|while|if|else|return|break|continue|class|struct|public|private|explicit|virtual|override|using|namespace|true|false|nullptr|new|delete)\b|\b(?:std|vector|string|stack|queue|set|map|unordered_map|size_t|unique_ptr|Node|AdjList)\b|\b\d+\b)/g;
    let out = "";
    let last = 0;
    let m;
    while ((m = tokenRe.exec(src)) !== null) {
      out += escapeHtml(src.slice(last, m.index));
      const tok = m[0];
      if (tok.startsWith("//")) out += `<span class="tok-com">${escapeHtml(tok)}</span>`;
      else if (tok.startsWith('"')) out += `<span class="tok-str">${escapeHtml(tok)}</span>`;
      else if (/^\d+$/.test(tok)) out += `<span class="tok-num">${tok}</span>`;
      else if (/^(std|vector|string|stack|queue|set|map|unordered_map|size_t|unique_ptr|Node|AdjList)$/.test(tok)) {
        out += `<span class="tok-type">${tok}</span>`;
      } else {
        out += `<span class="tok-kw">${tok}</span>`;
      }
      last = tokenRe.lastIndex;
    }
    out += escapeHtml(src.slice(last));
    return out;
  }

  function parseIntegerCsv(raw) {
    const parts = raw
      .split(",")
      .map((s) => s.trim())
      .filter((s) => s.length > 0);
    if (parts.length === 0) {
      throw new Error("Enter at least one integer.");
    }
    return parts.map((p) => {
      if (!/^-?\d+$/.test(p)) {
        throw new Error(`Invalid integer: "${p}"`);
      }
      return parseInt(p, 10);
    });
  }

  function switchDsTab(tabName) {
    document.querySelectorAll("#dsTabs .seg-btn").forEach((b) => {
      b.classList.toggle("active", b.dataset.dsTab === tabName);
    });
    document.querySelectorAll(".ds-panel").forEach((p) => {
      p.classList.toggle("active", p.id === `ds-tab-${tabName}`);
    });
  }

  function switchView(viewName, dsTab = null) {
    if (viewName === "bst") {
      viewName = "datastructures";
      dsTab = dsTab || "bst";
    }
    document.querySelectorAll(".nav-item").forEach((btn) => {
      btn.classList.toggle("active", btn.dataset.view === viewName);
    });
    document.querySelectorAll(".view").forEach((v) => {
      v.classList.toggle("active", v.id === `view-${viewName}`);
    });

    const [title, subtitle] = VIEW_META[viewName] || ["DSA Manager", ""];
    document.getElementById("viewTitle").textContent = title;
    document.getElementById("viewSubtitle").textContent = subtitle;
    document.getElementById("sidebar").classList.remove("open");

    if (dsTab) {
      switchDsTab(dsTab);
    }

    if (viewName === "dashboard") {
      loadHistory();
      loadProgress();
      loadEcosystem();
      loadIntelligenceAnalytics();
    } else if (viewName === "progress") {
      loadProgress();
    } else if (viewName === "library") {
      loadLibrary();
    } else if (viewName === "topics") {
      loadTopicsProgress();
    } else if (viewName === "solutions") {
      loadSolutionsLibrary();
    } else if (viewName === "leetcode") {
      loadLeetCodeProfile();
      if (!state.ecosystem) loadEcosystem();
    } else if (viewName === "revision") {
      loadIntelRevisionQueue();
      if (!state.ecosystem) loadEcosystem();
    } else if (viewName === "analytics") {
      loadIntelligenceAnalytics();
      if (!state.ecosystem) loadEcosystem();
    } else if (viewName === "roadmap" || viewName === "practice" || viewName === "resources") {
      if (!state.ecosystem) loadEcosystem();
    } else if (viewName === "codeviewer" && state.codeEntries.length === 0) {
      loadCodeViewer();
    }
  }

  function renderComplexityPanel(targetId, algoId) {
    const container = document.getElementById(targetId);
    const info = state.algorithms.find((a) => a.id === algoId);
    if (!info) {
      container.innerHTML = "";
      return;
    }
    container.innerHTML = `
      <div class="block-head">
        <h4>${info.name}</h4>
        <span class="mono-dim">${info.category}</span>
      </div>
      <p class="muted-text" style="margin-top: 6px;">${info.description}</p>
      <div class="complexity-grid">
        <div class="cx-box"><span>Best Time</span><strong>${info.bestTime}</strong></div>
        <div class="cx-box"><span>Average Time</span><strong>${info.averageTime}</strong></div>
        <div class="cx-box"><span>Worst Time</span><strong>${info.worstTime}</strong></div>
        <div class="cx-box"><span>Space</span><strong>${info.spaceComplexity}</strong></div>
      </div>
      <p class="muted-text" style="margin-top: 8px; font-size: 0.8rem;">${info.notes}</p>
    `;
  }

  // =========================================================================
  // Overview, Activity & Code Library Reference
  // =========================================================================

  async function loadOverview() {
    const statusPill = document.getElementById("serverStatus");
    try {
      const data = await api("/api/overview");
      state.algorithms = data.algorithms || [];
      document.getElementById("statAlgoCount").textContent = `${state.algorithms.length} modules`;

      statusPill.classList.remove("offline");
      statusPill.classList.add("online");
      statusPill.querySelector(".server-label").textContent = "C++17 Online";

      renderReference("all");
      renderComplexityPanel("sortComplexityPanel", document.getElementById("sortAlgoSelect").value);
      renderComplexityPanel("searchComplexityPanel", document.getElementById("searchAlgoSelect").value);
    } catch (err) {
      statusPill.classList.remove("online");
      statusPill.classList.add("offline");
      statusPill.querySelector(".server-label").textContent = "Disconnected";
      toast(`Backend error: ${err.message}`, true);
    }
  }

  async function loadHistory() {
    try {
      const data = await api("/api/history");
      const records = data.records || [];
      document.getElementById("statHistoryCount").textContent = records.length;

      const list = document.getElementById("historyList");
      if (records.length === 0) {
        list.innerHTML = `<p class="empty-muted">No operations logged yet.</p>`;
        return;
      }

      list.innerHTML = records
        .slice(0, 8)
        .map(
          (r) => `
        <div class="history-row">
          <div class="history-meta mono">
            <span>${r.category} · ${r.operation}</span>
            <span>${r.timestamp}</span>
          </div>
          <div>${r.summary}</div>
        </div>`
        )
        .join("");
    } catch (err) {
      toast(err.message, true);
    }
  }

  function renderReference(filter) {
    const grid = document.getElementById("referenceGrid");
    const items = filter === "all" ? state.algorithms : state.algorithms.filter((a) => a.category === filter);
    grid.innerHTML = items
      .map(
        (a) => `
      <div class="ref-card">
        <div class="block-head">
          <strong>${a.name}</strong>
          <span class="mono-dim">${a.category}</span>
        </div>
        <p class="muted-text">${a.description}</p>
        <div class="complexity-grid">
          <div class="cx-box"><span>Best</span><strong>${a.bestTime}</strong></div>
          <div class="cx-box"><span>Average</span><strong>${a.averageTime}</strong></div>
          <div class="cx-box"><span>Worst</span><strong>${a.worstTime}</strong></div>
          <div class="cx-box"><span>Space</span><strong>${a.spaceComplexity}</strong></div>
        </div>
        <p class="muted-text" style="font-size: 0.78rem;">${a.notes}</p>
      </div>`
      )
      .join("");
  }

  // =========================================================================
  // Sorting Lab
  // =========================================================================

  function renderSortBars(arr, step = null) {
    const stage = document.getElementById("sortBarsStage");
    if (!arr || arr.length === 0) {
      stage.innerHTML = "";
      return;
    }

    const minVal = Math.min(...arr, 0);
    const maxVal = Math.max(...arr, 1);
    const span = Math.max(maxVal - minVal, 1);
    const sortedSet = new Set(step ? step.sortedIndices : []);

    stage.innerHTML = arr
      .map((val, idx) => {
        let cls = "bar-col";
        if (sortedSet.has(idx)) cls += " sorted";
        if (step && (idx === step.indexA || idx === step.indexB)) {
          if (step.action === "swap") cls += " swapping";
          else if (step.action === "compare") cls += " comparing";
        }
        const pct = Math.max(8, Math.round(((val - minVal) / span) * 88) + 8);
        return `
          <div class="${cls}">
            <div class="bar-fill" style="height: ${pct}%"></div>
            <span class="bar-val">${val}</span>
          </div>`;
      })
      .join("");
  }

  function stopSortPlayback() {
    if (state.sort.timerId) {
      clearTimeout(state.sort.timerId);
      state.sort.timerId = null;
    }
    state.sort.playing = false;
    document.getElementById("sortStartBtn").disabled = false;
    document.getElementById("sortPauseBtn").disabled = true;
  }

  function applySortFrame(index) {
    const steps = state.sort.steps;
    if (!steps.length) {
      renderSortBars(state.sort.array);
      document.getElementById("sortComparisons").textContent = "0";
      document.getElementById("sortSwaps").textContent = "0";
      document.getElementById("sortStepCounter").textContent = "0 / 0";
      return;
    }
    const clamped = Math.max(0, Math.min(index, steps.length - 1));
    state.sort.stepIndex = clamped;
    const step = steps[clamped];
    renderSortBars(step.array, step);
    document.getElementById("sortComparisons").textContent = step.comparisons;
    document.getElementById("sortSwaps").textContent = step.swaps;
    document.getElementById("sortStepCounter").textContent = `${clamped + 1} / ${steps.length}`;
    document.getElementById("sortNarration").textContent = step.detail;
  }

  function scheduleSortTick() {
    if (!state.sort.playing) return;
    if (state.sort.stepIndex >= state.sort.steps.length - 1) {
      stopSortPlayback();
      return;
    }
    const delay = parseInt(document.getElementById("sortSpeedInput").value, 10);
    state.sort.timerId = setTimeout(() => {
      applySortFrame(state.sort.stepIndex + 1);
      scheduleSortTick();
    }, delay);
  }

  async function ensureSortTrace() {
    if (state.sort.steps.length > 0 && state.sort.stepIndex < state.sort.steps.length - 1) {
      return true;
    }
    const algo = document.getElementById("sortAlgoSelect").value;
    const data = await api("/api/sort", {
      method: "POST",
      body: JSON.stringify({ algorithm: algo, array: state.sort.array }),
    });
    state.sort.steps = data.steps;
    state.sort.stepIndex = 0;
    applySortFrame(0);
    return true;
  }

  function initSortHandlers() {
    const sizeInput = document.getElementById("sortSizeInput");
    const speedInput = document.getElementById("sortSpeedInput");
    const algoSelect = document.getElementById("sortAlgoSelect");

    document.querySelectorAll("#sortAlgoTabs .seg-btn").forEach((btn) => {
      btn.addEventListener("click", () => {
        document.querySelectorAll("#sortAlgoTabs .seg-btn").forEach((b) => b.classList.remove("active"));
        btn.classList.add("active");
        algoSelect.value = btn.dataset.sortAlgo;
        stopSortPlayback();
        state.sort.steps = [];
        state.sort.stepIndex = 0;
        applySortFrame(0);
        renderComplexityPanel("sortComplexityPanel", algoSelect.value);
      });
    });

    sizeInput.addEventListener("input", () => {
      document.getElementById("sortSizeLabel").textContent = sizeInput.value;
    });

    speedInput.addEventListener("input", () => {
      document.getElementById("sortSpeedLabel").textContent = `${speedInput.value} ms`;
    });

    document.getElementById("sortRandomBtn").addEventListener("click", async () => {
      stopSortPlayback();
      try {
        const size = parseInt(sizeInput.value, 10);
        const data = await api("/api/array/generate", {
          method: "POST",
          body: JSON.stringify({ size, min: 10, max: 99 }),
        });
        state.sort.array = data.array;
        state.sort.steps = [];
        state.sort.stepIndex = 0;
        document.getElementById("sortCustomInput").value = data.array.join(", ");
        applySortFrame(0);
        document.getElementById("sortNarration").textContent = "Generated new random array in C++.";
      } catch (err) {
        toast(err.message, true);
      }
    });

    document.getElementById("sortApplyCustomBtn").addEventListener("click", () => {
      stopSortPlayback();
      try {
        const nums = parseIntegerCsv(document.getElementById("sortCustomInput").value);
        if (nums.length < 2 || nums.length > 60) {
          throw new Error("Array must have 2 to 60 integers.");
        }
        state.sort.array = nums;
        state.sort.steps = [];
        state.sort.stepIndex = 0;
        applySortFrame(0);
        document.getElementById("sortNarration").textContent = "Loaded custom array.";
      } catch (err) {
        toast(err.message, true);
      }
    });

    document.getElementById("sortStartBtn").addEventListener("click", async () => {
      try {
        await ensureSortTrace();
        state.sort.playing = true;
        document.getElementById("sortStartBtn").disabled = true;
        document.getElementById("sortPauseBtn").disabled = false;
        scheduleSortTick();
      } catch (err) {
        toast(err.message, true);
      }
    });

    document.getElementById("sortPauseBtn").addEventListener("click", stopSortPlayback);

    document.getElementById("sortStepBtn").addEventListener("click", async () => {
      stopSortPlayback();
      try {
        if (state.sort.steps.length === 0) {
          await ensureSortTrace();
          return;
        }
        if (state.sort.stepIndex < state.sort.steps.length - 1) {
          applySortFrame(state.sort.stepIndex + 1);
        }
      } catch (err) {
        toast(err.message, true);
      }
    });

    document.getElementById("sortResetBtn").addEventListener("click", () => {
      stopSortPlayback();
      state.sort.steps = [];
      state.sort.stepIndex = 0;
      applySortFrame(0);
      document.getElementById("sortNarration").textContent = "Reset to initial array.";
    });

    document.getElementById("sortCustomInput").value = state.sort.array.join(", ");
    renderSortBars(state.sort.array);
  }

  // =========================================================================
  // Searching Lab
  // =========================================================================

  function renderSearchTiles(arr, step = null) {
    const stage = document.getElementById("searchStage");
    stage.innerHTML = arr
      .map((val, idx) => {
        let cls = "search-tile";
        const ptrs = [];
        if (step) {
          if (step.low >= 0 && step.high >= 0 && (idx < step.low || idx > step.high)) {
            cls += " eliminated";
          }
          if (idx === step.index) {
            if (step.status === "found") cls += " found";
            else if (step.status !== "not_found") cls += " inspecting";
          }
          if (idx === step.low && step.status !== "found") ptrs.push("L");
          if (idx === step.mid && step.status !== "found") ptrs.push("M");
          if (idx === step.high && step.status !== "found") ptrs.push("H");
        }
        const ptrBadge = ptrs.length ? `<span class="tile-pointer">${ptrs.join("/")}</span>` : "";
        return `
          <div class="${cls}">
            ${ptrBadge}
            <span class="tile-val">${val}</span>
            <span class="tile-idx">[${idx}]</span>
          </div>`;
      })
      .join("");
  }

  function stopSearchPlayback() {
    if (state.search.timerId) {
      clearTimeout(state.search.timerId);
      state.search.timerId = null;
    }
    state.search.playing = false;
    document.getElementById("searchStartBtn").disabled = false;
    document.getElementById("searchPauseBtn").disabled = true;
  }

  function applySearchFrame(idx) {
    const steps = state.search.steps;
    if (!steps.length) {
      renderSearchTiles(state.search.array);
      document.getElementById("searchComparisons").textContent = "0";
      document.getElementById("searchResultBadge").textContent = "Ready";
      return;
    }
    const clamped = Math.max(0, Math.min(idx, steps.length - 1));
    state.search.stepIndex = clamped;
    const step = steps[clamped];
    renderSearchTiles(state.search.array, step);
    document.getElementById("searchComparisons").textContent = step.comparisons;
    document.getElementById("searchNarration").textContent = step.detail;
    if (step.status === "found") {
      document.getElementById("searchResultBadge").textContent = `Found [${step.index}]`;
    } else if (step.status === "not_found") {
      document.getElementById("searchResultBadge").textContent = "Not Found";
    } else {
      document.getElementById("searchResultBadge").textContent = "Inspecting";
    }
  }

  function scheduleSearchTick() {
    if (!state.search.playing) return;
    if (state.search.stepIndex >= state.search.steps.length - 1) {
      stopSearchPlayback();
      return;
    }
    const delay = parseInt(document.getElementById("searchSpeedInput").value, 10);
    state.search.timerId = setTimeout(() => {
      applySearchFrame(state.search.stepIndex + 1);
      scheduleSearchTick();
    }, delay);
  }

  function initSearchHandlers() {
    const algoSelect = document.getElementById("searchAlgoSelect");
    const speedInput = document.getElementById("searchSpeedInput");

    document.querySelectorAll("#searchAlgoTabs .seg-btn").forEach((btn) => {
      btn.addEventListener("click", () => {
        document.querySelectorAll("#searchAlgoTabs .seg-btn").forEach((b) => b.classList.remove("active"));
        btn.classList.add("active");
        algoSelect.value = btn.dataset.searchAlgo;
        stopSearchPlayback();
        state.search.steps = [];
        applySearchFrame(0);
        renderComplexityPanel("searchComplexityPanel", algoSelect.value);
      });
    });

    speedInput.addEventListener("input", () => {
      document.getElementById("searchSpeedLabel").textContent = `${speedInput.value} ms`;
    });

    document.getElementById("searchRandomBtn").addEventListener("click", async () => {
      stopSearchPlayback();
      try {
        const data = await api("/api/array/generate", {
          method: "POST",
          body: JSON.stringify({ size: 12, min: 5, max: 95 }),
        });
        let arr = data.array;
        if (algoSelect.value === "binary") {
          const sorted = await api("/api/sort", {
            method: "POST",
            body: JSON.stringify({ algorithm: "insertion", array: arr }),
          });
          arr = sorted.sortedArray;
        }
        state.search.array = arr;
        state.search.steps = [];
        document.getElementById("searchCustomInput").value = arr.join(", ");
        applySearchFrame(0);
        document.getElementById("searchNarration").textContent = "Generated new array.";
      } catch (err) {
        toast(err.message, true);
      }
    });

    document.getElementById("searchSortArrayBtn").addEventListener("click", async () => {
      stopSearchPlayback();
      try {
        const res = await api("/api/sort", {
          method: "POST",
          body: JSON.stringify({ algorithm: "insertion", array: state.search.array }),
        });
        state.search.array = res.sortedArray;
        state.search.steps = [];
        document.getElementById("searchCustomInput").value = res.sortedArray.join(", ");
        applySearchFrame(0);
        document.getElementById("searchNarration").textContent = "Sorted ascending via C++ Insertion Sort.";
      } catch (err) {
        toast(err.message, true);
      }
    });

    document.getElementById("searchApplyCustomBtn").addEventListener("click", () => {
      stopSearchPlayback();
      try {
        const nums = parseIntegerCsv(document.getElementById("searchCustomInput").value);
        if (nums.length < 1 || nums.length > 60) {
          throw new Error("Array must have 1 to 60 integers.");
        }
        state.search.array = nums;
        state.search.steps = [];
        applySearchFrame(0);
        document.getElementById("searchNarration").textContent = "Loaded custom search array.";
      } catch (err) {
        toast(err.message, true);
      }
    });

    document.getElementById("searchStartBtn").addEventListener("click", async () => {
      try {
        if (state.search.steps.length === 0 || state.search.stepIndex >= state.search.steps.length - 1) {
          const targetRaw = document.getElementById("searchTargetInput").value.trim();
          if (!/^-?\d+$/.test(targetRaw)) {
            throw new Error("Target must be a valid integer.");
          }
          const data = await api("/api/search", {
            method: "POST",
            body: JSON.stringify({
              algorithm: algoSelect.value,
              array: state.search.array,
              target: parseInt(targetRaw, 10),
              autoSort: document.getElementById("searchAutoSortCheck").checked,
            }),
          });
          state.search.array = data.arrayUsed;
          document.getElementById("searchCustomInput").value = data.arrayUsed.join(", ");
          state.search.steps = data.steps;
          state.search.stepIndex = 0;
          applySearchFrame(0);
        }
        state.search.playing = true;
        document.getElementById("searchStartBtn").disabled = true;
        document.getElementById("searchPauseBtn").disabled = false;
        scheduleSearchTick();
      } catch (err) {
        toast(err.message, true);
        document.getElementById("searchNarration").textContent = err.message;
      }
    });

    document.getElementById("searchPauseBtn").addEventListener("click", stopSearchPlayback);

    document.getElementById("searchResetBtn").addEventListener("click", () => {
      stopSearchPlayback();
      state.search.steps = [];
      state.search.stepIndex = 0;
      applySearchFrame(0);
      document.getElementById("searchNarration").textContent = "Search state reset.";
    });

    document.getElementById("searchCustomInput").value = state.search.array.join(", ");
    renderSearchTiles(state.search.array);
  }

  // =========================================================================
  // Data Structures (Stack, Queue, Linked List, BST)
  // =========================================================================

  function renderStack(ds) {
    document.getElementById("stackSizeText").textContent = `${ds.size} / ${ds.capacity}`;
    document.getElementById("stackTopText").textContent = ds.hasTopOrFront ? ds.topOrFrontValue : "Empty";
    document.getElementById("stackCapSelect").value = String(ds.capacity);

    const well = document.getElementById("stackVisual");
    if (ds.elements.length === 0) {
      well.innerHTML = `<p class="empty-muted" style="text-align:center; margin:auto;">Stack empty</p>`;
      return;
    }
    well.innerHTML = ds.elements
      .map((val, idx) => {
        const isTop = idx === ds.elements.length - 1;
        return `
          <div class="stack-item${isTop ? " is-top" : ""}">
            <span>${val}</span>
            ${isTop ? `<span class="item-tag" style="margin:0;">TOP</span>` : `<span class="mono-dim">[${idx}]</span>`}
          </div>`;
      })
      .join("");
  }

  function renderQueue(ds) {
    document.getElementById("queueSizeText").textContent = `${ds.size} / ${ds.capacity}`;
    document.getElementById("queueFrontText").textContent = ds.hasTopOrFront ? ds.topOrFrontValue : "Empty";
    document.getElementById("queueCapSelect").value = String(ds.capacity);

    const lane = document.getElementById("queueVisual");
    if (ds.elements.length === 0) {
      lane.innerHTML = `<p class="empty-muted" style="margin:auto;">Queue empty</p>`;
      return;
    }
    lane.innerHTML = ds.elements
      .map((val, idx) => {
        const labels = [];
        if (idx === 0) labels.push("FRONT");
        if (idx === ds.elements.length - 1) labels.push("REAR");
        return `
          <div class="queue-item${idx === 0 ? " is-front" : ""}">
            ${labels.length ? `<span class="item-tag">${labels.join(" / ")}</span>` : `<span class="mono-dim">[${idx}]</span>`}
            <div>${val}</div>
          </div>`;
      })
      .join("");
  }

  function renderLinkedList(ds, highlightIndex = -1) {
    document.getElementById("listSizeText").textContent = `${ds.size} / ${ds.capacity}`;
    document.getElementById("listHeadText").textContent = ds.hasTopOrFront ? ds.topOrFrontValue : "NULL";

    const stage = document.getElementById("listVisual");
    if (!ds.elements || ds.elements.length === 0) {
      stage.innerHTML = `<span class="ll-null">HEAD -&gt; NULL</span>`;
      return;
    }

    const nodesHtml = ds.elements
      .map((val, idx) => {
        const headPrefix = idx === 0 ? `<span class="ll-head-badge">HEAD</span>` : "";
        const hi = idx === highlightIndex ? " highlighted" : "";
        return `
          <div class="ll-node-wrap">
            ${headPrefix}
            <div class="ll-node${hi}">
              <div class="ll-data">${val}</div>
              <div class="ll-next">next</div>
            </div>
            <span class="ll-arrow">--&gt;</span>
          </div>`;
      })
      .join("");

    stage.innerHTML = `${nodesHtml}<span class="ll-null">NULL</span>`;
  }

  function setDsFeedback(elementId, panelId, message, isError = false) {
    const el = document.getElementById(elementId);
    el.textContent = message;
    el.classList.toggle("error", isError);
    if (isError) {
      const panel = document.getElementById(panelId);
      panel.classList.remove("shake");
      void panel.offsetWidth;
      panel.classList.add("shake");
    }
  }

  async function loadDsState() {
    try {
      const data = await api("/api/ds/state");
      renderStack(data.stack);
      renderQueue(data.queue);
      if (data.list) renderLinkedList(data.list);
    } catch (err) {
      toast(err.message, true);
    }
  }

  function initDsHandlers() {
    document.querySelectorAll("#dsTabs .seg-btn").forEach((btn) => {
      btn.addEventListener("click", () => switchDsTab(btn.dataset.dsTab));
    });

    const runStackOp = async (payload) => {
      try {
        const data = await api("/api/ds/stack", { method: "POST", body: JSON.stringify(payload) });
        renderStack(data.stack);
        setDsFeedback("stackFeedback", "ds-tab-stack", data.message, false);
      } catch (err) {
        setDsFeedback("stackFeedback", "ds-tab-stack", err.message, true);
        toast(err.message, true);
      }
    };

    const runQueueOp = async (payload) => {
      try {
        const data = await api("/api/ds/queue", { method: "POST", body: JSON.stringify(payload) });
        renderQueue(data.queue);
        setDsFeedback("queueFeedback", "ds-tab-queue", data.message, false);
      } catch (err) {
        setDsFeedback("queueFeedback", "ds-tab-queue", err.message, true);
        toast(err.message, true);
      }
    };

    const runListOp = async (payload) => {
      try {
        const data = await api("/api/ds/list", { method: "POST", body: JSON.stringify(payload) });
        renderLinkedList(data.list, data.highlightIndex ?? -1);
        setDsFeedback("listFeedback", "ds-tab-list", data.message, false);
      } catch (err) {
        setDsFeedback("listFeedback", "ds-tab-list", err.message, true);
        toast(err.message, true);
      }
    };

    const readIntField = (id) => {
      const raw = document.getElementById(id).value.trim();
      if (!/^-?\d+$/.test(raw)) throw new Error("Enter a valid integer.");
      return parseInt(raw, 10);
    };

    document.getElementById("stackPushBtn").addEventListener("click", () => {
      try { runStackOp({ op: "push", value: readIntField("stackValueInput") }); }
      catch (e) { setDsFeedback("stackFeedback", "ds-tab-stack", e.message, true); }
    });
    document.getElementById("stackPopBtn").addEventListener("click", () => runStackOp({ op: "pop" }));
    document.getElementById("stackPeekBtn").addEventListener("click", () => runStackOp({ op: "peek" }));
    document.getElementById("stackClearBtn").addEventListener("click", async () => {
      if (await confirmAction("Clear Stack", "Remove all elements currently in the Stack?")) runStackOp({ op: "clear" });
    });
    document.getElementById("stackCapSelect").addEventListener("change", (e) => {
      runStackOp({ op: "set_capacity", capacity: parseInt(e.target.value, 10) });
    });

    document.getElementById("queueEnqueueBtn").addEventListener("click", () => {
      try { runQueueOp({ op: "enqueue", value: readIntField("queueValueInput") }); }
      catch (e) { setDsFeedback("queueFeedback", "ds-tab-queue", e.message, true); }
    });
    document.getElementById("queueDequeueBtn").addEventListener("click", () => runQueueOp({ op: "dequeue" }));
    document.getElementById("queueFrontBtn").addEventListener("click", () => runQueueOp({ op: "front" }));
    document.getElementById("queueClearBtn").addEventListener("click", async () => {
      if (await confirmAction("Clear Queue", "Remove all elements currently in the Queue?")) runQueueOp({ op: "clear" });
    });
    document.getElementById("queueCapSelect").addEventListener("change", (e) => {
      runQueueOp({ op: "set_capacity", capacity: parseInt(e.target.value, 10) });
    });

    document.getElementById("listInsertHeadBtn").addEventListener("click", () => {
      try { runListOp({ op: "insert_head", value: readIntField("listValueInput") }); }
      catch (e) { setDsFeedback("listFeedback", "ds-tab-list", e.message, true); }
    });
    document.getElementById("listInsertTailBtn").addEventListener("click", () => {
      try { runListOp({ op: "insert_tail", value: readIntField("listValueInput") }); }
      catch (e) { setDsFeedback("listFeedback", "ds-tab-list", e.message, true); }
    });
    document.getElementById("listSearchBtn").addEventListener("click", () => {
      try { runListOp({ op: "search", value: readIntField("listValueInput") }); }
      catch (e) { setDsFeedback("listFeedback", "ds-tab-list", e.message, true); }
    });
    document.getElementById("listDeleteValBtn").addEventListener("click", () => {
      try { runListOp({ op: "remove_value", value: readIntField("listValueInput") }); }
      catch (e) { setDsFeedback("listFeedback", "ds-tab-list", e.message, true); }
    });
    document.getElementById("listRemoveHeadBtn").addEventListener("click", () => runListOp({ op: "remove_head" }));
    document.getElementById("listSampleBtn").addEventListener("click", () => runListOp({ op: "sample" }));
    document.getElementById("listClearBtn").addEventListener("click", async () => {
      if (await confirmAction("Clear Linked List", "Remove all nodes from the Linked List?")) runListOp({ op: "clear" });
    });
  }

  // =========================================================================
  // BST Visualizer
  // =========================================================================

  function renderBstSvg(nodes, highlightValue = null, highlightAction = "") {
    const svg = document.getElementById("bstSvg");
    if (!nodes || nodes.length === 0) {
      svg.innerHTML = `<text x="430" y="180" fill="#5e6573" font-size="14" text-anchor="middle">BST empty. Insert keys or load sample.</text>`;
      return;
    }

    const maxOrder = Math.max(nodes.length - 1, 1);
    const maxDepth = Math.max(...nodes.map((n) => n.depth), 1);
    const padX = 56;
    const padY = 44;
    const width = 860 - padX * 2;
    const height = 360 - padY * 2;

    const coords = new Map();
    nodes.forEach((n) => {
      const x = nodes.length === 1 ? 430 : padX + (n.orderIndex / maxOrder) * width;
      const y = maxDepth === 0 ? 180 : padY + (n.depth / Math.max(maxDepth, 1)) * height;
      coords.set(n.value, { x, y });
    });

    const parts = [];
    nodes.forEach((n) => {
      const p = coords.get(n.value);
      if (n.hasLeft && coords.has(n.leftValue)) {
        const c = coords.get(n.leftValue);
        parts.push(`<line class="tree-edge" x1="${p.x}" y1="${p.y}" x2="${c.x}" y2="${c.y}" />`);
      }
      if (n.hasRight && coords.has(n.rightValue)) {
        const c = coords.get(n.rightValue);
        parts.push(`<line class="tree-edge" x1="${p.x}" y1="${p.y}" x2="${c.x}" y2="${c.y}" />`);
      }
    });

    nodes.forEach((n) => {
      const p = coords.get(n.value);
      let cls = "viz-node";
      if (highlightValue === n.value) {
        if (highlightAction === "found" || highlightAction === "insert" || highlightAction === "traverse") {
          cls += " found";
        } else if (highlightAction === "delete") {
          cls += " deleted";
        } else {
          cls += " active";
        }
      }
      parts.push(`
        <g class="${cls}">
          <circle cx="${p.x}" cy="${p.y}" r="20"></circle>
          <text x="${p.x}" y="${p.y}">${n.value}</text>
        </g>`);
    });

    svg.innerHTML = parts.join("");
  }

  function animateBstSteps(res) {
    if (state.bst.timerId) {
      clearTimeout(state.bst.timerId);
      state.bst.timerId = null;
    }
    state.bst.nodes = res.nodes || [];
    document.getElementById("bstNodeCount").textContent = res.nodeCount ?? 0;
    document.getElementById("bstHeight").textContent = res.height ?? 0;

    const strip = document.getElementById("bstTraversalStrip");
    strip.innerHTML = (res.traversalOrder || []).map((v) => `<span class="trav-chip">${v}</span>`).join("");

    const steps = res.steps || [];
    if (steps.length === 0) {
      renderBstSvg(state.bst.nodes);
      document.getElementById("bstNarration").textContent = res.message;
      return;
    }

    let idx = 0;
    function tick() {
      const st = steps[idx];
      renderBstSvg(state.bst.nodes, st.value, st.action);
      document.getElementById("bstNarration").textContent = st.detail;
      idx += 1;
      if (idx < steps.length) {
        state.bst.timerId = setTimeout(tick, 320);
      } else {
        document.getElementById("bstNarration").textContent = res.message;
      }
    }
    tick();
  }

  async function loadBstState() {
    try {
      const res = await api("/api/bst/state");
      state.bst.nodes = res.nodes || [];
      document.getElementById("bstNodeCount").textContent = res.nodeCount ?? 0;
      document.getElementById("bstHeight").textContent = res.height ?? 0;
      renderBstSvg(state.bst.nodes);
    } catch (err) {
      toast(err.message, true);
    }
  }

  function initBstHandlers() {
    const getVal = () => {
      const raw = document.getElementById("bstValueInput").value.trim();
      if (!/^-?\d+$/.test(raw)) throw new Error("Enter a valid integer key.");
      return parseInt(raw, 10);
    };

    const runOp = async (payload) => {
      try {
        const res = await api("/api/bst/op", { method: "POST", body: JSON.stringify(payload) });
        animateBstSteps(res);
      } catch (err) {
        toast(err.message, true);
        document.getElementById("bstNarration").textContent = err.message;
      }
    };

    document.getElementById("bstInsertBtn").addEventListener("click", () => {
      try { runOp({ op: "insert", value: getVal() }); } catch (e) { toast(e.message, true); }
    });
    document.getElementById("bstSearchBtn").addEventListener("click", () => {
      try { runOp({ op: "search", value: getVal() }); } catch (e) { toast(e.message, true); }
    });
    document.getElementById("bstDeleteBtn").addEventListener("click", () => {
      try { runOp({ op: "delete", value: getVal() }); } catch (e) { toast(e.message, true); }
    });
    document.getElementById("bstSampleBtn").addEventListener("click", () => runOp({ op: "sample" }));
    document.getElementById("bstClearBtn").addEventListener("click", async () => {
      if (await confirmAction("Clear BST", "Delete all nodes from the Binary Search Tree?")) runOp({ op: "clear" });
    });
    document.querySelectorAll("[data-bst-trav]").forEach((btn) => {
      btn.addEventListener("click", () => runOp({ op: "traverse", order: btn.dataset.bstTrav }));
    });
  }

  // =========================================================================
  // Graph Lab
  // =========================================================================

  function renderGraphSelects(vertices) {
    ["graphFromSelect", "graphToSelect", "graphStartSelect"].forEach((id, idx) => {
      const sel = document.getElementById(id);
      const prev = sel.value;
      sel.innerHTML = vertices.map((v) => `<option value="${v}">${v}</option>`).join("");
      if (vertices.includes(prev)) sel.value = prev;
      else if (vertices.length > idx && id === "graphToSelect") sel.value = vertices[1];
    });
  }

  function renderGraphSvg(snapshot, step = null) {
    const svg = document.getElementById("graphSvg");
    const vertices = snapshot.vertices || [];
    const edges = snapshot.edges || [];
    if (vertices.length === 0) {
      svg.innerHTML = `<text x="430" y="200" fill="#5e6573" font-size="14" text-anchor="middle">Graph empty. Add vertices or load sample.</text>`;
      return;
    }

    const cx = 430;
    const cy = 200;
    const rx = Math.min(310, 120 + vertices.length * 16);
    const ry = Math.min(150, 90 + vertices.length * 6);
    const pos = new Map();

    vertices.forEach((v, i) => {
      const angle = (2 * Math.PI * i) / vertices.length - Math.PI / 2;
      pos.set(v, {
        x: vertices.length === 1 ? cx : cx + rx * Math.cos(angle),
        y: vertices.length === 1 ? cy : cy + ry * Math.sin(angle),
      });
    });

    const visitedSet = new Set(step ? step.visited : []);
    const parts = [];

    if (snapshot.directed) {
      parts.push(`
        <defs>
          <marker id="arrowHead" markerWidth="10" markerHeight="7" refX="26" refY="3.5" orient="auto">
            <polygon points="0 0, 10 3.5, 0 7" fill="#8f96a3"></polygon>
          </marker>
        </defs>`);
    }

    edges.forEach((e) => {
      const p1 = pos.get(e.from);
      const p2 = pos.get(e.to);
      if (!p1 || !p2) return;
      const isActive =
        step &&
        ((step.edgeFrom === e.from && step.edgeTo === e.to) ||
          (!snapshot.directed && step.edgeFrom === e.to && step.edgeTo === e.from));
      const marker = snapshot.directed ? `marker-end="url(#arrowHead)"` : "";
      parts.push(`<line class="graph-edge${isActive ? " active" : ""}" x1="${p1.x}" y1="${p1.y}" x2="${p2.x}" y2="${p2.y}" ${marker} />`);
    });

    vertices.forEach((v) => {
      const p = pos.get(v);
      let cls = "viz-node";
      if (step && step.current === v) cls += " active";
      else if (visitedSet.has(v)) cls += " visited";
      parts.push(`
        <g class="${cls}">
          <circle cx="${p.x}" cy="${p.y}" r="21"></circle>
          <text x="${p.x}" y="${p.y}">${v}</text>
        </g>`);
    });

    svg.innerHTML = parts.join("");
  }

  function stopGraphPlayback() {
    if (state.graph.timerId) {
      clearTimeout(state.graph.timerId);
      state.graph.timerId = null;
    }
    state.graph.playing = false;
    document.getElementById("graphPauseBtn").disabled = true;
  }

  function applyGraphFrame(idx) {
    const steps = state.graph.steps;
    if (!steps.length) {
      renderGraphSvg(state.graph.snapshot);
      document.getElementById("graphFrontierText").textContent = "[]";
      document.getElementById("graphOrderText").textContent = "[]";
      return;
    }
    const clamped = Math.max(0, Math.min(idx, steps.length - 1));
    state.graph.stepIndex = clamped;
    const st = steps[clamped];
    renderGraphSvg(state.graph.snapshot, st);
    document.getElementById("graphFrontierText").textContent = `[${(st.frontier || []).join(", ")}]`;
    document.getElementById("graphOrderText").textContent = (st.visited || []).join(" -> ") || "[]";
    document.getElementById("graphNarration").textContent = st.detail;
  }

  function scheduleGraphTick() {
    if (!state.graph.playing) return;
    if (state.graph.stepIndex >= state.graph.steps.length - 1) {
      stopGraphPlayback();
      return;
    }
    state.graph.timerId = setTimeout(() => {
      applyGraphFrame(state.graph.stepIndex + 1);
      scheduleGraphTick();
    }, 450);
  }

  async function loadGraphState() {
    try {
      const res = await api("/api/graph/state");
      state.graph.snapshot = res.graph;
      document.getElementById("graphDirectedCheck").checked = Boolean(res.graph.directed);
      renderGraphSelects(res.graph.vertices || []);
      renderGraphSvg(res.graph);
    } catch (err) {
      toast(err.message, true);
    }
  }

  function initGraphHandlers() {
    const runGraphOp = async (payload) => {
      stopGraphPlayback();
      try {
        const res = await api("/api/graph/op", { method: "POST", body: JSON.stringify(payload) });
        state.graph.snapshot = res.graph;
        document.getElementById("graphDirectedCheck").checked = Boolean(res.graph.directed);
        renderGraphSelects(res.graph.vertices || []);
        if (res.traversal) {
          state.graph.steps = res.traversal.steps || [];
          state.graph.stepIndex = 0;
          state.graph.playing = true;
          document.getElementById("graphPauseBtn").disabled = false;
          applyGraphFrame(0);
          scheduleGraphTick();
        } else {
          state.graph.steps = [];
          applyGraphFrame(0);
          document.getElementById("graphNarration").textContent = res.message;
        }
      } catch (err) {
        toast(err.message, true);
        document.getElementById("graphNarration").textContent = err.message;
      }
    };

    document.getElementById("graphAddVertexBtn").addEventListener("click", () => {
      runGraphOp({ op: "add_vertex", vertex: document.getElementById("graphVertexInput").value });
    });
    document.getElementById("graphDelVertexBtn").addEventListener("click", () => {
      runGraphOp({ op: "remove_vertex", vertex: document.getElementById("graphVertexInput").value });
    });
    document.getElementById("graphAddEdgeBtn").addEventListener("click", () => {
      runGraphOp({
        op: "add_edge",
        from: document.getElementById("graphFromSelect").value,
        to: document.getElementById("graphToSelect").value,
      });
    });
    document.getElementById("graphDelEdgeBtn").addEventListener("click", () => {
      runGraphOp({
        op: "remove_edge",
        from: document.getElementById("graphFromSelect").value,
        to: document.getElementById("graphToSelect").value,
      });
    });
    document.getElementById("graphBfsBtn").addEventListener("click", () => {
      runGraphOp({ op: "bfs", start: document.getElementById("graphStartSelect").value });
    });
    document.getElementById("graphDfsBtn").addEventListener("click", () => {
      runGraphOp({ op: "dfs", start: document.getElementById("graphStartSelect").value });
    });
    document.getElementById("graphPauseBtn").addEventListener("click", stopGraphPlayback);
    document.getElementById("graphSampleBtn").addEventListener("click", () => runGraphOp({ op: "sample" }));
    document.getElementById("graphClearBtn").addEventListener("click", async () => {
      if (await confirmAction("Clear Graph", "Remove all vertices and edges from the Graph?")) runGraphOp({ op: "clear" });
    });
    document.getElementById("graphDirectedCheck").addEventListener("change", (e) => {
      runGraphOp({ op: "set_directed", directed: e.target.checked });
    });
  }

  // =========================================================================
  // Complexity Lab
  // =========================================================================

  function renderComparatorView() {
    const results = state.comparator.results;
    const metric = state.comparator.metric;
    const chart = document.getElementById("compChart");
    const tbody = document.getElementById("compTableBody");
    if (!results.length) return;

    const values = results.map((r) => {
      if (metric === "comparisons") return r.comparisons;
      if (metric === "swaps") return r.swaps;
      return r.elapsedMicroseconds;
    });
    const maxVal = Math.max(...values, 0.001);

    chart.innerHTML = results
      .map((r, i) => {
        const val = values[i];
        const pct = Math.max(4, Math.round((val / maxVal) * 100));
        const label = metric === "time" ? `${r.elapsedMicroseconds.toFixed(2)} µs` : val.toLocaleString();
        return `
          <div class="comp-bar-row">
            <strong>${r.info.name}</strong>
            <div class="comp-bar-track"><div class="comp-bar-fill" style="width:${pct}%"></div></div>
            <span class="mono">${label}</span>
          </div>`;
      })
      .join("");

    tbody.innerHTML = results
      .map(
        (r) => `
      <tr>
        <td><strong>${r.info.name}</strong></td>
        <td class="mono">${r.elapsedMicroseconds.toFixed(2)} µs</td>
        <td class="mono">${r.comparisons.toLocaleString()}</td>
        <td class="mono">${r.swaps.toLocaleString()}</td>
        <td class="mono">${r.info.bestTime}</td>
        <td class="mono">${r.info.averageTime}</td>
        <td class="mono">${r.info.worstTime}</td>
        <td class="mono">${r.info.spaceComplexity}</td>
      </tr>`
      )
      .join("");
  }

  function initComparatorHandlers() {
    document.getElementById("compRunBtn").addEventListener("click", async () => {
      try {
        const customRaw = document.getElementById("compCustomInput").value.trim();
        const body = customRaw.length > 0
          ? { array: parseIntegerCsv(customRaw) }
          : {
              size: parseInt(document.getElementById("compSizeSelect").value, 10),
              pattern: document.getElementById("compPatternSelect").value,
            };
        const res = await api("/api/compare", { method: "POST", body: JSON.stringify(body) });
        state.comparator.results = res.results || [];
        document.getElementById("compRunMeta").textContent = `Benchmarked n=${res.inputSize} (${res.pattern})`;
        renderComparatorView();
      } catch (err) {
        toast(err.message, true);
      }
    });

    document.querySelectorAll("#compMetricTabs .seg-btn").forEach((btn) => {
      btn.addEventListener("click", () => {
        document.querySelectorAll("#compMetricTabs .seg-btn").forEach((b) => b.classList.remove("active"));
        btn.classList.add("active");
        state.comparator.metric = btn.dataset.metric;
        renderComparatorView();
      });
    });
  }

  // =========================================================================
  // Quiz Arena
  // =========================================================================

  function renderQuizQuestion() {
    const panel = document.getElementById("quizStagePanel");
    const qz = state.quiz;
    if (!qz.questions.length) {
      panel.innerHTML = `<p class="empty-muted">Select a topic and click Start Quiz. Question order is randomized in C++.</p>`;
      return;
    }

    const q = qz.questions[qz.index];
    const progressPct = Math.round(((qz.index + (qz.answered ? 1 : 0)) / qz.questions.length) * 100);

    panel.innerHTML = `
      <div class="block-head">
        <span class="accent-tag">${q.category}</span>
        <span class="mono-dim">Question ${qz.index + 1} / ${qz.questions.length} · Score ${qz.score}</span>
      </div>
      <div class="quiz-progress-track"><div class="quiz-progress-fill" style="width:${progressPct}%"></div></div>
      <h4 style="font-size: 1.05rem; font-weight: 600;">${q.question}</h4>
      <div class="quiz-options">
        ${q.options
          .map(
            (opt, idx) =>
              `<button class="quiz-opt" data-opt-idx="${idx}"><span class="quiz-keycap">${String.fromCharCode(65 + idx)}</span><span>${opt}</span></button>`
          )
          .join("")}
      </div>
      <div id="quizFeedbackBox"></div>
    `;

    panel.querySelectorAll(".quiz-opt").forEach((btn) => {
      btn.addEventListener("click", async () => {
        if (state.quiz.answered) return;
        const selectedIndex = parseInt(btn.dataset.optIdx, 10);
        try {
          const ans = await api("/api/quiz/answer", {
            method: "POST",
            body: JSON.stringify({ sessionId: qz.sessionId, questionId: q.id, selectedIndex }),
          });
          state.quiz.answered = true;
          state.quiz.score = ans.currentScore;

          panel.querySelectorAll(".quiz-opt").forEach((b, i) => {
            b.disabled = true;
            if (i === ans.correctIndex) b.classList.add("correct");
            else if (i === selectedIndex && !ans.correct) b.classList.add("wrong");
          });

          const isLast = qz.index >= qz.questions.length - 1;
          document.getElementById("quizFeedbackBox").innerHTML = `
            <div class="ds-feedback ${ans.correct ? "" : "error"}">
              <strong>${ans.correct ? "Correct" : "Incorrect"}:</strong> ${ans.explanation}
            </div>
            <div style="margin-top: 12px;">
              <button class="btn btn-accent" id="quizNextBtn">${isLast ? "Finish & View Results" : "Next Question"}</button>
            </div>
          `;
          document.getElementById("quizNextBtn").addEventListener("click", async () => {
            if (isLast) {
              const summary = await api("/api/quiz/finish", {
                method: "POST",
                body: JSON.stringify({ sessionId: qz.sessionId }),
              });
              renderQuizSummary(summary.record, summary.progress);
            } else {
              qz.index += 1;
              qz.answered = false;
              renderQuizQuestion();
            }
          });
        } catch (err) {
          toast(err.message, true);
        }
      });
    });
  }

  function renderQuizSummary(rec, progress) {
    renderProgress(progress);
    const panel = document.getElementById("quizStagePanel");
    panel.innerHTML = `
      <div class="block-head">
        <h4>Quiz Complete</h4>
        <span class="accent-tag">${rec.accuracy.toFixed(1)}% Accuracy</span>
      </div>
      <div class="complexity-grid">
        <div class="cx-box"><span>Score</span><strong>${rec.score} / ${rec.total}</strong></div>
        <div class="cx-box"><span>Topic</span><strong>${rec.category}</strong></div>
        <div class="cx-box"><span>Total Attempts</span><strong>${progress.totalQuizzesAttempted}</strong></div>
        <div class="cx-box"><span>Average Accuracy</span><strong>${progress.averageAccuracy.toFixed(1)}%</strong></div>
      </div>
      <p class="muted-text">Results persisted to C++ Learning Progress.</p>
    `;
  }

  function initQuizHandlers() {
    document.getElementById("quizStartBtn").addEventListener("click", async () => {
      try {
        const res = await api("/api/quiz/start", {
          method: "POST",
          body: JSON.stringify({
            category: document.getElementById("quizCategorySelect").value,
            count: parseInt(document.getElementById("quizCountSelect").value, 10),
          }),
        });
        state.quiz = {
          sessionId: res.sessionId,
          questions: res.questions || [],
          index: 0,
          score: 0,
          answered: false,
        };
        renderQuizQuestion();
      } catch (err) {
        toast(err.message, true);
      }
    });
  }

  // =========================================================================
  // Learning Progress & Continue Learning
  // =========================================================================

  function renderProgress(p) {
    if (!p) return;
    state.progress = p;

    document.getElementById("progCompletedNum").textContent = `${p.completedCount}/${p.totalModules}`;
    document.getElementById("progAttemptsNum").textContent = p.totalQuizzesAttempted;
    document.getElementById("progAvgNum").textContent = `${p.averageAccuracy.toFixed(1)}%`;
    document.getElementById("progPercentText").textContent = `${p.completionPercent.toFixed(0)}%`;
    document.getElementById("progBarFill").style.width = `${p.completionPercent}%`;

    document.getElementById("statModulesDone").textContent = `${p.completedCount}/${p.totalModules}`;
    document.getElementById("statQuizAvg").textContent = `${p.averageAccuracy.toFixed(1)}%`;
    document.getElementById("topbarProgressChip").textContent = `${p.completedCount}/${p.totalModules} modules`;

    const nextMod = (p.modules || []).find((m) => !m.completed);
    const continueTitle = document.getElementById("continueTitle");
    const continueDesc = document.getElementById("continueDesc");
    const continueBtn = document.getElementById("continueBtn");
    if (nextMod && MODULE_DESTINATIONS[nextMod.id]) {
      const dest = MODULE_DESTINATIONS[nextMod.id];
      continueTitle.textContent = nextMod.name;
      continueDesc.textContent = dest.desc;
      continueBtn.dataset.jump = dest.view;
      continueBtn.dataset.dsTab = dest.dsTab || "";
      continueBtn.textContent = "Resume Module";
    } else {
      continueTitle.textContent = "All Modules Explored";
      continueDesc.textContent = "Challenge yourself with a mixed Quiz Arena session.";
      continueBtn.dataset.jump = "quiz";
      continueBtn.dataset.dsTab = "";
      continueBtn.textContent = "Launch Quiz";
    }

    const modList = document.getElementById("progModulesList");
    modList.innerHTML = (p.modules || [])
      .map(
        (m) => `
      <div class="check-row">
        <div>
          <strong>${m.name}</strong>
          <span class="mono-dim" style="margin-left: 8px;">${m.completed ? "Completed" : "Pending"}</span>
        </div>
        <button class="btn btn-xs ${m.completed ? "btn-secondary" : "btn-ghost"}" data-toggle-mod="${m.id}" data-next-state="${!m.completed}">
          ${m.completed ? "Done" : "Mark Done"}
        </button>
      </div>`
      )
      .join("");

    modList.querySelectorAll("[data-toggle-mod]").forEach((btn) => {
      btn.addEventListener("click", async () => {
        try {
          const res = await api("/api/progress/module", {
            method: "POST",
            body: JSON.stringify({
              module: btn.dataset.toggleMod,
              completed: btn.dataset.nextState === "true",
            }),
          });
          renderProgress(res.progress);
        } catch (err) {
          toast(err.message, true);
        }
      });
    });

    const qList = document.getElementById("progQuizList");
    const hist = p.quizHistory || [];
    if (hist.length === 0) {
      qList.innerHTML = `<p class="empty-muted">No quizzes completed yet.</p>`;
    } else {
      qList.innerHTML = hist
        .map(
          (q) => `
        <div class="history-row">
          <div class="history-meta mono">
            <span>${q.category}</span>
            <span>${q.timestamp}</span>
          </div>
          <div>Score: <strong class="mono">${q.score}/${q.total}</strong> (${q.accuracy.toFixed(1)}%)</div>
        </div>`
        )
        .join("");
    }
  }

  async function loadProgress() {
    try {
      const res = await api("/api/progress");
      renderProgress(res.progress);
    } catch (err) {
      toast(err.message, true);
    }
  }

  function initProgressHandlers() {
    document.getElementById("progResetBtn").addEventListener("click", async () => {
      if (!(await confirmAction("Reset Progress", "Clear all module completions and saved quiz scores?"))) return;
      try {
        const res = await api("/api/progress", { method: "DELETE" });
        renderProgress(res.progress);
        toast("Learning progress reset.");
      } catch (err) {
        toast(err.message, true);
      }
    });
  }

  // =========================================================================
  // Code Library (Source + Complexity Matrix)
  // =========================================================================

  function renderCodeEntry(id) {
    const entry = state.codeEntries.find((e) => e.id === id) || state.codeEntries[0];
    if (!entry) return;
    state.activeCodeId = entry.id;

    document.querySelectorAll("#codeSelectorPills button").forEach((b) => {
      b.classList.toggle("btn-accent", b.dataset.codeId === entry.id);
      b.classList.toggle("btn-ghost", b.dataset.codeId !== entry.id);
    });

    document.getElementById("codeComplexityHeader").innerHTML = `
      <div class="block-head">
        <h4>${entry.name}</h4>
        <span class="mono-dim">${entry.category}</span>
      </div>
      <div class="complexity-grid">
        <div class="cx-box"><span>Best Time</span><strong>${entry.bestTime}</strong></div>
        <div class="cx-box"><span>Average Time</span><strong>${entry.averageTime}</strong></div>
        <div class="cx-box"><span>Worst Time</span><strong>${entry.worstTime}</strong></div>
        <div class="cx-box"><span>Space</span><strong>${entry.spaceComplexity}</strong></div>
      </div>
    `;
    document.getElementById("cppCodeView").innerHTML = highlightCpp(entry.cppCode);
    document.getElementById("pseudoCodeView").textContent = entry.pseudocode;
  }

  async function loadCodeViewer() {
    try {
      const res = await api("/api/code");
      state.codeEntries = res.entries || [];
      const pills = document.getElementById("codeSelectorPills");
      pills.innerHTML = state.codeEntries
        .map((e) => `<button class="btn btn-ghost btn-sm" data-code-id="${e.id}">${e.name}</button>`)
        .join("");
      pills.querySelectorAll("button").forEach((btn) => {
        btn.addEventListener("click", () => renderCodeEntry(btn.dataset.codeId));
      });
      if (state.codeEntries.length > 0) {
        renderCodeEntry(state.codeEntries[0].id);
      }
    } catch (err) {
      toast(err.message, true);
    }
  }

  function initCodeViewerHandlers() {
    document.querySelectorAll("#codeLibraryModeTabs .seg-btn").forEach((btn) => {
      btn.addEventListener("click", () => {
        document.querySelectorAll("#codeLibraryModeTabs .seg-btn").forEach((b) => b.classList.remove("active"));
        btn.classList.add("active");
        const isCode = btn.dataset.libMode === "code";
        document.getElementById("codeModePanel").style.display = isCode ? "block" : "none";
        document.getElementById("referenceModePanel").style.display = isCode ? "none" : "block";
        document.getElementById("copyCppCodeBtn").style.display = isCode ? "inline-flex" : "none";
      });
    });

    document.getElementById("copyCppCodeBtn").addEventListener("click", async () => {
      const text = document.getElementById("cppCodeView").textContent;
      if (!text) return;
      try {
        await navigator.clipboard.writeText(text);
        toast("Copied C++ source.");
      } catch {
        toast("Clipboard copy unavailable.", true);
      }
    });
  }

  // =========================================================================
  // Learning Ecosystem (Roadmap, Practice, LeetCode, Revision, Resources, Analytics)
  // =========================================================================

  function applyEcosystemData(data) {
    state.ecosystem = data;
    const prefs = data.preferences || {};
    const analytics = data.analytics || {};

    document.getElementById("dashGoalInput").value = prefs.learningGoal || "";
    document.getElementById("lcUsernameInput").value = prefs.leetcodeUsername || "";
    const profileBtn = document.getElementById("lcOpenProfileBtn");
    profileBtn.href = prefs.leetcodeUsername
      ? `https://leetcode.com/u/${encodeURIComponent(prefs.leetcodeUsername)}/`
      : "https://leetcode.com/problemset/";

    document.getElementById("statRoadmapDone").textContent = `${analytics.roadmapCompleted ?? 0}/${analytics.roadmapTotal ?? 18}`;
    document.getElementById("statProblemsSolved").textContent = analytics.problemsSolved ?? 0;

    renderRoadmap();
    populateTopicFilters();
    renderPracticeList();
    renderPracticeDetail();
    renderLeetCodeList();
    renderRevisionFlashcard();
    renderNeedsRevisionList();
    renderMistakesList();
    renderResources();
    renderAnalytics(analytics);
  }

  async function loadEcosystem() {
    try {
      const data = await api("/api/ecosystem");
      applyEcosystemData(data);
    } catch (err) {
      toast(err.message, true);
    }
  }

  async function refreshAnalytics() {
    try {
      const res = await api("/api/analytics");
      if (state.ecosystem) state.ecosystem.analytics = res.analytics;
      renderAnalytics(res.analytics);
    } catch (err) {
      toast(err.message, true);
    }
  }

  function renderRoadmap() {
    if (!state.ecosystem) return;
    const topics = state.ecosystem.roadmap || [];
    const done = topics.filter((t) => t.completed).length;
    const pct = topics.length ? Math.round((done / topics.length) * 100) : 0;
    document.getElementById("roadmapSummaryText").textContent = `${done} / ${topics.length} topics completed (${pct}%)`;
    document.getElementById("roadmapProgressFill").style.width = `${pct}%`;

    const filter = state.roadmapStageFilter;
    const stages = ["FOUNDATIONS", "DATA STRUCTURES", "ALGORITHMS"].filter(
      (s) => filter === "all" || s === filter
    );

    const container = document.getElementById("roadmapContainer");
    container.innerHTML = stages
      .map((stage) => {
        const items = topics.filter((t) => t.stage === stage);
        return `
          <div class="stage-block">
            <div class="stage-heading">${stage}</div>
            ${items
              .map((t) => {
                const diffCls = t.difficulty === "Beginner" ? "easy" : t.difficulty === "Intermediate" ? "medium" : "hard";
                const prereq = t.prerequisites.length ? `Prereq: ${t.prerequisites.join(", ")}` : "No prerequisites";
                return `
                  <div class="roadmap-row${t.completed ? " completed" : ""}">
                    <div class="roadmap-head">
                      <div class="roadmap-meta">
                        <strong>${t.title}</strong>
                        <span class="badge ${diffCls}">${t.difficulty}</span>
                        <span class="mono-dim">${prereq}</span>
                      </div>
                      <div class="btn-group">
                        <button class="btn btn-ghost btn-xs" data-rm-viz="${t.relatedVisualizer}">Visualizer</button>
                        <button class="btn btn-ghost btn-xs" data-rm-prac="${t.title}">Practice (${t.relatedProblemIds.length})</button>
                        <button class="btn btn-xs ${t.completed ? "btn-accent" : "btn-secondary"}" data-rm-toggle="${t.id}" data-rm-next="${!t.completed}">
                          ${t.completed ? "Completed" : "Mark Complete"}
                        </button>
                      </div>
                    </div>
                    <p class="muted-text">${t.explanation}</p>
                  </div>`;
              })
              .join("")}
          </div>`;
      })
      .join("");

    container.querySelectorAll("[data-rm-toggle]").forEach((btn) => {
      btn.addEventListener("click", async () => {
        try {
          const data = await api("/api/roadmap/toggle", {
            method: "POST",
            body: JSON.stringify({
              topicId: btn.dataset.rmToggle,
              completed: btn.dataset.rmNext === "true",
            }),
          });
          applyEcosystemData(data);
        } catch (err) {
          toast(err.message, true);
        }
      });
    });

    container.querySelectorAll("[data-rm-viz]").forEach((btn) => {
      btn.addEventListener("click", () => {
        const parts = btn.dataset.rmViz.split(":");
        switchView(parts[0], parts[1] || null);
      });
    });

    container.querySelectorAll("[data-rm-prac]").forEach((btn) => {
      btn.addEventListener("click", () => {
        const topicSel = document.getElementById("practiceTopicFilter");
        if ([...topicSel.options].some((o) => o.value === btn.dataset.rmPrac)) {
          topicSel.value = btn.dataset.rmPrac;
        } else {
          topicSel.value = "all";
        }
        renderPracticeList();
        switchView("practice");
      });
    });
  }

  function populateTopicFilters() {
    if (!state.ecosystem) return;
    const topics = Array.from(new Set((state.ecosystem.practice || []).map((p) => p.topic))).sort();
    const pSel = document.getElementById("practiceTopicFilter");
    const prevP = pSel.value;
    pSel.innerHTML = `<option value="all">All Topics</option>${topics.map((t) => `<option value="${t}">${t}</option>`).join("")}`;
    if (topics.includes(prevP)) pSel.value = prevP;

    const lcTopics = Array.from(
      new Set([...topics, ...(state.ecosystem.leetcode || []).map((l) => l.topic)])
    ).sort();
    const lcSel = document.getElementById("lcFilterTopic");
    const prevL = lcSel.value;
    lcSel.innerHTML = `<option value="all">All Topics</option>${lcTopics.map((t) => `<option value="${t}">${t}</option>`).join("")}`;
    if (lcTopics.includes(prevL)) lcSel.value = prevL;
  }

  function renderPracticeList() {
    if (!state.ecosystem) return;
    const q = document.getElementById("practiceSearchInput").value.trim().toLowerCase();
    const topic = document.getElementById("practiceTopicFilter").value;
    const diff = document.getElementById("practiceDiffFilter").value;
    const flag = document.getElementById("practiceFlagFilter").value;

    const items = (state.ecosystem.practice || []).filter((p) => {
      if (q && !p.title.toLowerCase().includes(q) && !p.topic.toLowerCase().includes(q)) return false;
      if (topic !== "all" && p.topic !== topic) return false;
      if (diff !== "all" && p.difficulty !== diff) return false;
      if (flag === "solved" && !p.solved) return false;
      if (flag === "unsolved" && p.solved) return false;
      if (flag === "bookmarked" && !p.bookmarked) return false;
      if (flag === "revision" && !p.needsRevision) return false;
      return true;
    });

    document.getElementById("practiceCountText").textContent = `${items.length} problems`;
    if (!state.selectedPracticeId && items.length > 0) {
      state.selectedPracticeId = items[0].id;
    }

    const list = document.getElementById("practiceList");
    if (items.length === 0) {
      list.innerHTML = `<p class="empty-muted">No matching problems.</p>`;
      return;
    }
    list.innerHTML = items
      .map((p) => {
        const diffCls = p.difficulty.toLowerCase();
        const active = p.id === state.selectedPracticeId ? " active" : "";
        return `
          <div class="problem-item${active}" data-prob-id="${p.id}">
            <div>
              <strong>${p.title}</strong>
              <div class="mono-dim">${p.topic} · ${p.attempts} attempts</div>
            </div>
            <div class="btn-group">
              ${p.solved ? `<span class="accent-tag">Solved</span>` : ""}
              ${p.bookmarked ? `<span class="badge">Saved</span>` : ""}
              <span class="badge ${diffCls}">${p.difficulty}</span>
            </div>
          </div>`;
      })
      .join("");

    list.querySelectorAll("[data-prob-id]").forEach((el) => {
      el.addEventListener("click", () => {
        state.selectedPracticeId = el.dataset.probId;
        renderPracticeList();
        renderPracticeDetail();
      });
    });
  }

  function renderPracticeDetail() {
    if (!state.ecosystem) return;
    const pane = document.getElementById("practiceDetailPane");
    const p = (state.ecosystem.practice || []).find((x) => x.id === state.selectedPracticeId);
    if (!p) {
      pane.innerHTML = `<p class="empty-muted">Select any problem to inspect details.</p>`;
      return;
    }
    pane.innerHTML = `
      <div class="block-head">
        <h4>${p.title}</h4>
        <span class="badge ${p.difficulty.toLowerCase()}">${p.difficulty} · ${p.topic}</span>
      </div>
      <p class="muted-text">${p.prompt}</p>
      <div class="btn-group">
        <button class="btn btn-sm ${p.solved ? "btn-accent" : "btn-secondary"}" id="prSolveBtn">${p.solved ? "Solved" : "Mark Solved"}</button>
        <button class="btn btn-ghost btn-sm" id="prAttemptBtn">Log Attempt (${p.attempts})</button>
        <button class="btn btn-ghost btn-sm" id="prBookmarkBtn">${p.bookmarked ? "Bookmarked" : "Bookmark"}</button>
        <button class="btn btn-ghost btn-sm" id="prRevisionBtn">${p.needsRevision ? "Needs Revision" : "Flag Revision"}</button>
      </div>
      <details class="info-callout">
        <summary style="cursor:pointer; font-weight:600; color:var(--text);">Hint &amp; Reference Approach</summary>
        <p style="margin-top:8px;"><strong>Hint:</strong> ${p.hint}</p>
        <p style="margin-top:6px;"><strong>Approach:</strong> ${p.defaultApproach}</p>
      </details>
      <div class="field">
        <label for="prApproachInput">Your Solution Approach</label>
        <textarea id="prApproachInput" rows="2" placeholder="Summarize time/space approach...">${p.userApproach || ""}</textarea>
      </div>
      <div class="field">
        <label for="prNotesInput">Personal Notes</label>
        <textarea id="prNotesInput" rows="3" placeholder="Edge cases, bugs, or takeaways...">${p.notes || ""}</textarea>
      </div>
      <button class="btn btn-secondary btn-sm" id="prSaveNotesBtn">Save Notes</button>
    `;

    const runAction = async (action, extra = {}) => {
      try {
        const data = await api("/api/practice/update", {
          method: "POST",
          body: JSON.stringify({ problemId: p.id, action, ...extra }),
        });
        applyEcosystemData(data);
      } catch (err) {
        toast(err.message, true);
      }
    };

    document.getElementById("prSolveBtn").addEventListener("click", () => runAction("toggle_solve"));
    document.getElementById("prAttemptBtn").addEventListener("click", () => runAction("attempt"));
    document.getElementById("prBookmarkBtn").addEventListener("click", () => runAction("toggle_bookmark"));
    document.getElementById("prRevisionBtn").addEventListener("click", () => runAction("toggle_revision"));
    document.getElementById("prSaveNotesBtn").addEventListener("click", () => {
      runAction("save_notes", {
        approach: document.getElementById("prApproachInput").value,
        notes: document.getElementById("prNotesInput").value,
      });
      toast("Problem notes saved.");
    });
  }

  function renderLeetCodeList() {
    if (!state.ecosystem) return;
    const topic = document.getElementById("lcFilterTopic").value;
    const diff = document.getElementById("lcFilterDiff").value;
    const status = document.getElementById("lcFilterStatus").value;

    const items = (state.ecosystem.leetcode || []).filter((l) => {
      if (topic !== "all" && l.topic !== topic) return false;
      if (diff !== "all" && l.difficulty !== diff) return false;
      if (status !== "all" && l.status !== status) return false;
      return true;
    });

    document.getElementById("lcTrackedCount").textContent = items.length;
    const list = document.getElementById("lcProblemList");
    if (items.length === 0) {
      list.innerHTML = `<p class="empty-muted">No LeetCode problems tracked yet. Add one above.</p>`;
      return;
    }

    list.innerHTML = items
      .map(
        (l) => `
      <div class="roadmap-row">
        <div class="roadmap-head">
          <div class="roadmap-meta">
            <strong>${l.title}</strong>
            <span class="badge ${l.difficulty.toLowerCase()}">${l.difficulty}</span>
            <span class="badge">${l.topic}</span>
          </div>
          <div class="btn-group">
            <select data-lc-status="${l.id}">
              ${["Not Started", "Attempted", "Solved", "Needs Revision"]
                .map((s) => `<option value="${s}"${l.status === s ? " selected" : ""}>${s}</option>`)
                .join("")}
            </select>
            <button class="btn btn-xs ${l.bookmarked ? "btn-accent" : "btn-ghost"}" data-lc-bookmark="${l.id}">
              ${l.bookmarked ? "Bookmarked" : "Bookmark"}
            </button>
            <a class="btn btn-ghost btn-xs" href="${l.url}" target="_blank" rel="noopener noreferrer">Open</a>
            <button class="btn btn-danger btn-xs" data-lc-del="${l.id}">Delete</button>
          </div>
        </div>
        <div class="inline-input">
          <input type="text" data-lc-note-input="${l.id}" value="${(l.notes || "").replace(/"/g, "&quot;")}" placeholder="Add personal note..." style="flex:1;" />
          <button class="btn btn-ghost btn-xs" data-lc-save-note="${l.id}">Save Note</button>
        </div>
      </div>`
      )
      .join("");

    list.querySelectorAll("[data-lc-status]").forEach((sel) => {
      sel.addEventListener("change", async () => {
        try {
          const data = await api("/api/leetcode/update", {
            method: "POST",
            body: JSON.stringify({ id: sel.dataset.lcStatus, status: sel.value }),
          });
          applyEcosystemData(data);
        } catch (err) {
          toast(err.message, true);
        }
      });
    });

    list.querySelectorAll("[data-lc-bookmark]").forEach((btn) => {
      btn.addEventListener("click", async () => {
        try {
          const data = await api("/api/leetcode/update", {
            method: "POST",
            body: JSON.stringify({ id: btn.dataset.lcBookmark, toggleBookmark: true }),
          });
          applyEcosystemData(data);
        } catch (err) {
          toast(err.message, true);
        }
      });
    });

    list.querySelectorAll("[data-lc-save-note]").forEach((btn) => {
      btn.addEventListener("click", async () => {
        const id = btn.dataset.lcSaveNote;
        const notes = list.querySelector(`[data-lc-note-input="${id}"]`).value;
        try {
          const data = await api("/api/leetcode/update", {
            method: "POST",
            body: JSON.stringify({ id, notes }),
          });
          applyEcosystemData(data);
          toast("LeetCode note saved.");
        } catch (err) {
          toast(err.message, true);
        }
      });
    });

    list.querySelectorAll("[data-lc-del]").forEach((btn) => {
      btn.addEventListener("click", async () => {
        if (!(await confirmAction("Delete Problem", "Remove this problem from your LeetCode tracker?"))) return;
        try {
          const data = await api("/api/leetcode/delete", {
            method: "POST",
            body: JSON.stringify({ id: btn.dataset.lcDel }),
          });
          applyEcosystemData(data);
        } catch (err) {
          toast(err.message, true);
        }
      });
    });
  }

  function renderRevisionFlashcard() {
    if (!state.ecosystem) return;
    const due = state.ecosystem.analytics?.revisionDueCount ?? 0;
    document.getElementById("revSummaryChip").textContent = `${due} due today`;

    const allCards = state.ecosystem.flashcards || [];
    const cards = state.flashcardCategory === "all"
      ? allCards
      : allCards.filter((c) => c.category === state.flashcardCategory);

    const stage = document.getElementById("flashcardStage");
    if (cards.length === 0) {
      stage.innerHTML = `<p class="empty-muted">No flashcards in this category.</p>`;
      document.getElementById("fcPositionText").textContent = "0 / 0";
      return;
    }

    const idx = state.flashcardIndex % cards.length;
    state.flashcardIndex = idx;
    const card = cards[idx];
    document.getElementById("fcPositionText").textContent = `${idx + 1} / ${cards.length}`;

    stage.innerHTML = `
      <div class="block-head">
        <span class="accent-tag">${card.category}</span>
        <span class="mono-dim">Next: ${card.nextReviewDate || "Today"} · Last: ${card.lastRating} (${card.reviewCount} reviews)</span>
      </div>
      <h4 style="font-size: 1.12rem; margin: 12px 0;">${card.front}</h4>
      ${
        state.flashcardRevealed
          ? `<div class="info-callout" style="color:var(--text);">${card.back}</div>`
          : `<button class="btn btn-secondary btn-sm" id="fcRevealBtn" style="align-self:flex-start;">Reveal Explanation</button>`
      }
      <div class="transport-bar">
        <div class="btn-group">
          <button class="btn btn-danger btn-sm" data-fc-rate="again">Again</button>
          <button class="btn btn-secondary btn-sm" data-fc-rate="difficult">Difficult</button>
          <button class="btn btn-accent btn-sm" data-fc-rate="good">Good</button>
          <button class="btn btn-ghost btn-sm" data-fc-rate="easy">Easy</button>
        </div>
        <button class="btn btn-ghost btn-xs" id="fcNextOnlyBtn">Skip</button>
      </div>
    `;

    const revBtn = document.getElementById("fcRevealBtn");
    if (revBtn) {
      revBtn.addEventListener("click", () => {
        state.flashcardRevealed = true;
        renderRevisionFlashcard();
      });
    }

    document.getElementById("fcNextOnlyBtn").addEventListener("click", () => {
      state.flashcardIndex = (idx + 1) % cards.length;
      state.flashcardRevealed = false;
      renderRevisionFlashcard();
    });

    stage.querySelectorAll("[data-fc-rate]").forEach((btn) => {
      btn.addEventListener("click", async () => {
        try {
          const data = await api("/api/revision/review", {
            method: "POST",
            body: JSON.stringify({ cardId: card.id, rating: btn.dataset.fcRate }),
          });
          state.flashcardIndex = (idx + 1) % cards.length;
          state.flashcardRevealed = false;
          applyEcosystemData(data);
        } catch (err) {
          toast(err.message, true);
        }
      });
    });
  }

  function renderNeedsRevisionList() {
    if (!state.ecosystem) return;
    const pRev = (state.ecosystem.practice || []).filter((p) => p.needsRevision);
    const lRev = (state.ecosystem.leetcode || []).filter((l) => l.status === "Needs Revision");
    const box = document.getElementById("revNeedsList");

    if (pRev.length === 0 && lRev.length === 0) {
      box.innerHTML = `<p class="empty-muted">No problems currently flagged for revision.</p>`;
      return;
    }

    box.innerHTML = [
      ...pRev.map(
        (p) => `
        <div class="recent-row">
          <div><strong>${p.title}</strong> <span class="badge">${p.topic}</span></div>
          <button class="btn btn-ghost btn-xs" data-rev-open-pr="${p.id}">Open in Practice</button>
        </div>`
      ),
      ...lRev.map(
        (l) => `
        <div class="recent-row">
          <div><strong>${l.title}</strong> <span class="badge">LeetCode · ${l.topic}</span></div>
          <a class="btn btn-ghost btn-xs" href="${l.url}" target="_blank" rel="noopener noreferrer">Open LeetCode</a>
        </div>`
      ),
    ].join("");

    box.querySelectorAll("[data-rev-open-pr]").forEach((btn) => {
      btn.addEventListener("click", () => {
        state.selectedPracticeId = btn.dataset.revOpenPr;
        renderPracticeList();
        renderPracticeDetail();
        switchView("practice");
      });
    });
  }

  function renderMistakesList() {
    if (!state.ecosystem) return;
    const list = state.ecosystem.mistakes || [];
    const box = document.getElementById("mistakesList");
    if (list.length === 0) {
      box.innerHTML = `<p class="empty-muted">No saved mistakes yet.</p>`;
      return;
    }
    box.innerHTML = list
      .map(
        (m) => `
      <div class="roadmap-row">
        <div class="roadmap-head">
          <div><strong>${m.title}</strong> <span class="badge">${m.topic}</span> <span class="mono-dim">${m.createdAt}</span></div>
          <button class="btn btn-ghost btn-xs" data-del-mistake="${m.id}">Resolve</button>
        </div>
        <p class="muted-text">${m.note}</p>
      </div>`
      )
      .join("");

    box.querySelectorAll("[data-del-mistake]").forEach((btn) => {
      btn.addEventListener("click", async () => {
        if (!(await confirmAction("Resolve Mistake", "Remove this mistake note from your log?"))) return;
        try {
          const data = await api("/api/revision/mistake", {
            method: "POST",
            body: JSON.stringify({ op: "remove", id: btn.dataset.delMistake }),
          });
          applyEcosystemData(data);
        } catch (err) {
          toast(err.message, true);
        }
      });
    });
  }

  function renderResources() {
    if (!state.ecosystem) return;
    const grid = document.getElementById("resourcesGrid");
    grid.innerHTML = (state.ecosystem.resources || [])
      .map(
        (r) => `
      <article class="resource-card">
        <div class="resource-head">
          <div style="display:flex; align-items:center; gap:10px;">
            <span class="resource-icon" aria-hidden="true">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8">
                <path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"></path>
                <polyline points="15 3 21 3 21 9"></polyline>
                <line x1="10" y1="14" x2="21" y2="3"></line>
              </svg>
            </span>
            <div>
              <strong>${r.name}</strong>
              <div class="mono-dim">${r.category}</div>
            </div>
          </div>
          <a class="btn btn-secondary btn-sm" href="${r.url}" target="_blank" rel="noopener noreferrer">Open Website</a>
        </div>
        <p class="muted-text">${r.description}</p>
      </article>`
      )
      .join("");
  }

  function renderAnalytics(a) {
    if (!a) return;
    document.getElementById("anSolved").textContent = a.problemsSolved ?? 0;
    document.getElementById("anAttempted").textContent = a.problemsAttempted ?? 0;
    document.getElementById("anTopicPct").textContent = `${(a.topicCompletionPercent ?? 0).toFixed(0)}%`;
    document.getElementById("anQuizAvg").textContent = `${(a.quizAvgAccuracy ?? 0).toFixed(1)}%`;
    document.getElementById("anRevReviews").textContent = a.revisionTotalReviews ?? 0;
    document.getElementById("anNeedsRev").textContent = a.needsRevisionCount ?? 0;

    const weekly = a.weeklyActivity || [];
    const maxW = Math.max(...weekly.map((d) => d.count), 1);
    document.getElementById("anWeeklyBars").innerHTML = weekly
      .map((d) => {
        const pct = d.count === 0 ? 4 : Math.max(10, Math.round((d.count / maxW) * 100));
        return `
          <div class="week-col" title="${d.date}: ${d.count} actions">
            <span class="mono-dim">${d.count}</span>
            <div class="week-bar" style="height:${pct}%"></div>
            <span class="mono-dim">${d.date.slice(5)}</span>
          </div>`;
      })
      .join("");

    const heat = a.dailyHeatmap || [];
    document.getElementById("anHeatmapGrid").innerHTML = heat
      .map((d) => {
        let lvl = "";
        if (d.count >= 5) lvl = " l3";
        else if (d.count >= 2) lvl = " l2";
        else if (d.count >= 1) lvl = " l1";
        return `<div class="heat-cell${lvl}" title="${d.date}: ${d.count} activities">${d.date.slice(8)}</div>`;
      })
      .join("");

    const topics = a.mostPracticedTopics || [];
    const topMax = Math.max(...topics.map((t) => t.count), 1);
    document.getElementById("anTopTopics").innerHTML = topics.length
      ? topics
          .map((t) => {
            const pct = Math.max(10, Math.round((t.count / topMax) * 100));
            return `
              <div class="act-bar-item">
                <span>${t.topic}</span>
                <div class="act-track"><div class="act-fill" style="width:${pct}%"></div></div>
                <span class="mono" style="text-align:right;">${t.count}</span>
              </div>`;
          })
          .join("")
      : `<p class="empty-muted">Solve or attempt problems to see topic distribution.</p>`;

    document.getElementById("anRevisionStats").innerHTML = `
      <div class="check-row"><span>Roadmap Topics Completed</span><strong class="mono">${a.roadmapCompleted} / ${a.roadmapTotal}</strong></div>
      <div class="check-row"><span>LeetCode Solved / Tracked</span><strong class="mono">${a.leetcodeSolved} / ${a.leetcodeTotal}</strong></div>
      <div class="check-row"><span>Quizzes Attempted</span><strong class="mono">${a.quizAttempts}</strong></div>
      <div class="check-row"><span>Flashcards Due Today</span><strong class="mono">${a.revisionDueCount}</strong></div>
      <div class="check-row"><span>Saved Mistake Notes</span><strong class="mono">${a.mistakesCount}</strong></div>
    `;
  }

  function initEcosystemHandlers() {
    document.getElementById("dashSaveGoalBtn").addEventListener("click", async () => {
      try {
        const data = await api("/api/preferences", {
          method: "POST",
          body: JSON.stringify({ learningGoal: document.getElementById("dashGoalInput").value.trim() }),
        });
        applyEcosystemData(data);
        toast("Learning goal saved.");
      } catch (err) {
        toast(err.message, true);
      }
    });

    document.querySelectorAll("#roadmapStageTabs .seg-btn").forEach((btn) => {
      btn.addEventListener("click", () => {
        document.querySelectorAll("#roadmapStageTabs .seg-btn").forEach((b) => b.classList.remove("active"));
        btn.classList.add("active");
        state.roadmapStageFilter = btn.dataset.stage;
        renderRoadmap();
      });
    });

    ["practiceSearchInput", "practiceTopicFilter", "practiceDiffFilter", "practiceFlagFilter"].forEach((id) => {
      document.getElementById(id).addEventListener("input", renderPracticeList);
      document.getElementById(id).addEventListener("change", renderPracticeList);
    });

    document.getElementById("lcSaveUserBtn").addEventListener("click", async () => {
      try {
        const data = await api("/api/preferences", {
          method: "POST",
          body: JSON.stringify({ leetcodeUsername: document.getElementById("lcUsernameInput").value.trim() }),
        });
        applyEcosystemData(data);
        toast("LeetCode username saved.");
      } catch (err) {
        toast(err.message, true);
      }
    });

    document.getElementById("lcAddBtn").addEventListener("click", async () => {
      try {
        const data = await api("/api/leetcode/add", {
          method: "POST",
          body: JSON.stringify({
            title: document.getElementById("lcAddTitle").value.trim(),
            url: document.getElementById("lcAddUrl").value.trim(),
            difficulty: document.getElementById("lcAddDifficulty").value,
            topic: document.getElementById("lcAddTopic").value.trim(),
            status: document.getElementById("lcAddStatus").value,
            notes: document.getElementById("lcAddNotes").value.trim(),
          }),
        });
        document.getElementById("lcAddTitle").value = "";
        document.getElementById("lcAddUrl").value = "";
        document.getElementById("lcAddNotes").value = "";
        applyEcosystemData(data);
        toast("LeetCode problem added.");
      } catch (err) {
        toast(err.message, true);
      }
    });

    ["lcFilterTopic", "lcFilterDiff", "lcFilterStatus"].forEach((id) => {
      document.getElementById(id).addEventListener("change", renderLeetCodeList);
    });

    document.querySelectorAll("#revisionTabs .seg-btn").forEach((btn) => {
      btn.addEventListener("click", () => {
        document.querySelectorAll("#revisionTabs .seg-btn").forEach((b) => b.classList.remove("active"));
        btn.classList.add("active");
        document.querySelectorAll(".rev-panel").forEach((p) => p.classList.remove("active"));
        document.getElementById(`rev-tab-${btn.dataset.revTab}`).classList.add("active");
      });
    });

    document.querySelectorAll("#flashcardCatTabs .seg-btn").forEach((btn) => {
      btn.addEventListener("click", () => {
        document.querySelectorAll("#flashcardCatTabs .seg-btn").forEach((b) => b.classList.remove("active"));
        btn.classList.add("active");
        state.flashcardCategory = btn.dataset.fcCat;
        state.flashcardIndex = 0;
        state.flashcardRevealed = false;
        renderRevisionFlashcard();
      });
    });

    document.getElementById("mistakeAddBtn").addEventListener("click", async () => {
      try {
        const data = await api("/api/revision/mistake", {
          method: "POST",
          body: JSON.stringify({
            op: "add",
            title: document.getElementById("mistakeTitleInput").value.trim(),
            topic: document.getElementById("mistakeTopicInput").value.trim(),
            note: document.getElementById("mistakeNoteInput").value.trim(),
          }),
        });
        document.getElementById("mistakeTitleInput").value = "";
        document.getElementById("mistakeNoteInput").value = "";
        applyEcosystemData(data);
        toast("Saved mistake note.");
      } catch (err) {
        toast(err.message, true);
      }
    });

    document.getElementById("analyticsRefreshBtn").addEventListener("click", () => {
      refreshAnalytics();
      loadIntelligenceAnalytics();
    });
  }

  // =========================================================================
  // DSA Intelligence Hub (Catalogue, Detail/Notes, Topics, Solutions, LC, Rev)
  // =========================================================================

  function escapeHtml(s) {
    return String(s || "")
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }

  function renderSimpleMarkdown(md) {
    const src = String(md || "");
    const parts = src.split(/(```[\s\S]*?```)/g);
    return parts
      .map((part) => {
        if (part.startsWith("```") && part.endsWith("```")) {
          const code = part.replace(/^```[^\n]*\n?/, "").replace(/```$/, "");
          return `<pre class="code-block"><code>${highlightCpp(code.trim())}</code></pre>`;
        }
        return escapeHtml(part)
          .replace(/`([^`]+)`/g, "<code>$1</code>")
          .replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>")
          .replace(/\n/g, "<br />");
      })
      .join("");
  }

  async function loadLibrary() {
    try {
      const res = await api("/api/library/query", {
        method: "POST",
        body: JSON.stringify({
          search: document.getElementById("libSearchInput").value.trim(),
          difficulty: document.getElementById("libDiffFilter").value,
          topic: document.getElementById("libTopicFilter").value,
          availability: document.getElementById("libAvailFilter").value,
          status: document.getElementById("libStatusFilter").value,
          sortBy: document.getElementById("libSortBy").value,
          sortDir: document.getElementById("libSortDir").value,
          page: state.library.page,
          pageSize: state.library.pageSize,
        }),
      });

      state.library.items = res.items || [];
      state.library.page = res.page || 1;
      state.library.totalPages = res.totalPages || 1;
      state.library.topics = res.availableTopics || [];

      const topicSel = document.getElementById("libTopicFilter");
      const prevTopic = topicSel.value;
      topicSel.innerHTML = `<option value="all">All Topics / Tags</option>${state.library.topics
        .map((t) => `<option value="${escapeHtml(t)}">${escapeHtml(t)}</option>`)
        .join("")}`;
      if (state.library.topics.includes(prevTopic)) topicSel.value = prevTopic;

      const solTopicSel = document.getElementById("solTopicFilter");
      if (solTopicSel) {
        const prevSolT = solTopicSel.value;
        solTopicSel.innerHTML = `<option value="all">All Topics</option>${state.library.topics
          .map((t) => `<option value="${escapeHtml(t)}">${escapeHtml(t)}</option>`)
          .join("")}`;
        if (state.library.topics.includes(prevSolT)) solTopicSel.value = prevSolT;
      }

      const lastSync = res.profile?.lastCatalogSync || "Never synced";
      document.getElementById("libSyncMeta").textContent = `${res.totalCatalogue} problems · Sync: ${lastSync}`;
      document.getElementById("dashCatalogCount").textContent = `${res.totalCatalogue} in library`;
      document.getElementById("libCountMeta").textContent = `${res.totalFiltered} matching`;
      document.getElementById("libPageInfo").textContent = `Page ${res.page} of ${res.totalPages}`;
      document.getElementById("libPrevPageBtn").disabled = res.page <= 1;
      document.getElementById("libNextPageBtn").disabled = res.page >= res.totalPages;

      const tbody = document.getElementById("libTableBody");
      if (state.library.items.length === 0) {
        tbody.innerHTML = `<tr><td colspan="7" class="empty-muted">No problems in catalogue yet. Click "Sync LeetCode Catalogue" or "Import JSON".</td></tr>`;
        return;
      }

      tbody.innerHTML = state.library.items
        .map(({ problem: p, progress: pr, hasNotes, solutionCount }) => {
          const tagsHtml = (p.topicTags || [])
            .slice(0, 3)
            .map((t) => `<span class="badge">${escapeHtml(t)}</span>`)
            .join("");
          return `
            <tr>
              <td class="mono">${p.id}</td>
              <td>
                <button class="btn btn-ghost btn-xs" data-open-detail="${p.slug}" style="font-weight:600; color:var(--text);">
                  ${escapeHtml(p.title)}
                </button>
                ${hasNotes ? `<span class="accent-tag" style="margin-left:4px;">Notes</span>` : ""}
                ${solutionCount ? `<span class="badge" style="margin-left:4px;">${solutionCount} sol</span>` : ""}
              </td>
              <td><div class="tag-row">${tagsHtml}</div></td>
              <td><span class="badge ${p.difficulty.toLowerCase()}">${p.difficulty}</span></td>
              <td class="mono-dim">${p.isPremium ? "Premium" : "Free"}</td>
              <td>
                <select data-lib-status="${p.slug}">
                  ${["Not Started", "Attempted", "Solved", "Needs Revision"]
                    .map((s) => `<option value="${s}"${pr.status === s ? " selected" : ""}>${s}</option>`)
                    .join("")}
                </select>
              </td>
              <td>
                <div class="btn-group">
                  <button class="btn btn-xs ${pr.bookmarked ? "btn-accent" : "btn-ghost"}" data-lib-bm="${p.slug}">
                    ${pr.bookmarked ? "Saved" : "Save"}
                  </button>
                  <button class="btn btn-secondary btn-xs" data-open-detail="${p.slug}">Detail</button>
                  <a class="btn btn-ghost btn-xs" href="${p.url}" target="_blank" rel="noopener noreferrer">LC</a>
                </div>
              </td>
            </tr>`;
        })
        .join("");

      tbody.querySelectorAll("[data-open-detail]").forEach((btn) => {
        btn.addEventListener("click", () => openProblemDetail(btn.dataset.openDetail));
      });

      tbody.querySelectorAll("[data-lib-status]").forEach((sel) => {
        sel.addEventListener("change", async () => {
          try {
            await api("/api/library/status", {
              method: "POST",
              body: JSON.stringify({ slug: sel.dataset.libStatus, status: sel.value }),
            });
            loadTopicsProgress();
            loadIntelligenceAnalytics();
          } catch (err) {
            toast(err.message, true);
          }
        });
      });

      tbody.querySelectorAll("[data-lib-bm]").forEach((btn) => {
        btn.addEventListener("click", async () => {
          try {
            await api("/api/library/status", {
              method: "POST",
              body: JSON.stringify({ slug: btn.dataset.libBm, toggleBookmark: true }),
            });
            loadLibrary();
          } catch (err) {
            toast(err.message, true);
          }
        });
      });
    } catch (err) {
      toast(err.message, true);
    }
  }

  async function openProblemDetail(slug) {
    try {
      const detail = await api("/api/library/detail", {
        method: "POST",
        body: JSON.stringify({ slug }),
      });
      state.activeProblemSlug = slug;
      state.notesPreview = false;
      document.getElementById("pdNotesEditorGrid").style.display = "grid";
      document.getElementById("pdNotesPreviewBox").style.display = "none";

      const p = detail.problem;
      const pr = detail.progress;
      const n = detail.notes;

      document.getElementById("pdStatusSelect").value = pr.status || "Not Started";
      document.getElementById("pdBookmarkBtn").textContent = pr.bookmarked ? "Bookmarked" : "Bookmark";
      document.getElementById("pdBookmarkBtn").className = `btn btn-sm ${pr.bookmarked ? "btn-accent" : "btn-secondary"}`;
      document.getElementById("pdLeetCodeLink").href = p.url;

      document.getElementById("pdHeaderCard").innerHTML = `
        <div class="block-head">
          <div>
            <span class="mono-dim">#${p.id} · ${p.isPremium ? "Premium" : "Free"}</span>
            <h3 style="margin-top:2px;">${escapeHtml(p.title)}</h3>
          </div>
          <span class="badge ${p.difficulty.toLowerCase()}">${p.difficulty}</span>
        </div>
        <div class="tag-row">${(p.topicTags || []).map((t) => `<span class="badge">${escapeHtml(t)}</span>`).join("")}</div>
        <div class="metrics-strip">
          <span class="telemetry-item">First Attempt <strong class="mono">${pr.firstAttemptDate || "-"}</strong></span>
          <span class="telemetry-item">Latest Attempt <strong class="mono">${pr.latestAttemptDate || "-"}</strong></span>
          <span class="telemetry-item">Attempts <strong class="mono">${pr.attemptCount || 0}</strong></span>
          <span class="telemetry-item">Source <strong class="mono">${pr.source || "manual"}</strong></span>
        </div>
      `;

      document.getElementById("pdNoteIntuition").value = n.intuition || "";
      document.getElementById("pdNoteBrute").value = n.bruteForce || "";
      document.getElementById("pdNoteOpt").value = n.optimized || "";
      document.getElementById("pdNoteTime").value = n.timeComplexity || "";
      document.getElementById("pdNoteSpace").value = n.spaceComplexity || "";
      document.getElementById("pdNoteMistakes").value = n.mistakesMade || "";
      document.getElementById("pdNoteObs").value = n.observations || "";
      document.getElementById("pdNoteCode").value = n.cppSnippet || "";
      document.getElementById("pdNoteRev").value = n.revisionComments || "";
      document.getElementById("pdSaveIndicator").textContent = n.updatedAt ? `Saved ${n.updatedAt}` : "Ready";

      const sols = detail.solutions || [];
      document.getElementById("pdSolutionsList").innerHTML = sols.length
        ? sols
            .map(
              (s) => `
          <div class="ref-card">
            <div class="block-head">
              <strong>${escapeHtml(s.approachName)}</strong>
              <div class="btn-group">
                <span class="badge">${escapeHtml(s.timeComplexity)} / ${escapeHtml(s.spaceComplexity)}</span>
                <button class="btn btn-danger btn-xs" data-pd-del-sol="${s.id}">Delete</button>
              </div>
            </div>
            <p class="muted-text">${escapeHtml(s.explanation)}</p>
            <pre class="code-block"><code>${highlightCpp(s.cppCode)}</code></pre>
          </div>`
            )
            .join("")
        : `<p class="empty-muted">No saved solution approaches for this problem yet.</p>`;

      document.querySelectorAll("[data-pd-del-sol]").forEach((b) => {
        b.addEventListener("click", async () => {
          if (!(await confirmAction("Delete Solution", "Remove this saved solution approach?"))) return;
          await api("/api/solutions/delete", {
            method: "POST",
            body: JSON.stringify({ id: b.dataset.pdDelSol }),
          });
          openProblemDetail(slug);
        });
      });

      const subs = detail.submissions || [];
      document.getElementById("pdSubHistoryList").innerHTML = subs.length
        ? subs
            .map(
              (sub) => `
          <div class="recent-row">
            <div><strong>${escapeHtml(sub.source)}</strong> <span class="mono-dim">${sub.timestamp}</span></div>
            <span class="accent-tag">Accepted</span>
          </div>`
            )
            .join("")
        : `<p class="empty-muted">No recorded submissions for this problem yet.</p>`;

      switchView("problem-detail");
    } catch (err) {
      toast(err.message, true);
    }
  }

  async function saveActiveProblemNotes(silent = false) {
    if (!state.activeProblemSlug) return;
    document.getElementById("pdSaveIndicator").textContent = "Saving...";
    try {
      const res = await api("/api/library/notes", {
        method: "POST",
        body: JSON.stringify({
          slug: state.activeProblemSlug,
          intuition: document.getElementById("pdNoteIntuition").value,
          bruteForce: document.getElementById("pdNoteBrute").value,
          optimized: document.getElementById("pdNoteOpt").value,
          timeComplexity: document.getElementById("pdNoteTime").value,
          spaceComplexity: document.getElementById("pdNoteSpace").value,
          mistakesMade: document.getElementById("pdNoteMistakes").value,
          observations: document.getElementById("pdNoteObs").value,
          cppSnippet: document.getElementById("pdNoteCode").value,
          revisionComments: document.getElementById("pdNoteRev").value,
        }),
      });
      document.getElementById("pdSaveIndicator").textContent = `Saved ${res.notes.updatedAt}`;
      if (!silent) toast("Problem notes saved.");
    } catch (err) {
      document.getElementById("pdSaveIndicator").textContent = "Save failed";
      toast(err.message, true);
    }
  }

  async function loadTopicsProgress() {
    try {
      const res = await api("/api/topics");
      const topics = res.topics || [];
      const grid = document.getElementById("topicsGrid");
      if (topics.length === 0) {
        grid.innerHTML = `<p class="empty-muted">Import or sync the Question Library first to compute per-topic progress.</p>`;
        return;
      }

      grid.innerHTML = topics
        .map(
          (t) => `
        <div class="ref-card">
          <div class="block-head">
            <strong>${escapeHtml(t.topic)}</strong>
            <span class="mono-dim">${t.solvedProblems} / ${t.totalProblems} (${t.completionPercent.toFixed(1)}%)</span>
          </div>
          <div class="progress-track">
            <div class="progress-fill" style="width:${Math.min(100, t.completionPercent)}%"></div>
          </div>
          <div class="metrics-strip">
            <span class="telemetry-item">Attempted <strong class="mono">${t.attemptedProblems}</strong></span>
            <span class="telemetry-item">Remaining <strong class="mono">${t.remainingProblems}</strong></span>
            <span class="telemetry-item">E/M/H <strong class="mono">${t.easySolved}/${t.mediumSolved}/${t.hardSolved}</strong></span>
          </div>
          <div class="inline-input">
            <span class="mono-dim" style="align-self:center;">Goal:</span>
            <input type="number" min="1" max="5000" value="${t.goalTarget}" data-topic-goal-input="${escapeHtml(t.topic)}" style="width:90px;" />
            <button class="btn btn-secondary btn-xs" data-topic-goal-save="${escapeHtml(t.topic)}">Set Goal</button>
          </div>
        </div>`
        )
        .join("");

      grid.querySelectorAll("[data-topic-goal-save]").forEach((btn) => {
        btn.addEventListener("click", async () => {
          const topic = btn.dataset.topicGoalSave;
          const inp = grid.querySelector(`[data-topic-goal-input="${CSS.escape(topic)}"]`);
          try {
            await api("/api/topics/goal", {
              method: "POST",
              body: JSON.stringify({ topic, goalTarget: parseInt(inp.value, 10) }),
            });
            toast(`Updated goal for ${topic}.`);
          } catch (err) {
            toast(err.message, true);
          }
        });
      });
    } catch (err) {
      toast(err.message, true);
    }
  }

  async function loadSolutionsLibrary() {
    try {
      const res = await api("/api/solutions/search", {
        method: "POST",
        body: JSON.stringify({
          query: document.getElementById("solSearchInput").value.trim(),
          topic: document.getElementById("solTopicFilter").value,
          complexity: document.getElementById("solCompFilter").value,
        }),
      });
      const list = res.solutions || [];
      document.getElementById("solCountText").textContent = `${list.length} solutions`;
      const grid = document.getElementById("solutionsLibraryGrid");
      if (list.length === 0) {
        grid.innerHTML = `<p class="empty-muted">No saved solutions yet. Open any problem in Question Library to save approaches.</p>`;
        return;
      }
      grid.innerHTML = list
        .map(
          (s) => `
        <div class="ref-card">
          <div class="block-head">
            <div>
              <strong>#${s.problemId || "-"} ${escapeHtml(s.problemTitle || s.slug)}</strong>
              <div class="mono-dim">${escapeHtml(s.approachName)} · ${escapeHtml(s.timeComplexity)} / ${escapeHtml(s.spaceComplexity)}</div>
            </div>
            <button class="btn btn-secondary btn-xs" data-sol-open="${s.slug}">Open Problem</button>
          </div>
          <p class="muted-text">${escapeHtml(s.explanation)}</p>
          <pre class="code-block"><code>${highlightCpp(s.cppCode)}</code></pre>
        </div>`
        )
        .join("");

      grid.querySelectorAll("[data-sol-open]").forEach((b) => {
        b.addEventListener("click", () => openProblemDetail(b.dataset.solOpen));
      });
    } catch (err) {
      toast(err.message, true);
    }
  }

  async function loadNotesLibrary() {
    try {
      const res = await api("/api/notes/search", {
        method: "POST",
        body: JSON.stringify({ query: document.getElementById("solSearchInput").value.trim() }),
      });
      const notes = res.notes || [];
      document.getElementById("solCountText").textContent = `${notes.length} notes`;
      const grid = document.getElementById("notesLibraryGrid");
      if (notes.length === 0) {
        grid.innerHTML = `<p class="empty-muted">No matching study notes found. Open any problem in Question Library to add notes.</p>`;
        return;
      }
      grid.innerHTML = notes
        .map(
          (n) => `
        <div class="ref-card">
          <div class="block-head">
            <div>
              <strong>${escapeHtml(n.slug)}</strong>
              <div class="mono-dim">${escapeHtml(n.timeComplexity || "-")} / ${escapeHtml(n.spaceComplexity || "-")} · ${n.updatedAt || ""}</div>
            </div>
            <button class="btn btn-secondary btn-xs" data-note-open="${n.slug}">Open Notes</button>
          </div>
          ${n.intuition ? `<p class="muted-text"><strong>Intuition:</strong> ${escapeHtml(n.intuition)}</p>` : ""}
          ${n.optimized ? `<p class="muted-text"><strong>Optimized:</strong> ${escapeHtml(n.optimized)}</p>` : ""}
          ${n.mistakesMade ? `<p class="muted-text"><strong>Mistakes:</strong> ${escapeHtml(n.mistakesMade)}</p>` : ""}
          ${n.cppSnippet ? `<pre class="code-block"><code>${highlightCpp(n.cppSnippet)}</code></pre>` : ""}
        </div>`
        )
        .join("");

      grid.querySelectorAll("[data-note-open]").forEach((b) => {
        b.addEventListener("click", () => openProblemDetail(b.dataset.noteOpen));
      });
    } catch (err) {
      toast(err.message, true);
    }
  }

  async function loadLeetCodeProfile() {
    try {
      const res = await api("/api/leetcode/profile");
      const p = res.profile || {};
      document.getElementById("lcPubTotal").textContent = p.totalSolved ?? 0;
      document.getElementById("lcPubEasy").textContent = p.easySolved ?? 0;
      document.getElementById("lcPubMedium").textContent = p.mediumSolved ?? 0;
      document.getElementById("lcPubHard").textContent = p.hardSolved ?? 0;
      document.getElementById("lcPubRank").textContent = p.ranking ? `#${p.ranking}` : "-";
      document.getElementById("lcExtCount").textContent = p.extensionEventsCount ?? 0;
      document.getElementById("lcSyncMeta").textContent = p.lastProfileSync
        ? `Synced ${p.lastProfileSync}`
        : p.lastSyncError || "Not synced yet";
      document.getElementById("extLastSeenText").textContent = p.extensionLastSeen || "None recorded";
      document.getElementById("extStatusBadge").textContent = p.extensionLastSeen ? "Active" : "Ready";

      const subs = res.submissions || [];
      const box = document.getElementById("lcRecentSubList");
      box.innerHTML = subs.length
        ? subs
            .map(
              (s) => `
          <div class="recent-row">
            <div>
              <strong>${escapeHtml(s.title)}</strong>
              <span class="badge">${escapeHtml(s.source)}</span>
            </div>
            <span class="mono-dim">${s.timestamp}</span>
          </div>`
            )
            .join("")
        : `<p class="empty-muted">No detected or imported submissions yet.</p>`;
    } catch (err) {
      toast(err.message, true);
    }
  }

  async function loadIntelRevisionQueue() {
    try {
      const res = await api("/api/intel/revision");
      const items = res.items || [];
      document.getElementById("intelRevCount").textContent = `${items.length} problems`;
      const box = document.getElementById("intelRevList");
      if (items.length === 0) {
        box.innerHTML = `<p class="empty-muted">No catalogue problems marked Needs Revision yet.</p>`;
        return;
      }
      box.innerHTML = items
        .map(
          ({ problem: p, progress: pr }) => `
        <div class="roadmap-row">
          <div class="roadmap-head">
            <div class="roadmap-meta">
              <strong>#${p.id} ${escapeHtml(p.title)}</strong>
              <span class="badge ${p.difficulty.toLowerCase()}">${p.difficulty}</span>
              <span class="mono-dim">Next: ${pr.nextReviewDate || "Today"} · Last: ${pr.lastRating} (${pr.reviewCount})</span>
            </div>
            <div class="btn-group">
              <button class="btn btn-danger btn-xs" data-irev-rate="${p.slug}" data-rate="again">Again</button>
              <button class="btn btn-secondary btn-xs" data-irev-rate="${p.slug}" data-rate="difficult">Difficult</button>
              <button class="btn btn-accent btn-xs" data-irev-rate="${p.slug}" data-rate="good">Good</button>
              <button class="btn btn-ghost btn-xs" data-irev-rate="${p.slug}" data-rate="easy">Easy</button>
              <button class="btn btn-ghost btn-xs" data-irev-open="${p.slug}">Notes</button>
            </div>
          </div>
        </div>`
        )
        .join("");

      box.querySelectorAll("[data-irev-rate]").forEach((btn) => {
        btn.addEventListener("click", async () => {
          try {
            await api("/api/intel/revision/review", {
              method: "POST",
              body: JSON.stringify({ slug: btn.dataset.irevRate, rating: btn.dataset.rate }),
            });
            loadIntelRevisionQueue();
          } catch (err) {
            toast(err.message, true);
          }
        });
      });

      box.querySelectorAll("[data-irev-open]").forEach((btn) => {
        btn.addEventListener("click", () => openProblemDetail(btn.dataset.irevOpen));
      });
    } catch (err) {
      toast(err.message, true);
    }
  }

  async function loadIntelligenceAnalytics() {
    try {
      const res = await api("/api/intel/analytics");
      const a = res.analytics;
      if (!a) return;

      document.getElementById("anSolved").textContent = a.totalSolved;
      document.getElementById("anAttempted").textContent = a.totalAttempted;
      document.getElementById("anDiffSplit").textContent = `${a.solvedEasy} / ${a.solvedMedium} / ${a.solvedHard}`;
      document.getElementById("anStreak").textContent = `${a.currentStreak}d`;
      document.getElementById("anTopicPct").textContent = `${a.overallTopicCompletionPercent.toFixed(1)}%`;
      document.getElementById("statProblemsSolved").textContent = a.totalSolved;
      document.getElementById("dashProgressFill").style.width = `${Math.min(100, a.overallTopicCompletionPercent)}%`;
      document.getElementById("dashProgressLabel").textContent = `${a.totalSolved} solved · ${a.overallTopicCompletionPercent.toFixed(1)}% topic coverage`;

      const topDash = a.mostPracticedTopics || [];
      const dashTopicsEl = document.getElementById("dashTopicsPreview");
      if (dashTopicsEl) {
        dashTopicsEl.innerHTML = topDash.length
          ? topDash.slice(0, 4).map((t) => `
              <div class="act-bar-item">
                <span>${escapeHtml(t.topic)}</span>
                <div class="act-track"><div class="act-fill" style="width:${Math.min(100, Math.round(t.completionPercent))}%"></div></div>
                <span class="mono" style="text-align:right;">${t.solvedProblems}/${t.totalProblems}</span>
              </div>`).join("")
          : `<p class="empty-muted">Import or sync the Question Library to track topic progress.</p>`;
      }

      // Update Dashboard Recent Solved & Upcoming Revisions
      Promise.all([api("/api/leetcode/profile"), api("/api/intel/revision")]).then(([lcRes, revRes]) => {
        const subs = lcRes.submissions || [];
        const solvedEl = document.getElementById("dashRecentSolved");
        if (solvedEl) {
          solvedEl.innerHTML = subs.length
            ? subs.slice(0, 4).map((s) => `
                <div class="recent-row">
                  <strong>${escapeHtml(s.title)}</strong>
                  <span class="mono-dim">${s.date || s.timestamp}</span>
                </div>`).join("")
            : `<p class="empty-muted">No solved submissions recorded yet.</p>`;
        }
        const revs = revRes.items || [];
        const revEl = document.getElementById("dashUpcomingRevisions");
        if (revEl) {
          revEl.innerHTML = revs.length
            ? revs.slice(0, 4).map(({ problem: p, progress: pr }) => `
                <div class="recent-row">
                  <strong>#${p.id} ${escapeHtml(p.title)}</strong>
                  <span class="mono-dim">${pr.nextReviewDate || "Due"}</span>
                </div>`).join("")
            : `<p class="empty-muted">No revisions scheduled yet.</p>`;
        }
      }).catch(() => {});

      const weekly = a.weeklyActivity || [];
      const maxW = Math.max(...weekly.map((d) => d.count), 1);
      document.getElementById("anWeeklyBars").innerHTML = weekly
        .map((d) => {
          const pct = d.count === 0 ? 4 : Math.max(10, Math.round((d.count / maxW) * 100));
          return `
            <div class="week-col" title="${d.date}: ${d.count} actions">
              <span class="mono-dim">${d.count}</span>
              <div class="week-bar" style="height:${pct}%"></div>
              <span class="mono-dim">${d.date.slice(5)}</span>
            </div>`;
        })
        .join("");

      const heat = a.dailyActivityHeatmap || [];
      document.getElementById("anHeatmapGrid").innerHTML = heat
        .map((d) => {
          let lvl = "";
          if (d.count >= 5) lvl = " l3";
          else if (d.count >= 2) lvl = " l2";
          else if (d.count >= 1) lvl = " l1";
          return `<div class="heat-cell${lvl}" title="${d.date}: ${d.count} activities">${d.date.slice(8)}</div>`;
        })
        .join("");

      const top = a.mostPracticedTopics || [];
      const topMax = Math.max(...top.map((t) => t.solvedProblems + t.attemptedProblems), 1);
      document.getElementById("anTopTopics").innerHTML = top.length
        ? top
            .map((t) => {
              const cnt = t.solvedProblems + t.attemptedProblems;
              const pct = Math.max(8, Math.round((cnt / topMax) * 100));
              return `
                <div class="act-bar-item">
                  <span>${escapeHtml(t.topic)}</span>
                  <div class="act-track"><div class="act-fill" style="width:${pct}%"></div></div>
                  <span class="mono" style="text-align:right;">${t.solvedProblems}/${t.totalProblems}</span>
                </div>`;
            })
            .join("")
        : `<p class="empty-muted">Solve problems to track most practiced topics.</p>`;

      const weak = a.weakestTopics || [];
      document.getElementById("anRevisionStats").innerHTML = [
        `<div class="check-row"><span>Revision Reviews Completed</span><strong class="mono">${a.revisionCompleted}</strong></div>`,
        `<div class="check-row"><span>Revision Due / Flagged</span><strong class="mono">${a.revisionDue} due · ${a.needsRevisionCount} flagged</strong></div>`,
        `<div class="check-row"><span>Saved Notes &amp; Solutions</span><strong class="mono">${a.totalNotes} notes · ${a.totalSolutions} solutions</strong></div>`,
        ...weak.slice(0, 3).map(
          (w) => `<div class="check-row"><span>Weakest Topic: ${escapeHtml(w.topic)}</span><strong class="mono">${w.completionPercent.toFixed(1)}%</strong></div>`
        ),
      ].join("");
    } catch (err) {
      toast(err.message, true);
    }
  }

  function initIntelligenceHandlers() {
    document.getElementById("libSyncBtn").addEventListener("click", async () => {
      const btn = document.getElementById("libSyncBtn");
      btn.disabled = true;
      btn.textContent = "Syncing...";
      document.getElementById("libSyncMeta").textContent = "Syncing from LeetCode...";
      try {
        const res = await api("/api/library/sync", { method: "POST", body: "{}" });
        toast(`Synced ${res.totalCatalogue} LeetCode problems.`);
        loadLibrary();
        loadTopicsProgress();
      } catch (err) {
        toast(err.message, true);
        loadLibrary();
      } finally {
        btn.disabled = false;
        btn.textContent = "Sync LeetCode Catalogue";
      }
    });

    document.getElementById("libToggleImportBtn").addEventListener("click", () => {
      const p = document.getElementById("libImportPanel");
      p.style.display = p.style.display === "none" ? "flex" : "none";
    });

    document.getElementById("libImportFile").addEventListener("change", async (e) => {
      const file = e.target.files[0];
      if (!file) return;
      document.getElementById("libImportInput").value = await file.text();
    });

    document.getElementById("libImportBtn").addEventListener("click", async () => {
      try {
        const res = await api("/api/library/import", {
          method: "POST",
          body: document.getElementById("libImportInput").value.trim(),
        });
        toast(`Imported ${res.added} new problems.`);
        document.getElementById("libImportPanel").style.display = "none";
        loadLibrary();
        loadTopicsProgress();
      } catch (err) {
        toast(err.message, true);
      }
    });

    ["libSearchInput", "libDiffFilter", "libTopicFilter", "libAvailFilter", "libStatusFilter", "libSortBy", "libSortDir"].forEach((id) => {
      document.getElementById(id).addEventListener("input", () => {
        state.library.page = 1;
        loadLibrary();
      });
      document.getElementById(id).addEventListener("change", () => {
        state.library.page = 1;
        loadLibrary();
      });
    });

    document.getElementById("libPrevPageBtn").addEventListener("click", () => {
      if (state.library.page > 1) {
        state.library.page -= 1;
        loadLibrary();
      }
    });
    document.getElementById("libNextPageBtn").addEventListener("click", () => {
      if (state.library.page < state.library.totalPages) {
        state.library.page += 1;
        loadLibrary();
      }
    });

    // Problem Detail handlers
    document.getElementById("pdBackBtn").addEventListener("click", () => switchView("library"));
    document.querySelectorAll("#pdTabs .seg-btn").forEach((btn) => {
      btn.addEventListener("click", () => {
        document.querySelectorAll("#pdTabs .seg-btn").forEach((b) => b.classList.remove("active"));
        btn.classList.add("active");
        document.querySelectorAll(".pd-panel").forEach((p) => p.classList.remove("active"));
        document.getElementById(`pd-tab-${btn.dataset.pdTab}`).classList.add("active");
      });
    });

    document.getElementById("pdStatusSelect").addEventListener("change", async (e) => {
      if (!state.activeProblemSlug) return;
      await api("/api/library/status", {
        method: "POST",
        body: JSON.stringify({ slug: state.activeProblemSlug, status: e.target.value }),
      });
      openProblemDetail(state.activeProblemSlug);
    });

    document.getElementById("pdBookmarkBtn").addEventListener("click", async () => {
      if (!state.activeProblemSlug) return;
      await api("/api/library/status", {
        method: "POST",
        body: JSON.stringify({ slug: state.activeProblemSlug, toggleBookmark: true }),
      });
      openProblemDetail(state.activeProblemSlug);
    });

    document.getElementById("pdRevisionBtn").addEventListener("click", async () => {
      if (!state.activeProblemSlug) return;
      await api("/api/intel/revision/schedule", {
        method: "POST",
        body: JSON.stringify({ slug: state.activeProblemSlug, needsRevision: true }),
      });
      toast("Scheduled for revision.");
      openProblemDetail(state.activeProblemSlug);
    });

    document.getElementById("pdSaveNotesBtn").addEventListener("click", () => saveActiveProblemNotes(false));

    ["pdNoteIntuition", "pdNoteBrute", "pdNoteOpt", "pdNoteTime", "pdNoteSpace", "pdNoteMistakes", "pdNoteObs", "pdNoteRev", "pdNoteCode"].forEach((id) => {
      document.getElementById(id).addEventListener("input", () => {
        document.getElementById("pdSaveIndicator").textContent = "Unsaved changes...";
        clearTimeout(state.notesTimer);
        state.notesTimer = setTimeout(() => saveActiveProblemNotes(true), 900);
      });
    });

    document.getElementById("pdPreviewToggleBtn").addEventListener("click", () => {
      state.notesPreview = !state.notesPreview;
      const ed = document.getElementById("pdNotesEditorGrid");
      const pv = document.getElementById("pdNotesPreviewBox");
      if (state.notesPreview) {
        const md = [
          `**Intuition**\n${document.getElementById("pdNoteIntuition").value}`,
          `**Brute-force**\n${document.getElementById("pdNoteBrute").value}`,
          `**Optimized**\n${document.getElementById("pdNoteOpt").value}`,
          `**Complexity**: Time \`${document.getElementById("pdNoteTime").value}\` · Space \`${document.getElementById("pdNoteSpace").value}\``,
          `**Mistakes Made**\n${document.getElementById("pdNoteMistakes").value}`,
          `**Observations**\n${document.getElementById("pdNoteObs").value}`,
          `**Revision Comments**\n${document.getElementById("pdNoteRev").value}`,
          "```cpp\n" + document.getElementById("pdNoteCode").value + "\n```",
        ].join("\n\n");
        pv.innerHTML = renderSimpleMarkdown(md);
        ed.style.display = "none";
        pv.style.display = "block";
      } else {
        ed.style.display = "grid";
        pv.style.display = "none";
      }
    });

    document.getElementById("pdSolAddBtn").addEventListener("click", async () => {
      if (!state.activeProblemSlug) return;
      try {
        await api("/api/solutions/add", {
          method: "POST",
          body: JSON.stringify({
            slug: state.activeProblemSlug,
            approachName: document.getElementById("pdSolName").value.trim(),
            timeComplexity: document.getElementById("pdSolTime").value.trim(),
            spaceComplexity: document.getElementById("pdSolSpace").value.trim(),
            explanation: document.getElementById("pdSolExp").value.trim(),
            cppCode: document.getElementById("pdSolCode").value,
          }),
        });
        document.getElementById("pdSolName").value = "";
        document.getElementById("pdSolExp").value = "";
        document.getElementById("pdSolCode").value = "";
        toast("Saved solution approach.");
        openProblemDetail(state.activeProblemSlug);
      } catch (err) {
        toast(err.message, true);
      }
    });

    // Solutions & Notes Library search + mode tabs
    document.querySelectorAll("#solLibraryModeTabs .seg-btn").forEach((btn) => {
      btn.addEventListener("click", () => {
        document.querySelectorAll("#solLibraryModeTabs .seg-btn").forEach((b) => b.classList.remove("active"));
        btn.classList.add("active");
        const isSol = btn.dataset.solMode === "solutions";
        document.getElementById("solutionsLibraryGrid").style.display = isSol ? "grid" : "none";
        document.getElementById("notesLibraryGrid").style.display = isSol ? "none" : "grid";
        if (isSol) loadSolutionsLibrary();
        else loadNotesLibrary();
      });
    });

    ["solSearchInput", "solTopicFilter", "solCompFilter"].forEach((id) => {
      document.getElementById(id).addEventListener("input", () => {
        loadSolutionsLibrary();
        loadNotesLibrary();
      });
      document.getElementById(id).addEventListener("change", () => {
        loadSolutionsLibrary();
        loadNotesLibrary();
      });
    });

    document.getElementById("topicsRefreshBtn").addEventListener("click", loadTopicsProgress);

    // LeetCode Sync & Submission Import
    document.getElementById("lcSyncProfileBtn").addEventListener("click", async () => {
      const btn = document.getElementById("lcSyncProfileBtn");
      const username = document.getElementById("lcUsernameInput").value.trim();
      btn.disabled = true;
      btn.textContent = "Syncing...";
      document.getElementById("lcSyncMeta").textContent = "Syncing public profile...";
      try {
        await api("/api/leetcode/sync", {
          method: "POST",
          body: JSON.stringify({ username }),
        });
        toast(`Synced public profile for ${username}.`);
        loadLeetCodeProfile();
        loadEcosystem();
        loadIntelligenceAnalytics();
      } catch (err) {
        toast(err.message, true);
        loadLeetCodeProfile();
      } finally {
        btn.disabled = false;
        btn.textContent = "Sync Public Profile";
      }
    });

    document.getElementById("lcToggleImportSubBtn").addEventListener("click", () => {
      const box = document.getElementById("lcImportSubBox");
      box.style.display = box.style.display === "none" ? "flex" : "none";
    });

    document.getElementById("lcImportSubBtn").addEventListener("click", async () => {
      try {
        const res = await api("/api/leetcode/import-submissions", {
          method: "POST",
          body: document.getElementById("lcImportSubInput").value.trim(),
        });
        toast(`Imported ${res.imported} submissions.`);
        document.getElementById("lcImportSubBox").style.display = "none";
        loadLeetCodeProfile();
        loadIntelligenceAnalytics();
      } catch (err) {
        toast(err.message, true);
      }
    });
  }

  // =========================================================================
  // Navigation & Boot
  // =========================================================================

  function initNavigation() {
    document.querySelectorAll(".nav-item").forEach((btn) => {
      btn.addEventListener("click", () => switchView(btn.dataset.view));
    });

    document.querySelectorAll("[data-jump]").forEach((btn) => {
      btn.addEventListener("click", () => switchView(btn.dataset.jump, btn.dataset.dsTab || null));
    });

    document.getElementById("sidebarCollapseBtn").addEventListener("click", () => {
      document.getElementById("appShell").classList.toggle("sidebar-collapsed");
    });

    document.getElementById("mobileMenuBtn").addEventListener("click", () => {
      document.getElementById("sidebar").classList.toggle("open");
    });

    document.getElementById("refreshHistoryBtn").addEventListener("click", loadHistory);
    document.getElementById("clearHistoryBtn").addEventListener("click", async () => {
      if (!(await confirmAction("Clear Activity Log", "Delete all recorded entries in dsa_history.log?"))) return;
      try {
        await api("/api/history", { method: "DELETE" });
        await loadHistory();
        toast("Activity log cleared.");
      } catch (err) {
        toast(err.message, true);
      }
    });

    document.querySelectorAll("#refFilterGroup .seg-btn").forEach((btn) => {
      btn.addEventListener("click", () => {
        document.querySelectorAll("#refFilterGroup .seg-btn").forEach((b) => b.classList.remove("active"));
        btn.classList.add("active");
        renderReference(btn.dataset.filter);
      });
    });
  }

  async function boot() {
    initNavigation();
    initSortHandlers();
    initSearchHandlers();
    initDsHandlers();
    initBstHandlers();
    initGraphHandlers();
    initComparatorHandlers();
    initQuizHandlers();
    initProgressHandlers();
    initCodeViewerHandlers();
    initEcosystemHandlers();
    initIntelligenceHandlers();
    await Promise.all([
      loadOverview(),
      loadHistory(),
      loadDsState(),
      loadBstState(),
      loadGraphState(),
      loadProgress(),
      loadEcosystem(),
      loadLibrary(),
      loadIntelligenceAnalytics(),
    ]);
  }

  document.addEventListener("DOMContentLoaded", boot);
})();
