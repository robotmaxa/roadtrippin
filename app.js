/* Renders everything from window.SITE (data.js) and window.SITE_CODE (code/*.js).
   No frameworks, no build step: edit data.js, refresh. */
(function () {
  "use strict";
  const S = window.SITE;
  const CODE = window.SITE_CODE || {};
  const $ = (sel, el) => (el || document).querySelector(sel);

  /* ---------- helpers ---------- */
  function el(tag, attrs, ...children) {
    const node = document.createElement(tag);
    if (attrs) {
      for (const [k, v] of Object.entries(attrs)) {
        if (k === "class") node.className = v;
        else if (k === "html") node.innerHTML = v;
        else node.setAttribute(k, v);
      }
    }
    for (const c of children) {
      if (c == null) continue;
      node.append(c.nodeType ? c : document.createTextNode(c));
    }
    return node;
  }

  function h2(text) {
    const h = el("h2", null, text);
    h.append(el("span", { class: "rule", "aria-hidden": "true" }));
    return h;
  }

  function terminal(title, text) {
    const t = el("div", { class: "terminal" });
    const bar = el("div", { class: "terminal-bar" },
      el("span", { class: "dot" }), el("span", { class: "dot" }), el("span", { class: "dot" }),
      el("span", null, title));
    const pre = el("pre");
    pre.textContent = text;
    t.append(bar, pre);
    return t;
  }

  function dataTable(cols, rows, opts) {
    const wrapper = el("div", { class: "table-scroll" });
    const table = el("table");
    const thead = el("thead");
    const hr = el("tr");
    cols.forEach((c) => hr.append(el("th", { scope: "col" }, c)));
    thead.append(hr);
    const tbody = el("tbody");
    rows.forEach((r) => {
      const tr = el("tr");
      r.forEach((cell, i) => {
        const td = el("td", null, cell);
        if (opts && opts.highlightCol === i) td.classList.add("win");
        tr.append(td);
      });
      tbody.append(tr);
    });
    table.append(thead, tbody);
    wrapper.append(table);
    return wrapper;
  }

  /* ---------- masthead + rail ---------- */
  $("#site-eyebrow").textContent = S.meta.author;
  $("#site-title").innerHTML =
    escapeHtml(S.meta.title) + '<span class="sub">' + escapeHtml(S.meta.subtitle) + "</span>";
  $("#site-tagline").textContent = S.meta.tagline;
  $("#site-footer-text").textContent = S.meta.footer;

  function escapeHtml(s) {
    return s.replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
  }

  (function renderRail() {
    const rail = $("#mile-rail");

    /* One marker: miles left to the destination (the decreasing invariant)
       over miles actually driven to reach it (the increasing one). */
    function node(opts) {
      const n = el("div", { class: "rail-node" + (opts.terminus ? " terminus" : "") },
        el("span", { class: "post", "aria-hidden": "true" }));
      const mark = el("span", { class: "mark" }, String(opts.left));
      mark.append(el("small", null, " mi left"));
      n.append(mark);
      const driven = el("span", { class: "driven" }, String(opts.driven));
      driven.append(el("small", null, " mi driven"));
      n.append(driven);
      n.append(el("span", { class: "park" }, opts.name));
      return n;
    }

    rail.append(node({
      left: S.rail.originLeft, driven: 0,
      name: S.rail.origin + " · start", terminus: true,
    }));
    S.rail.stops.forEach((stop) => {
      rail.append(node({ left: stop.left, driven: stop.driven, name: stop.name }));
    });
    rail.append(node({
      left: 0, driven: S.rail.destinationDriven,
      name: S.rail.destination, terminus: true,
    }));
    $("#rail-caption").textContent = S.rail.caption;
  })();

  /* ---------- tabs ---------- */
  const tabbar = $("#tabbar");
  const main = $("#main");
  let current = null;

  S.tabs.forEach((t) => {
    const btn = el("button", {
      class: "tab", role: "tab", "aria-selected": "false", id: "tab-" + t.id,
    }, t.label);
    btn.addEventListener("click", () => go(t.id, true));
    tabbar.append(btn);
  });

  function go(id, push) {
    if (!S.tabs.some((t) => t.id === id)) id = S.tabs[0].id;
    current = id;
    S.tabs.forEach((t) => {
      $("#tab-" + t.id).setAttribute("aria-selected", String(t.id === id));
    });
    main.innerHTML = "";
    main.append(render(id));
    if (push) {
      history.replaceState(null, "", "#/" + id);
      window.scrollTo({ top: 0 });
    }
  }

  function render(id) {
    const panel = el("section", { class: "panel", role: "tabpanel" });
    if (id === "study") renderStudy(panel);
    else if (id === "compare") renderCompare(panel);
    else renderImpl(panel, id);
    return panel;
  }

  /* ---------- the essay ---------- */
  function renderStudy(panel) {
    const essay = el("article", { class: "essay" });
    S.blog.forEach((section) => {
      if (section.heading) essay.append(h2(section.heading));
      essay.append(el("div", { html: section.html }));
    });
    panel.append(essay);
  }

  /* ---------- implementation tabs ---------- */
  function renderImpl(panel, id) {
    const impl = S.implementations[id];
    if (!impl) { panel.textContent = "Missing implementation data: " + id; return; }

    const head = el("div", { class: "impl-head" });
    head.append(el("span", { class: "impl-attr" }, impl.attribution));
    head.append(h2(impl.name));
    head.append(el("p", { class: "impl-summary" }, impl.summary));
    panel.append(head);

    const facts = el("div", { class: "facts" });
    impl.facts.forEach((f) => {
      facts.append(el("div", { class: "fact" },
        el("div", { class: "k" }, f.k),
        el("div", { class: "v" }, f.v)));
    });
    panel.append(facts);

    const story = el("div", { class: "impl-story" });
    impl.story.forEach((p) => story.append(el("p", null, p)));
    panel.append(story);

    panel.append(h2("Build & run"));
    panel.append(el("div", { class: "build-cmd" }, impl.buildCmd));

    panel.append(h2("Real output"));
    if (impl.outputNote) panel.append(el("p", { class: "output-note" }, impl.outputNote));
    impl.outputs.forEach((o) => panel.append(terminal(o.title, o.text)));

    panel.append(h2("Browse the code"));
    panel.append(codeBrowser(id));
  }

  function codeBrowser(id) {
    const files = CODE[id] || [];
    const holder = el("div");
    if (!files.length) {
      holder.append(el("p", { class: "output-note" },
        "No embedded code for this tab yet. Add files under source/" + id +
        "/ and run tools/embed.py."));
      return holder;
    }
    const bar = el("div", { class: "codebar", role: "tablist" });
    const view = el("div", { class: "codeview" });
    const barTitle = el("span", null, "");
    const meta = el("span", null, "");
    view.append(el("div", { class: "terminal-bar" }, barTitle, meta));
    const pre = el("pre");
    const codeEl = el("code");
    pre.append(codeEl);
    view.append(pre);

    function show(i) {
      const f = files[i];
      barTitle.textContent = f.name;
      meta.textContent = f.lines + " lines";
      codeEl.innerHTML = "";
      const frag = document.createDocumentFragment();
      const lines = f.content.split("\n");
      lines.forEach((line, n) => {
        const span = el("span", { class: "line", "data-n": String(n + 1) });
        span.append(document.createTextNode(line.length ? line : " "));
        frag.append(span);
      });
      codeEl.append(frag);
      pre.scrollTop = 0;
      Array.from(bar.children).forEach((chip, j) =>
        chip.setAttribute("aria-selected", String(i === j)));
    }

    files.forEach((f, i) => {
      const chip = el("button", { class: "filechip", role: "tab", "aria-selected": "false" }, f.name);
      chip.append(el("span", { class: "ln" }, String(f.lines)));
      chip.addEventListener("click", () => show(i));
      bar.append(chip);
    });

    holder.append(bar, view);
    show(0);
    return holder;
  }

  /* ---------- comparison ---------- */
  function renderCompare(panel) {
    panel.append(h2("Head-to-Head"));
    panel.append(el("p", { class: "impl-summary" }, S.comparison.intro));
    panel.append(dataTable(S.comparison.main.cols, S.comparison.main.rows, { highlightCol: 1 }));

    panel.append(h2(S.comparison.results.title));
    panel.append(dataTable(S.comparison.results.cols, S.comparison.results.rows));
    panel.append(el("p", { class: "compare-note" }, S.comparison.results.note));

    panel.append(h2(S.comparison.scaling.title));
    panel.append(dataTable(S.comparison.scaling.cols, S.comparison.scaling.rows));
    panel.append(el("p", { class: "compare-note" }, S.comparison.scaling.note));
  }

  /* ---------- screenshot zoom ----------
     Figures in the essay are dense transcripts; click any one to see it full
     size. Click anywhere, or press Escape, to close. */
  (function lightbox() {
    let overlay = null;

    function close() {
      if (!overlay) return;
      overlay.remove();
      overlay = null;
    }

    function open(src, alt) {
      close();
      const img = el("img", { src: src, alt: alt || "" });
      overlay = el("div", { class: "lightbox", role: "dialog", "aria-modal": "true" },
        img, el("p", { class: "lightbox-hint" }, "click anywhere or press esc to close"));
      overlay.addEventListener("click", close);
      document.body.append(overlay);
    }

    document.addEventListener("click", (e) => {
      const img = e.target.closest && e.target.closest(".shot img");
      if (!img) return;
      open(img.getAttribute("src"), img.getAttribute("alt"));
    });

    document.addEventListener("keydown", (e) => {
      if (e.key === "Escape") close();
    });
  })();

  /* ---------- boot ---------- */
  const fromHash = (location.hash.match(/^#\/(.+)$/) || [])[1];
  go(fromHash || S.tabs[0].id, false);
  window.addEventListener("hashchange", () => {
    const id = (location.hash.match(/^#\/(.+)$/) || [])[1];
    if (id && id !== current) go(id, false);
  });
})();
